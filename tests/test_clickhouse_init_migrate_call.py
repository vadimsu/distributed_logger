def test_clickhouse_init_calls_migrate():
    # Ensure Init was updated to call GetMigrations/Migrate
    contents = open('clickhouse_init.go').read()
    assert 'GetMigrations' in contents
    assert 'Migrate(migrations)' in contents
