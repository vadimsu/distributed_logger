module main

go 1.22.9

replace distributedlogger.com/listener => ../listener

replace distributedlogger.com/decoder => ../decoder

require (
	//        distributedlogger.com/clickhouse v0.0.0-00010101000000-000000000000
	distributedlogger.com/listener v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/pg v0.0.0-00010101000000-000000000000
	//        distributedlogger.com/redis v0.0.0-00010101000000-000000000000
	distributedlogger.com/storage v0.0.0-00010101000000-000000000000
)

require distributedlogger.com/mongo v0.0.0-00010101000000-000000000000

require (
	distributedlogger.com/config v0.0.0-00010101000000-000000000000 // indirect
	distributedlogger.com/decoder v0.0.0-00010101000000-000000000000 // indirect
	github.com/golang/snappy v1.0.0 // indirect
	github.com/klauspost/compress v1.16.7 // indirect
	github.com/xdg-go/pbkdf2 v1.0.0 // indirect
	github.com/xdg-go/scram v1.1.2 // indirect
	github.com/xdg-go/stringprep v1.0.4 // indirect
	github.com/youmark/pkcs8 v0.0.0-20240726163527-a2c0da244d78 // indirect
	go.mongodb.org/mongo-driver/v2 v2.4.1 // indirect
	golang.org/x/crypto v0.33.0 // indirect
	golang.org/x/sync v0.11.0 // indirect
	golang.org/x/text v0.22.0 // indirect
)

replace distributedlogger.com/storage => ../storage

replace distributedlogger.com/config => ../config

replace distributedlogger.com/redis => ../storage/redis

replace distributedlogger.com/pg => ../storage/pg

replace distributedlogger.com/mongo => ../storage/mongo

replace distributedlogger.com/clickhouse => ../storage/clickhouse
