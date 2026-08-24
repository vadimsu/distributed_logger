#pragma once

#include <seastar/core/reactor.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>
#include "../storage.hh"
#include "../../../seastar_based_server/storage.hh"
#include "../../../seastar_based_server/native_protocol.hh"
#include "../../event_decoder/event_decoder.hh"
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace DistributedLogger {

// ClickHouse native-TCP-protocol storage backend for the Seastar server (see
// native_protocol.hh for the wire-level client). Mirrors ClickHouseStorage's
// (HTTP, clickhouse_init.hh) public shape and DDL statements; only the
// transport used by Flush()'s bulk inserts differs.
class ClickHouseNativeStorage : public Storage {
public:
	ClickHouseNativeStorage(seastar::sstring ip, seastar::sstring port, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password)
		: _ip(ip)
		, _port(port)
		, _dbname(std::move(dbname))
		, _username(std::move(username))
		, _password(std::move(password))
		, _table("events") {
	}

	seastar::future<> close() {
		if (!_conn) {
			return seastar::make_ready_future<>();
		}
		return _conn->close();
	}

	static seastar::future<> globalInit(seastar::sstring host, seastar::sstring port, seastar::sstring dbname, seastar::sstring username, seastar::sstring password){
		auto client = std::make_shared<ClickHouseNativeStorage>(host, port, std::move(dbname), std::move(username), std::move(password));
		seastar::sstring query = "CREATE TABLE IF NOT EXISTS events (event UInt64, payload String) ENGINE = MergeTree() ORDER BY tuple()";
		return client->execute(query, true).then([client]{
			auto stmts = client->getMigrations();
			return client->migrate(stmts).then([client]{
				return client->close();
			});
		});
	}

	// Connects lazily (on first execute()/insertRows() call) and runs the
	// generated schema migrations. `host`/`port` describe the ClickHouse
	// native TCP endpoint (default port 9000).
	static std::shared_ptr<ClickHouseNativeStorage> Init(
			seastar::sstring host, seastar::sstring port, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password) {
		return std::make_shared<ClickHouseNativeStorage>(host, port, std::move(dbname), std::move(username), std::move(password));
	}

	// Runs a set of DDL statements (typed per-event tables) sequentially.
	seastar::future<> migrate(std::vector<seastar::sstring> stmts) {
		return seastar::do_with(std::move(stmts), [this] (std::vector<seastar::sstring>& stmts) {
			return seastar::do_for_each(stmts.begin(), stmts.end(), [this] (seastar::sstring& stmt) {
				return execute(stmt, true);
			});
		});
	}

	seastar::future<> ensureConnected() {
		if (_conn) {
			return seastar::make_ready_future<>();
		}
		return ClickHouseNative::ClickHouseNativeConnection::connect(_ip, _port, _dbname, _username, _password)
			.then([this] (seastar::lw_shared_ptr<ClickHouseNative::ClickHouseNativeConnection> conn) {
				_conn = conn;
			});
	}

	// Executes a query against the ClickHouse native TCP interface. Used for
	// the best-effort DDL statements run from the constructor/migrations.
	seastar::future<> execute(seastar::sstring query, bool debug = false) {
		return ensureConnected().then([this, query]{
			return _conn->executeQuery(query);
		}).handle_exception([query, debug](std::exception_ptr ep) {
			if (debug) {
				fmt::print("{} {} ClickHouse native query failed: {} query {}\n", __FILE__, __LINE__, ep, query);
			}
			return seastar::make_ready_future<>();
		});
	}

	// Bulk-inserts a columnar (event, payload) batch for one event type into
	// `_table` via the native protocol's INSERT phase (Block-encoded, not text
	// VALUES). `batch` is moved so its vectors are handed straight to the
	// connection without an extra copy.
	seastar::future<> insertRows(ClickHouseNative::ColumnBatch batch) {
		return ensureConnected().then([this, batch = std::move(batch)] () mutable {
			return _conn->insertRows(_table, std::move(batch));
		}).handle_exception([this](std::exception_ptr ep) {
			fmt::print("{} {} ClickHouse native insert into {} failed: {}\n", __FILE__, __LINE__, _table, ep);
			return seastar::make_ready_future<>();
		});
	}

	// Groups the decoded packets in `batch` per event type and bulk inserts
	// each group into `_table` via the native protocol. Generated below.
	seastar::future<> Flush(std::vector<seastar::temporary_buffer<char>>&& batch) override;

	// Returns the DDL statements creating the typed per-event tables and the
	// materialized views projecting `payload` into typed columns. Generated below.
	std::vector<seastar::sstring> getMigrations();

protected:
	seastar::sstring _ip;
	seastar::sstring _port;
	seastar::sstring _dbname;
	seastar::sstring _username;
	seastar::sstring _password;
	seastar::sstring _table;
	seastar::lw_shared_ptr<ClickHouseNative::ClickHouseNativeConnection> _conn;
};

// Flush() and getMigrations() are generated below by SeastarServerCodeGen and
// defined out-of-line so they can be regenerated whenever the event schema
// changes without touching the boilerplate above.

