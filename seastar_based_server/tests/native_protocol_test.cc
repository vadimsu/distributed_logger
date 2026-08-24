// Integration test for the ClickHouse native-TCP-protocol storage backend.
//
// Exercises both layers:
//  1. ClickHouseNative::ClickHouseNativeConnection (native_protocol.hh) directly:
//     DDL, native Block-encoded INSERT, and a SELECT read back through the
//     same connection.
//  2. ClickHouseNativeStorage (generated/seastar_based_server/storage/clickhouse/clickhouse_native.hh):
//     globalInit() + Flush() with synthetic decoder-format packets, mirroring
//     exactly what the running server does with real client traffic.
//
// Requires a reachable ClickHouse server (defaults to 127.0.0.1:9000,
// user/password "default"/"default" - matches storage_config_clickhouse_native.json).
// Run with: bin/native_protocol_test [host] [port] [user] [password]

#include <seastar/core/app-template.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/util/log.hh>

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <exception>
#include <vector>

#include "../native_protocol.hh"
#include "clickhouse_native.hh"

namespace {

int g_failures = 0;

DistributedLogger::ClickHouseNative::ColumnBatch makeColumnBatch(
		const std::vector<std::pair<uint64_t, seastar::sstring>>& rows) {
	DistributedLogger::ClickHouseNative::ColumnBatch batch;
	batch.events.reserve(rows.size());
	batch.payloads.reserve(rows.size());
	for (const auto& row : rows) {
		batch.events.push_back(row.first);
		batch.total_payload_bytes += row.second.size();
		batch.payloads.push_back(row.second);
	}
	return batch;
}

void check(bool cond, const char* what) {
	if (cond) {
		fmt::print("[PASS] {}\n", what);
	} else {
		fmt::print("[FAIL] {}\n", what);
		++g_failures;
	}
}

// Builds a synthetic decoder-format packet matching decoder.hh's expectations:
// an 8-byte big-endian event id, followed by big-endian uint64 fields and/or
// 2-byte-big-endian-length-prefixed strings, in declaration order - exactly
// what real client-encoded LogEvent(...) packets look like on the wire.
seastar::temporary_buffer<char> makeEvent0Packet(uint64_t eventId, uint64_t shard, const std::string& host) {
	std::string buf;
	auto putU64 = [&buf](uint64_t v) {
		uint64_t be = __builtin_bswap64(v);
		buf.append(reinterpret_cast<const char*>(&be), 8);
	};
	auto putStr = [&buf](const std::string& s) {
		uint16_t len = static_cast<uint16_t>(s.size());
		uint16_t be = htons(len);
		buf.append(reinterpret_cast<const char*>(&be), 2);
		buf.append(s);
	};
	putU64(eventId);
	putU64(shard);
	putStr(host);
	return seastar::temporary_buffer<char>(buf.data(), buf.size());
}

seastar::temporary_buffer<char> makeEvent1Packet(uint64_t eventId, uint64_t shard, const std::string& host, uint64_t timestamp) {
	std::string buf;
	auto putU64 = [&buf](uint64_t v) {
		uint64_t be = __builtin_bswap64(v);
		buf.append(reinterpret_cast<const char*>(&be), 8);
	};
	auto putStr = [&buf](const std::string& s) {
		uint16_t len = static_cast<uint16_t>(s.size());
		uint16_t be = htons(len);
		buf.append(reinterpret_cast<const char*>(&be), 2);
		buf.append(s);
	};
	putU64(eventId);
	putU64(shard);
	putStr(host);
	putU64(timestamp);
	return seastar::temporary_buffer<char>(buf.data(), buf.size());
}

seastar::future<> testLowLevelConnection(seastar::sstring host, seastar::sstring port,
		seastar::sstring user, seastar::sstring password) {
	fmt::print("--- Low-level ClickHouseNativeConnection test ---\n");
	auto conn = co_await DistributedLogger::ClickHouseNative::ClickHouseNativeConnection::connect(
			host, port, "default", user, password);
	check(true, "connect() completed handshake without throwing");

	co_await conn->executeQuery("CREATE DATABASE IF NOT EXISTS native_protocol_test");
	co_await conn->executeQuery("DROP TABLE IF EXISTS native_protocol_test.raw");
	co_await conn->executeQuery(
			"CREATE TABLE native_protocol_test.raw (event UInt64, payload String) ENGINE = MergeTree() ORDER BY tuple()");
	check(true, "DDL statements executed");

	std::vector<std::pair<uint64_t, seastar::sstring>> rows;
	rows.emplace_back(1, seastar::sstring("{\"a\":1}"));
	rows.emplace_back(2, seastar::sstring("{\"a\":2}"));
	rows.emplace_back(1, seastar::sstring("{\"a\":3}"));
	co_await conn->insertRows("native_protocol_test.raw", makeColumnBatch(rows));
	check(true, "insertRows() completed without throwing");

	std::vector<std::vector<seastar::sstring>> outRows;
	co_await conn->executeQuery("SELECT count() FROM native_protocol_test.raw", &outRows);
	bool countOk = outRows.size() == 1 && outRows[0].size() == 1 &&
			outRows[0][0] == seastar::sstring("3");
	check(countOk, "SELECT count() via native protocol returns 3 after inserting 3 rows");

	outRows.clear();
	co_await conn->executeQuery("SELECT payload FROM native_protocol_test.raw WHERE event = 1 ORDER BY payload", &outRows);
	bool payloadOk = outRows.size() == 2 &&
			outRows[0][0] == seastar::sstring("{\"a\":1}") &&
			outRows[1][0] == seastar::sstring("{\"a\":3}");
	check(payloadOk, "SELECT payload via native protocol decodes String column correctly");

	// Reuse the same connection for a second, independent insertRows() call
	// to isolate whether connection reuse across multiple inserts corrupts data.
	std::vector<std::pair<uint64_t, seastar::sstring>> rows2;
	rows2.emplace_back(9, seastar::sstring("{\"b\":9}"));
	co_await conn->insertRows("native_protocol_test.raw", makeColumnBatch(rows2));
	outRows.clear();
	co_await conn->executeQuery("SELECT count() FROM native_protocol_test.raw", &outRows);
	check(outRows.size() == 1 && outRows[0][0] == seastar::sstring("4"),
			"second insertRows() on same connection brings count to 4");
	outRows.clear();
	co_await conn->executeQuery("SELECT payload FROM native_protocol_test.raw WHERE event = 9", &outRows);
	check(outRows.size() == 1 && outRows[0][0] == seastar::sstring("{\"b\":9}"),
			"second insertRows() payload decodes correctly (no corruption from reuse)");

	// Exception path: querying a nonexistent table must surface as
	// ClickHouseServerException, not hang or crash.
	bool threw = false;
	try {
		co_await conn->executeQuery("SELECT * FROM native_protocol_test.does_not_exist");
	} catch (const DistributedLogger::ClickHouseNative::ClickHouseServerException&) {
		threw = true;
	}
	check(threw, "querying a nonexistent table raises ClickHouseServerException");

	co_await conn->executeQuery("DROP TABLE IF EXISTS native_protocol_test.raw");
	co_await conn->close();
}

seastar::future<> testStorageFlush(seastar::sstring host, seastar::sstring port,
		seastar::sstring user, seastar::sstring password) {
	fmt::print("--- ClickHouseNativeStorage::Flush() integration test ---\n");
	// Isolated database so this test never touches the shared "events" data
	// used by the running server / other examples.
	seastar::sstring dbname = "native_protocol_test";

	auto bootstrap = co_await DistributedLogger::ClickHouseNative::ClickHouseNativeConnection::connect(
			host, port, "default", user, password);
	co_await bootstrap->executeQuery("CREATE DATABASE IF NOT EXISTS native_protocol_test");
	co_await bootstrap->executeQuery("DROP TABLE IF EXISTS native_protocol_test.events");
	co_await bootstrap->executeQuery("DROP TABLE IF EXISTS native_protocol_test.events_event0");
	co_await bootstrap->executeQuery("DROP TABLE IF EXISTS native_protocol_test.events_event1");
	co_await bootstrap->close();

	co_await DistributedLogger::ClickHouseNativeStorage::globalInit(host, port, dbname, user, password);
	check(true, "ClickHouseNativeStorage::globalInit() created base table + migrations");

	auto storage = DistributedLogger::ClickHouseNativeStorage::Init(host, port, dbname, user, password);

	std::vector<seastar::temporary_buffer<char>> batch;
	batch.push_back(makeEvent0Packet(DistributedLogger::Events::event0, 3, "node-1"));
	batch.push_back(makeEvent1Packet(DistributedLogger::Events::event1, 7, "node-2", 1710000000));
	batch.push_back(makeEvent0Packet(DistributedLogger::Events::event0, 4, "node-3"));

	co_await storage->Flush(std::move(batch));
	check(true, "Flush() completed without throwing");
	co_await storage->close();

	// Verify via a fresh connection that the rows landed both in the raw
	// table and were projected into the typed per-event tables by the
	// materialized views created in getMigrations().
	auto verify = co_await DistributedLogger::ClickHouseNative::ClickHouseNativeConnection::connect(
			host, port, dbname, user, password);

	std::vector<std::vector<seastar::sstring>> outRows;
	co_await verify->executeQuery("SELECT count() FROM native_protocol_test.events", &outRows);
	check(outRows.size() == 1 && outRows[0][0] == seastar::sstring("3"),
			"raw events table has 3 rows after Flush()");

	outRows.clear();
	co_await verify->executeQuery("SELECT count() FROM native_protocol_test.events_event0", &outRows);
	check(outRows.size() == 1 && outRows[0][0] == seastar::sstring("2"),
			"events_event0 materialized view projected 2 rows");

	outRows.clear();
	co_await verify->executeQuery(
			"SELECT Shard, Host FROM native_protocol_test.events_event1", &outRows);
	bool event1Ok = outRows.size() == 1 && outRows[0][0] == seastar::sstring("7") &&
			outRows[0][1] == seastar::sstring("node-2");
	check(event1Ok, "events_event1 materialized view projected typed Shard/Host columns");

	co_await verify->close();
}

} // namespace

