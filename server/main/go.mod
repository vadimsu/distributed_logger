module main

go 1.22.9

replace distributedlogger.com/listener => ../listener

replace distributedlogger.com/decoder => ../decoder

replace distributedlogger.com/event_decoder => ../../generated/server/event_decoder

require (
	//        distributedlogger.com/clickhouse v0.0.0-00010101000000-000000000000
	distributedlogger.com/listener v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/pg v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/redis v0.0.0-00010101000000-000000000000
	distributedlogger.com/storage v0.0.0-00010101000000-000000000000
)

require (
	distributedlogger.com/clickhouse v0.0.0-00010101000000-000000000000
	distributedlogger.com/mongo v0.0.0-00010101000000-000000000000
)

require (
	distributedlogger.com/config v0.0.0-00010101000000-000000000000 // indirect
	distributedlogger.com/decoder v0.0.0-00010101000000-000000000000 // indirect
	distributedlogger.com/event_decoder v0.0.0-00010101000000-000000000000 // indirect
	github.com/ClickHouse/ch-go v0.52.1 // indirect
	github.com/ClickHouse/clickhouse-go/v2 v2.8.0 // indirect
	github.com/andybalholm/brotli v1.0.5 // indirect
	github.com/go-faster/city v1.0.1 // indirect
	github.com/go-faster/errors v0.6.1 // indirect
	github.com/golang/snappy v1.0.0 // indirect
	github.com/google/uuid v1.3.0 // indirect
	github.com/klauspost/compress v1.16.7 // indirect
	github.com/paulmach/orb v0.9.0 // indirect
	github.com/pierrec/lz4/v4 v4.1.17 // indirect
	github.com/pkg/errors v0.9.1 // indirect
	github.com/segmentio/asm v1.2.0 // indirect
	github.com/shopspring/decimal v1.3.1 // indirect
	github.com/xdg-go/pbkdf2 v1.0.0 // indirect
	github.com/xdg-go/scram v1.1.2 // indirect
	github.com/xdg-go/stringprep v1.0.4 // indirect
	github.com/youmark/pkcs8 v0.0.0-20240726163527-a2c0da244d78 // indirect
	go.mongodb.org/mongo-driver/v2 v2.4.1 // indirect
	go.opentelemetry.io/otel v1.13.0 // indirect
	go.opentelemetry.io/otel/trace v1.13.0 // indirect
	golang.org/x/crypto v0.33.0 // indirect
	golang.org/x/sync v0.11.0 // indirect
	golang.org/x/sys v0.30.0 // indirect
	golang.org/x/text v0.22.0 // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)

replace distributedlogger.com/storage => ../../generated/server/storage

replace distributedlogger.com/config => ../config

replace distributedlogger.com/redis => ../../generated/server/storage/redis

replace distributedlogger.com/pg => ../../generated/server/storage/pg

replace distributedlogger.com/mongo => ../../generated/server/storage/mongo

replace distributedlogger.com/clickhouse => ../../generated/server/storage/clickhouse
