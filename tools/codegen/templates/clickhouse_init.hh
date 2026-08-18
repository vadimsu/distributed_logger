#pragma once

#include <seastar/core/reactor.hh>
#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/iostream.hh>
#include <seastar/core/thread.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/http/client.hh>
#include <seastar/http/request.hh>
#include <seastar/http/reply.hh>
#include "nlohmann/json.hpp"
#include "../storage.hh"
#include "../../../seastar_based_server/storage.hh"
#include "../../event_decoder/event_decoder.hh"
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

namespace DistributedLogger {

// HTTP based ClickHouse storage backend for the Seastar server.
//
// The constructor connects a seastar::http::client to the ClickHouse HTTP
// interface (host/port are supplied as arguments, default ClickHouse HTTP
// port is 8123), creates the base "events" table and runs the generated
// per-event-type migrations (typed tables + materialized views).
//
// Flush() groups a batch of decoded packets by event type and issues one
// bulk INSERT (FORMAT CSV) per event type through the HTTP client,
// mirroring the way the Go clickhouse.Flush groups rows before sending them
// to the server.
class ClickHouseStorage : public Storage {
public:
	ClickHouseStorage(seastar::sstring ip, seastar::sstring port, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password)
		: _ip(ip)
		, _port(port)
		, _client(nullptr)
		, _dbname(std::move(dbname))
		, _username(std::move(username))
		, _password(std::move(password))
		, _table("events"){
		auto sock = seastar::socket_address(ip, atoi(port.c_str()));
		_client = seastar::make_lw_shared<seastar::http::experimental::client>(sock);
	}

	static seastar::future<> globalInit(seastar::sstring host, seastar::sstring port, seastar::sstring dbname, seastar::sstring username, seastar::sstring password){
		auto client = std::make_shared<ClickHouseStorage>(host, port, std::move(dbname), std::move(username), std::move(password));
		seastar::sstring query = "CREATE TABLE IF NOT EXISTS events (event UInt64, payload String) ENGINE = MergeTree() ORDER BY tuple()";
		return client->execute(query, "",true).then([client]{
			auto stmts = client->getMigrations();
			return client->migrate(stmts).then([client]{
				return seastar::make_ready_future<>();
			});
		});
	}

	// Connects the HTTP client to ClickHouse and runs the generated schema
	// migrations. `host`/`port` describe the ClickHouse HTTP endpoint.
	static std::shared_ptr<ClickHouseStorage> Init(
			seastar::sstring host, seastar::sstring port, seastar::sstring dbname,
			seastar::sstring username, seastar::sstring password) {
		return std::make_shared<ClickHouseStorage>(host, port, std::move(dbname), std::move(username), std::move(password));
	}

	// Runs a set of DDL statements (typed per-event tables) sequentially.
	seastar::future<> migrate(std::vector<seastar::sstring> stmts) {
		return seastar::do_with(std::move(stmts), [this] (std::vector<seastar::sstring>& stmts) {
			return seastar::do_for_each(stmts, [this] (seastar::sstring& stmt) {
				return execute(stmt);
			});
		});
	}

	// Executes a query against the ClickHouse HTTP interface without waiting
	// for the reply to be consumed by the caller. Used for the best-effort
	// DDL statements run synchronously from the constructor.
	seastar::future<> execute(seastar::sstring query, seastar::sstring body = seastar::sstring(), bool debug=false) {
		seastar::sstring uri;
		if (query != ""){
		       uri = "/?query=" + url_encode(query);
		}
		if (!_dbname.empty()) {
			if (uri != ""){
				uri += "&";
			}else{
				uri = "/?";
			}
			uri += "database=" + url_encode(_dbname);
		}
		auto host = fmt::format("{}:{}",_ip,_port);
		auto req = seastar::http::request::make("POST", host, uri);
		if (!_username.empty()) {
			req._headers["X-ClickHouse-User"] = std::string(_username.c_str(), _username.size());
		}
		if (!_password.empty()) {
			req._headers["X-ClickHouse-Key"] = std::string(_password.c_str(), _password.size());
		}
		if (!body.empty()) {
			req._headers["Content-Type"] = "text/plain";
			req.write_body("text", body);
		}else{
			req._headers["Content-Length"] = "0";
		}
		return _client->make_request(std::move(req), [this, query, body, debug] (const seastar::http::reply& response, seastar::input_stream<char>&& in) {
			auto status = response._status;
			if (debug){
				fmt::print("{} {} status {} query {} body {}\n",__FILE__,__LINE__,response._status,query,body);
			}

			if (status != seastar::http::reply::status_type::ok) {
//				throw std::runtime_error("ClickHouse HTTP request failed with status " +
//						std::to_string(static_cast<int>(status)));
				fmt::print("{} {} {}\n",__FILE__,__LINE__,status);
			}
			return seastar::make_ready_future<>();
		}).handle_exception([this, host, body, query, debug](auto ep) {
			if (debug){
				fmt::print("{} {} {} {} query {} body {}\n",__FILE__,__LINE__,ep,host,query,body);
			}
			return seastar::make_ready_future<>();
		});
	}

	seastar::future<> close() override {
		if (_connection) {
			return _connection->close();
		}
		return seastar::make_ready_future<>();
	}

	// Groups the decoded packets in `batch` per event type and bulk inserts
	// each group into its typed table via the HTTP client. Generated below.
	seastar::future<> Flush(std::vector<seastar::temporary_buffer<char>>&& batch) override;

	// Returns the DDL statements creating the typed per-event tables and the
	// materialized views projecting `payload` into typed columns. Generated below.
	std::vector<seastar::sstring> getMigrations();

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

	seastar::sstring _ip;
	seastar::sstring _port;
	seastar::lw_shared_ptr<seastar::http::experimental::client> _client;
	seastar::lw_shared_ptr<seastar::http::experimental::connection> _connection;
	seastar::sstring _dbname;
	seastar::sstring _username;
	seastar::sstring _password;
	seastar::sstring _table;
};

// Flush() and getMigrations() are generated below by SeastarServerCodeGen and
// defined out-of-line so they can be regenerated whenever the event schema
// changes without touching the boilerplate above.

