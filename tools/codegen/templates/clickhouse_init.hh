#pragma once

#include <seastar/core/reactor.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/http/client.hh>
#include <seastar/http/request.hh>
#include <seastar/http/reply.hh>
#include "nlohmann/json.hpp"
#include "../storage.hh"
#include "../../event_decoder/event_decoder.hh"
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace DistributedLogger {

// HTTP based ClickHouse storage backend for the Seastar server.
//
// Init() allocates a seastar::http::experimental::client, connects it to the
// ClickHouse HTTP interface (host/port are supplied as arguments, default
// ClickHouse HTTP port is 8123) and ensures the generated per-event-type
// tables exist.
//
// Flush() groups a batch of decoded packets by event type and issues one
// bulk INSERT (FORMAT JSONEachRow) per event type through the HTTP client,
// mirroring the way the Go clickhouse.Flush groups rows before sending them
// to the server.
class ClickHouseStorage : public seastar::enable_lw_shared_from_this<ClickHouseStorage> {
public:
	ClickHouseStorage(seastar::socket_address addr, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password)
		: _addr(addr)
		, _client(seastar::make_lw_shared<seastar::http::experimental::client>())
		, _dbname(std::move(dbname))
		, _username(std::move(username))
		, _password(std::move(password))
		, _table("events") {
	}

	// Connects the HTTP client to ClickHouse and runs the generated schema
	// migrations. `host`/`port` describe the ClickHouse HTTP endpoint.
	static seastar::future<seastar::lw_shared_ptr<ClickHouseStorage>> Init(
			seastar::sstring host, seastar::sstring port, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password) {
		seastar::socket_address server_addr{seastar::ipv4_addr(std::string(host.c_str()), std::atoi(port.c_str()))};
		auto storage = seastar::make_lw_shared<ClickHouseStorage>(server_addr, std::move(dbname), std::move(username), std::move(password));
		return storage->_client->connect(server_addr).then([storage] (seastar::http::experimental::client::connection conn) {
			storage->_connection = seastar::make_lw_shared<seastar::http::experimental::client::connection>(std::move(conn));
			return storage->migrate(storage->GetMigrations());
		}).then([storage] {
			return storage;
		});
	}

	// Runs a set of DDL statements (typed per-event tables) sequentially.
	seastar::future<> migrate(std::vector<seastar::sstring> stmts) {
		return seastar::do_with(std::move(stmts), [this] (std::vector<seastar::sstring>& stmts) {
			return seastar::do_for_each(stmts, [this] (seastar::sstring& stmt) {
				return execute(stmt);
			});
		});
	}

	// Executes a query against the ClickHouse HTTP interface. When `body` is
	// non-empty it is streamed as the insert payload (e.g. FORMAT JSONEachRow
	// rows), otherwise the request carries no body (DDL/migrations).
	seastar::future<> execute(seastar::sstring query, seastar::sstring body = seastar::sstring()) {
		seastar::sstring uri = "/?query=" + url_encode(query);
		if (!_dbname.empty()) {
			uri += "&database=" + url_encode(_dbname);
		}
		return _client->make_request("POST", uri).then([this, body = std::move(body)] (auto req) mutable {
			if (!_username.empty()) {
				req._headers["X-ClickHouse-User"] = std::string(_username.c_str(), _username.size());
			}
			if (!_password.empty()) {
				req._headers["X-ClickHouse-Key"] = std::string(_password.c_str(), _password.size());
			}
			if (!body.empty()) {
				req._headers["Content-Type"] = "application/json";
				req.set_body(std::string(body.c_str(), body.size()));
			}
			return req.send();
		}).then([] (auto response) {
			auto status = response.get_status();
			if (status != seastar::http::reply::status_type::ok) {
				throw std::runtime_error("ClickHouse HTTP request failed with status " +
						std::to_string(static_cast<int>(status)));
			}
			return seastar::make_ready_future<>();
		});
	}

	seastar::future<> close() {
		if (_connection) {
			return _connection->close();
		}
		return seastar::make_ready_future<>();
	}

	// Groups the decoded packets in `batch` per event type and bulk inserts
	// each group into its typed table via the HTTP client. Generated below.
	seastar::future<> Flush(std::vector<seastar::temporary_buffer<char>> batch);

	// Returns the DDL statements creating the typed per-event tables. Generated below.
	std::vector<seastar::sstring> GetMigrations();

protected:
	static seastar::sstring url_encode(const seastar::sstring& value) {
		static const char* hex = "0123456789ABCDEF";
		std::string out;
		out.reserve(value.size());
		for (unsigned char c : value) {
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				out.push_back(static_cast<char>(c));
			} else if (c == ' ') {
				out.push_back('+');
			} else {
				out.push_back('%');
				out.push_back(hex[c >> 4]);
				out.push_back(hex[c & 0xF]);
			}
		}
		return seastar::sstring(out);
	}

	seastar::socket_address _addr;
	seastar::lw_shared_ptr<seastar::http::experimental::client> _client;
	seastar::lw_shared_ptr<seastar::http::experimental::client::connection> _connection;
	seastar::sstring _dbname;
	seastar::sstring _username;
	seastar::sstring _password;
	seastar::sstring _table;
};

// Flush() and GetMigrations() are generated below by SeastarServerCodeGen and
// defined out-of-line so they can be regenerated whenever the event schema
// changes without touching the boilerplate above.

