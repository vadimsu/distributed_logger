module distributedlogger.com/main

go 1.22.9

replace distributedlogger.com/event_collector => ../event_collector

require (
	//        distributedlogger.com/clickhouse v0.0.0-00010101000000-000000000000
	distributedlogger.com/event_collector v0.0.0-00010101000000-000000000000
	distributedlogger.com/mongo v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/pg v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/redis v0.0.0-00010101000000-000000000000
	distributedlogger.com/storage v0.0.0-00010101000000-000000000000
)

require distributedlogger.com/clickhouse v0.0.0-00010101000000-000000000000

require (
	github.com/ClickHouse/ch-go v0.65.1 // indirect
	github.com/ClickHouse/clickhouse-go/v2 v2.32.2 // indirect
	github.com/andybalholm/brotli v1.1.1 // indirect
	github.com/go-faster/city v1.0.1 // indirect
	github.com/go-faster/errors v0.7.1 // indirect
	github.com/golang/snappy v0.0.4 // indirect
	github.com/google/uuid v1.6.0 // indirect
	github.com/klauspost/compress v1.17.11 // indirect
	github.com/niemeyer/pretty v0.0.0-20200227124842-a10e7caefd8e // indirect
	github.com/paulmach/orb v0.11.1 // indirect
	github.com/pierrec/lz4/v4 v4.1.22 // indirect
	github.com/pkg/errors v0.9.1 // indirect
	github.com/segmentio/asm v1.2.0 // indirect
	github.com/shopspring/decimal v1.4.0 // indirect
	github.com/xdg-go/pbkdf2 v1.0.0 // indirect
	github.com/xdg-go/scram v1.1.2 // indirect
	github.com/xdg-go/stringprep v1.0.4 // indirect
	github.com/youmark/pkcs8 v0.0.0-20240726163527-a2c0da244d78 // indirect
	go.mongodb.org/mongo-driver/v2 v2.0.0 // indirect
	go.opentelemetry.io/otel v1.34.0 // indirect
	go.opentelemetry.io/otel/trace v1.34.0 // indirect
	golang.org/x/crypto v0.33.0 // indirect
	golang.org/x/sync v0.11.0 // indirect
	golang.org/x/sys v0.30.0 // indirect
	golang.org/x/text v0.22.0 // indirect
	gopkg.in/check.v1 v1.0.0-20200227125254-8fa46927fb4f // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)

replace distributedlogger.com/storage => ../storage

replace distributedlogger.com/redis => ../storage/redis

replace distributedlogger.com/pg => ../storage/pg

replace distributedlogger.com/mongo => ../storage/mongo

replace distributedlogger.com/clickhouse => ../storage/clickhouse
