module listener

go 1.22.2

replace distributedlogger.com/decoder => ../decoder

replace distributedlogger.com/storage => ../storage

replace distributedlogger.com/config => ../config

require (
	distributedlogger.com/decoder v0.0.0-00010101000000-000000000000
	distributedlogger.com/storage v0.0.0-00010101000000-000000000000
)

require distributedlogger.com/config v0.0.0-00010101000000-000000000000