int main(int argc, char** argv) {
	seastar::app_template app;
	app.add_options()
		("host", boost::program_options::value<std::string>()->default_value("127.0.0.1"), "ClickHouse host")
		("port", boost::program_options::value<std::string>()->default_value("9000"), "ClickHouse native TCP port")
		("user", boost::program_options::value<std::string>()->default_value("default"), "ClickHouse user")
		("password", boost::program_options::value<std::string>()->default_value("default"), "ClickHouse password");

	return app.run(argc, argv, [&app] () -> seastar::future<int> {
		auto& cfg = app.configuration();
		seastar::sstring host = cfg["host"].as<std::string>();
		seastar::sstring port = cfg["port"].as<std::string>();
		seastar::sstring user = cfg["user"].as<std::string>();
		seastar::sstring password = cfg["password"].as<std::string>();

		try {
			co_await testLowLevelConnection(host, port, user, password);
			co_await testStorageFlush(host, port, user, password);
		} catch (const std::exception& e) {
			fmt::print("[FAIL] unhandled exception: {}\n", e.what());
			++g_failures;
		}

		if (g_failures == 0) {
			fmt::print("\nAll native protocol tests passed.\n");
		} else {
			fmt::print("\n{} native protocol test(s) failed.\n", g_failures);
		}
		co_return g_failures == 0 ? 0 : 1;
	});
}
