**MongoDB**

Distributed Logger is designed to support **multiple storage backends** while keeping the **client API identical**. The ingestion server receives events from clients and routes them to the configured storage backend.

The system currently supports:

MongoDB

ClickHouse

Both backends are optimized for **high-throughput append-only logging workloads**, but their internal storage models differ significantly.

**Ingestion Pipeline Overview**
```mermaid
flowchart TD

A[Client Application] --LogEvent(...)--> B[Generated Encoder]
B --> C[TCP/TLS Network Transport]
C --> D[Listener Server]
D --> E[Event Decoder]
E --> F[Storage worker queue]
F --> G[Storage backend]
```
The server decodes events and passes them to storage workers responsible for batching and writing to the configured backend.

**MongoDB Storage Model**
For MongoDB, events are stored directly as documents.

Example event:
`{
  "event": "event0",
  "Shard": 3,
  "Host": "node-1",
  "Parameter1": "value",
  "Parameter2": 42,
  "Timestamp": 1710000000
}`

Characteristics:

* flexible schema

* no pre-defined structure required

* efficient for moderate ingestion rates

* easy ad-hoc querying

MongoDB is therefore a good default for:

* operational logging

* debugging

* moderate-scale telemetry

**ClickHouse Storage Model**

ClickHouse is optimized for **analytical workloads and very large datasets**. However it requires a predefined schema.

To maintain flexibility while supporting heterogeneous event structures, Distributed Logger uses a **two-stage ingestion architecture**.

**Raw Event Table**

All events are first written to a single append-only table.

Example schema:
`CREATE TABLE events_raw
(
    event UInt64,
    payload String
)
ENGINE = MergeTree
ORDER BY tuple()`

Where:

* `event` identifies the event type

* `payload` contains the full event encoded as JSON

Example record:

`event:   0
payload: {"Shard":3,"Host":"node-1","Parameter1":"value","Parameter2":42}`

Advantages:

* supports arbitrary event schemas

* very simple ingestion path

* avoids schema migrations

**Materialized Views**

Per-event tables are populated automatically using materialized views.

Example:
`CREATE MATERIALIZED VIEW mv_events_event0
TO events_event0
AS
SELECT
    JSONExtractUInt(payload, 'Shard') AS Shard,
    JSONExtractString(payload, 'Host') AS Host
FROM events_raw
WHERE event = 0`

This creates a typed table optimized for analytics.
`events_event0`

Example schema:

`CREATE TABLE events_event0
(
    Shard UInt32,
    Host String
)
ENGINE = MergeTree
ORDER BY Shard`

This architecture allows:

* flexible ingestion

* efficient analytical queries

* automatic schema projection

**Performance Considerations**

ClickHouse performance strongly depends on **batch size**.

During testing the following observations were made:

|Configuration| Relative Throughput |
|-------------|---------------------|
|MongoDB      |	baseline            |
|ClickHouse (raw table only)|	~85–100% of Mongo|
|ClickHouse (raw + materialized views)|	~60% of Mongo|

This reduction occurs because materialized views perform additional work during each insert:

`insert
   ↓
write raw table
   ↓
execute materialized view
   ↓
write typed event table`

However this overhead enables efficient analytical queries.

**Recommended ClickHouse Configuration**

For high ingestion throughput:

batch inserts between **10k and 100k rows**

avoid extremely small insert batches

use append-only tables

Example configuration used during testing:

`batch_size = 10000
flush_interval = 10ms`

**When to Use Each Backend**

MongoDB is recommended for:

* operational logging

* moderate ingestion rates

* flexible event structures

* simple deployment

ClickHouse is recommended for:

* large telemetry streams

* analytical workloads

* long-term event storage

* large datasets with aggregation queries

**Future Optimization**

A future storage mode may allow **direct typed insertion into ClickHouse tables**, bypassing JSON extraction and materialized views. This would increase ingestion performance further at the cost of reduced schema flexibility.

**ClickHouse Wire Protocol: HTTP vs Native**

The Seastar-based server can talk to ClickHouse over either of two wire protocols, selected per-deployment via the `"Protocol"` field in the ClickHouse storage config JSON (`seastar_based_server/storage_config_clickhouse.json` / `storage_config_clickhouse_native.json`):

* `"Protocol": "http"` (default) - `ClickHouseStorage` (`tools/codegen/templates/clickhouse_init.hh`), talks to ClickHouse's HTTP interface (default port `8123`) using `seastar::http::experimental::client`. Row batches are sent as text `INSERT ... VALUES` bodies.
* `"Protocol": "native"` - `ClickHouseNativeStorage` (`tools/codegen/templates/clickhouse_native_init.hh`), talks to ClickHouse's binary TCP protocol (default port `9000`) via a small hand-written client in `seastar_based_server/native_protocol.hh` (see the [Native Protocol](https://clickhouse.com/docs/reference/interfaces/specs/NativeProtocol) and [Native Format](https://clickhouse.com/docs/reference/interfaces/specs/NativeFormat) specs). Row batches are encoded as native columnar `Block`s and streamed through the INSERT phase (`Query` → schema block → `Data` block(s) → empty end-of-input marker), avoiding text (de)serialization on the hot path.

Both backends implement the same `Storage` interface (`Init`/`globalInit`/`close`/`Flush`), create the same raw `events` table and per-event materialized views, and are generated by the same `SeastarServerCodeGen` (`tools/code_gen.py`) from the event header - only the transport differs. `Storage::Init`/`Storage::globalInit` (`seastar_based_server/storage.cc`) pick the implementation based on the configured `Protocol`, defaulting to `"http"` when the field is absent, so existing configs keep working unchanged.

The native client intentionally advertises an old protocol revision (below `DBMS_MIN_REVISION_WITH_CLIENT_INFO`) to stay in the simplest wire shape (no `ClientInfo`, no per-column custom-serialization byte, minimal `ServerHello`); it supports the fixed-width integer, `Bool`, and `String` column types used by the generated schema and raises on unsupported composite types (`Array`/`Tuple`/`Nullable`/`LowCardinality`/...).

**Design Goal**

The storage layer is intentionally designed so that:

* the **client API never changes**

* event schemas are defined **once in the header file**

* storage implementations can evolve independently

This allows applications to migrate from MongoDB to ClickHouse (or other backends) without modifying client code.
