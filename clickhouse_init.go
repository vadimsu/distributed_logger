package clickhouse

import (
	"context"
	"fmt"
	"github.com/ClickHouse/clickhouse-go/v2"
)

type ClickHouseStorage struct{
	conn clickhouse.Conn
	table string
}

func Init(args ...any) (*ClickHouseStorage, error){
	host, _ := args[1].(string)
	port, _ := args[2].(string)
	dbname, _ := args[3].(string)
	username, _ := args[4].(string)
	password, _ := args[5].(string)

	addr := host+":"+port
	conn, err := clickhouse.Open(&clickhouse.Options{
		Addr: []string{addr},
		Auth: clickhouse.Auth{Username: username, Password: password},
		Database: dbname,
	})
	if err != nil {
		return nil, err
	}
	table := "events"
	return &ClickHouseStorage{conn: conn, table: table}, nil
}
