
#pragma once

#include <seastar/core/reactor.hh>
#include <seastar/net/api.hh>
#include <seastar/core/iostream.hh>
#include <seastar/net/tls.hh>
#include <seastar/core/temporary_buffer.hh>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/validate.hpp>
#include <iostream>
#include <memory>
#include <vector>


namespace DistributedLogger {

// MongoDB Wire Protocol Header (16 bytes)
struct MongoHeader {
    int32_t message_length; 
    int32_t request_id;     
    int32_t response_to;    
    int32_t op_code;        // 2013 for OP_MSG
};

class SeastarMongoClient : public std::enable_shared_from_this<SeastarMongoClient> {
private:
    seastar::connected_socket _socket;
    seastar::input_stream<char> _in;
    seastar::output_stream<char> _out;
    int32_t _request_id = 1;

public:
    SeastarMongoClient(seastar::connected_socket&& socket)
        : _socket(std::move(socket)), _in(_socket.input()), _out(_socket.output()) {}

    // 1. Connection Handshake Logic
    seastar::future<> connect_and_handshake() {
        using bsoncxx::builder::basic::kvp;

        // Build the standard MongoDB 'hello' payload
        auto cmd = bsoncxx::builder::basic::document{};
        cmd.append(kvp("hello", 1));
        cmd.append(kvp("$db", "admin"));
        auto cmd_bson = cmd.extract();

        int32_t flags = 0; // 0 = Expecting a network response packet
        uint8_t payload_type = 0;
        int32_t current_id = _request_id++;

        int32_t total_size = sizeof(MongoHeader) + sizeof(flags) + sizeof(payload_type) + cmd_bson.view().length();

        // Serialize Handshake Outbound Packet
        std::vector<char> buffer(total_size);
        char* ptr = buffer.data();

        MongoHeader header{total_size, current_id, 0, 2013};
        std::memcpy(ptr, &header, sizeof(MongoHeader)); ptr += sizeof(MongoHeader);
        std::memcpy(ptr, &flags, sizeof(flags)); ptr += sizeof(flags);
        std::memcpy(ptr, &payload_type, sizeof(payload_type)); ptr += sizeof(payload_type);
        std::memcpy(ptr, cmd_bson.view().data(), cmd_bson.view().length());

        // Send handshake packet asynchronously
        return _out.write(buffer.data(), buffer.size())
            .then([this] { return _out.flush(); })
            .then([this] {
                // Read and consume response header (16 bytes)
                return _in.read_exactly(sizeof(MongoHeader));
            })
            .then([this](seastar::temporary_buffer<char> header_buf) {
                if (header_buf.size() < sizeof(MongoHeader)) {
                    throw std::runtime_error("Malformed MongoDB connection response header");
                }
                MongoHeader resp_header;
                std::memcpy(&resp_header, header_buf.get(), sizeof(MongoHeader));

                // Calculate remaining payload bytes to read
                int32_t body_size = resp_header.message_length - sizeof(MongoHeader);
                return _in.read_exactly(body_size);
            })
            .then([](seastar::temporary_buffer<char> body_buf) {
                // Skip incoming 4-byte flags + 1-byte payload type to reach raw BSON
                const char* bson_ptr = body_buf.get() + 5; 
                size_t bson_len = body_buf.size() - 5;

                // Validate payload response
                auto view = bsoncxx::document::view(reinterpret_cast<const uint8_t*>(bson_ptr), bson_len);
                if (!view["ok"] || view["ok"].get_double() != 1.0) {
                    throw std::runtime_error("MongoDB handshake rejected by server engine");
                }
                std::cout << "[Seastar-Mongo] Handshake complete. Node ready.\n";
            });
    }

    // 2. High-Performance True Non-Blocking Bulk Insert (w=0)
    seastar::future<> bulk_insert_unacknowledged(
        const std::string& db_name,
        const std::string& collection_name,
        const std::vector<bsoncxx::document::view>& docs) 
    {
        using bsoncxx::builder::basic::kvp;
        using bsoncxx::builder::basic::sub_array;

        // Build main command framework
        auto cmd_builder = bsoncxx::builder::basic::document{};
        cmd_builder.append(kvp("insert", collection_name));
        cmd_builder.append(kvp("ordered", false));
        cmd_builder.append(kvp("$db", db_name));
        
        // Inject batch items seamlessly
        cmd_builder.append(kvp("documents", [&](sub_array sub) {
            for (const auto& doc : docs) {
                sub.append(doc);
            }
        }));

        auto cmd_bson = cmd_builder.extract();
        
        // Bit 1 = 'moreToExecute'. Directs MongoDB to execute completely without replying
        int32_t flags = 1 << 1; 
        uint8_t payload_type = 0; 
        int32_t current_id = _request_id++;

        int32_t total_size = sizeof(MongoHeader) + sizeof(flags) + sizeof(payload_type) + cmd_bson.view().length();

        // Seastar Optimization: Allocate a zero-copy temporary heap buffer tracking scope
        seastar::temporary_buffer<char> write_buf(total_size);
        char* ptr = write_buf.get_writeable();

        MongoHeader header{total_size, current_id, 0, 2013};
        std::memcpy(ptr, &header, sizeof(MongoHeader)); ptr += sizeof(MongoHeader);
        std::memcpy(ptr, &flags, sizeof(flags)); ptr += sizeof(flags);
        std::memcpy(ptr, &payload_type, sizeof(payload_type)); ptr += sizeof(payload_type);
        std::memcpy(ptr, cmd_bson.view().data(), cmd_bson.view().length());

        // Stream raw byte batch buffer straight over the reactor core.
        // Returns immediately to the execution engine poll loops.
        return _out.write(std::move(write_buf)).then([this] {
            return _out.flush();
        });
    }

    seastar::future<> close() {
        return _out.close().then([this] { return _in.close(); });
    }
};

// Slices a raw Seastar network buffer into zero-copy MongoDB views.
// NOTE: The resulting views are only valid as long as 'buffer' remains alive!
std::vector<bsoncxx::document::view> extract_views_from_buffer(
    const seastar::temporary_buffer<char>& buffer) 
{
    std::vector<bsoncxx::document::view> views;
    
    const char* data_ptr = buffer.get();
    size_t total_bytes = buffer.size();
    size_t offset = 0;

    while (offset < total_bytes) {
        // 1. Boundary Check: Ensure we have at least 4 bytes left to read the length
        if (total_bytes - offset < 4) {
            throw std::runtime_error("Malformed stream: Remaining fragment too small for BSON header");
        }

        // 2. Read BSON Length: The first 4 bytes of any BSON document represent its total size (Little-Endian)
        int32_t bson_len;
        std::memcpy(&bson_len, data_ptr + offset, sizeof(int32_t));

        // 3. Validation: Sanity check the parsed size boundaries
        if (bson_len < 5) { // Minimum valid BSON document size is 5 bytes (4 bytes size + 1 byte null terminator)
            throw std::runtime_error("Malformed stream: Invalid BSON length header detected");
        }
        if (offset + bson_len > total_bytes) {
            throw std::runtime_error("Malformed stream: BSON document exceeds temporary_buffer boundaries");
        }

        // 4. Construct the Zero-Copy View:
        // Cast the raw byte sequence directly into the viewer
        bsoncxx::document::view view(
            reinterpret_cast<const uint8_t*>(data_ptr + offset), 
            static_cast<size_t>(bson_len)
        );

        views.push_back(view);

        // 5. Advance offset to the start of the next contiguous document
        offset += bson_len;
    }

    return views;
}


}
