from code_gen import SeastarServerCodeGen


def sample_funcs():
    return [
        {
            'name': 'Store',
            'return': 'error',
            'params': [('event0', 'uint64_t'), ('shard', 'uint64_t'), ('host', 'string')],
        }
    ]


def test_seastar_codegen_generates_native_flush_and_migrations():
    g = SeastarServerCodeGen(sample_funcs())
    g.generate_code()

    native_code = g.get_clickhouse_native_definitions_code()

    # Flush() should be generated against ClickHouseNativeStorage and dispatch
    # decoded batches into insertRows() (native Block-based bulk insert),
    # rather than building a text VALUES clause like the HTTP backend does.
    assert 'ClickHouseNativeStorage::Flush' in native_code
    assert 'insertRows(std::move(kv.second))' in native_code
    assert 'std::map<int, std::vector<std::pair<uint64_t, seastar::sstring>>>' in native_code
    assert 'case Events::event0' in native_code
    assert 'INSERT INTO' not in native_code  # no text VALUES clause

    # getMigrations() DDL should be identical in spirit to the HTTP backend:
    # typed per-event table + materialized view projecting the JSON payload.
    assert 'ClickHouseNativeStorage::getMigrations' in native_code
    assert 'CREATE TABLE IF NOT EXISTS' in native_code
    assert 'CREATE MATERIALIZED VIEW IF NOT EXISTS' in native_code
    assert 'JSONExtract' in native_code


def test_seastar_codegen_native_and_http_definitions_share_migrations_shape():
    g = SeastarServerCodeGen(sample_funcs())
    g.generate_code()

    http_code = g.get_clickhouse_definitions_code()
    native_code = g.get_clickhouse_native_definitions_code()

    # Both backends must target the same table/column names generated from the
    # event schema, only the transport differs.
    assert 'events_event0' not in http_code and 'events_event0' not in native_code
    assert '_event0' in http_code and '_event0' in native_code
    assert 'ClickHouseStorage::Flush' in http_code
    assert 'ClickHouseNativeStorage::Flush' in native_code


def test_seastar_codegen_empty_funcs_produces_empty_native_bodies():
    g = SeastarServerCodeGen([])
    g.generate_code()

    native_code = g.get_clickhouse_native_definitions_code()
    assert 'ClickHouseNativeStorage::Flush' in native_code
    assert 'ClickHouseNativeStorage::getMigrations' in native_code
    assert 'case Events::' not in native_code
