def test_main_includes_clickhouse_init_call():
    # ensure main.go was updated to call clickhouse.Init when storage type is clickhouse
    with open('server/main/main.go') as f:
        contents = f.read()

    assert 'clickhouse.Init' in contents
    assert 'distributedlogger.com/clickhouse' in contents
