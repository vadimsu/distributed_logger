package clickhouse

import (
        "context"
//      "crypto/tls"
        "fmt"
        "strconv"
        "github.com/google/uuid"
        "github.com/ClickHouse/clickhouse-go/v2"
        "github.com/ClickHouse/clickhouse-go/v2/lib/driver"
        "time"
)

type ClickhouseStorage struct{
        client driver.Conn
}

func connect(username string, password string, dbname string, shard int) (driver.Conn, error) {
        ctx  := context.Background()
        conn, err := clickhouse.Open(&clickhouse.Options{
                Addr: []string{"127.0.0.1:9000"},
                Auth: clickhouse.Auth{
                                Database: dbname,
                                Username: username,
                                Password: password,
                        },
                })

        if err != nil {
                return nil, err
        }

        if err := conn.Ping(ctx); err != nil {
                if exception, ok := err.(*clickhouse.Exception); ok {
                        fmt.Printf("Exception [%d] %s \n%s\n", exception.Code, exception.Message, exception.StackTrace)
                }
                return nil, err
        }
        err = conn.Exec(ctx, `
                CREATE TABLE IF NOT EXISTS events`+strconv.Itoa(shard)+` (
                        zoneuid UUID
                        , serial Int32
                        , host String
                        , version UInt16
                        , event UInt16
                        , timestamp UInt64
                        , eventtime DateTime
                        , args String
                ) Engine = MergeTree PRIMARY KEY (eventtime) ORDER BY (eventtime,timestamp) TTL eventtime + INTERVAL 8 HOUR
        `)
        if err == nil {
                err = conn.Exec(ctx, `ALTER TABLE events`+strconv.Itoa(shard)+` ADD INDEX zoneuid_serial (zoneuid,serial) TYPE set(8192)`)
                if err != nil {
                        fmt.Println(err)
                        return nil, err
                }
        }
        if err != nil{
                return nil, err
        }
        err = conn.Exec(context.Background(), "SET async_insert=1")
        if err != nil {
                fmt.Println(err)
        }
        fmt.Println("clickhouse conn ",conn)
        return conn, nil
}

func Init(args ...any) (*ClickhouseStorage, error){
        username := fmt.Sprintf("%v", args[0])
        password:= fmt.Sprintf("%v", args[1])
        dbname := fmt.Sprintf("%v", args[2])
        shard,_ := args[3].(int)
        conn, err := connect(username, password, dbname, shard)
        if err != nil{
                return &ClickhouseStorage{}, err
        }
        return &ClickhouseStorage{ client: conn }, nil
}

func (s *ClickhouseStorage) Store(key string, serial int, key2 string, key3 string, shard int, storedData map[string]string) (bool, error){
        version, err := strconv.Atoi(storedData["version"])
        timestamp , err :=  strconv.Atoi(storedData["timestamp"])
        event, err :=  strconv.Atoi(key3)
        zoneuid, err := uuid.Parse(key)
        args := ""
        for i := 1;  ;i++ {
                key := "arg#"+strconv.Itoa(i)
                val, ok := storedData[key]
                if !ok {
                        break
                }
                args = args + key + ":" + val
        }
        ctx := context.Background()
        err = s.client.AsyncInsert(ctx, `INSERT INTO events`+strconv.Itoa(shard)+` VALUES (
                        ?, ?, ?, ?, ?, ?, now(), ?
                )`, false, zoneuid, serial, key2, version, event, timestamp, args)
        if err != nil{
                fmt.Println(err)
                return false, err
        }
//      err = s.client.Exec(ctx, `ALTER TABLE ids`+strconv.Itoa(shard)+` UPDATE zoneRecordId=`+strconv.Itoa(int(id + 1))+ ` WHERE 1 `)
//      if err != nil{
//              fmt.Println("UPDATE ",err)
//              return false, err
//      }

        return true, nil
}

func (s *ClickhouseStorage) Query(zoneuid string)([]string,error){
        var zoneUidHost []string
        ctx := context.Background()
        statement := `SELECT * FROM events WHERE zoneuid=toUUID('`+zoneuid+`')`
//      fmt.Println("executing statement ",statement)
        rows, err := s.client.Query(ctx, statement)
        if err != nil{
                fmt.Println(err)
                return nil, err
        }
//      fmt.Println(rows)
//      fmt.Println(rows.Err())
        for rows.Next(){
                var zoneuid uuid.UUID
                var host string
                var shard uint16
                var version uint16
                var event uint16
                var timestamp uint64
                var eventtime time.Time
                var args string
                if err = rows.Scan(&zoneuid, &host, &shard, &version, &event, &timestamp, &eventtime, &args); err != nil {
                        fmt.Println("Error on scan: ",err)
                }else{
                        zoneUidHost = append(zoneUidHost, zoneuid.String())
                        zoneUidHost = append(zoneUidHost, host)
                        zoneUidHost = append(zoneUidHost, strconv.Itoa(int(shard)))
                        zoneUidHost = append(zoneUidHost, strconv.Itoa(int(version)))
                        zoneUidHost = append(zoneUidHost, strconv.Itoa(int(event)))
                        zoneUidHost = append(zoneUidHost, strconv.Itoa(int(timestamp)))
                        zoneUidHost = append(zoneUidHost, eventtime.String())
                        zoneUidHost = append(zoneUidHost, args)
                        break
                }
        }
        rows.Close()
        return zoneUidHost, nil
}

func (s *ClickhouseStorage) QueryLeastRecent()([]string,error){
        var zoneUids []string
        ctx := context.Background()
        statement := `SELECT * FROM events WHERE event=12 LIMIT 10`
//      fmt.Println("executing statement ",statement)
        rows, err := s.client.Query(ctx, statement)
        if err != nil{
                fmt.Println(err)
                return nil, err
        }
//      fmt.Println(rows)
//      fmt.Println(rows.Err())
        for rows.Next(){
                var zoneuid uuid.UUID
                var host string
                var shard uint16
                var version uint16
                var event uint16
                var timestamp uint64
                var eventtime time.Time
                var args string
                if err = rows.Scan(&zoneuid, &host, &shard, &version, &event, &timestamp, &eventtime, &args); err != nil {
                        fmt.Println("Error on scan: ",err)
                }else{
                        zoneUids = append(zoneUids, zoneuid.String())
                }
        }
        rows.Close()
        return zoneUids, nil
}

func (s *ClickhouseStorage) Delete(key string)(int64, error){
        statement := `DELETE FROM events WHERE zoneuid=toUUID('`+key+`') AND event != 12`
        return -1, s.client.Exec(context.Background(), statement)
}
