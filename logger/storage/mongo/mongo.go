package mongo

import (
        "context"
        "fmt"
        "strconv"
        "go.mongodb.org/mongo-driver/v2/bson"
        "go.mongodb.org/mongo-driver/v2/mongo"
        "go.mongodb.org/mongo-driver/v2/mongo/options"
        "time"
)

type MongoStorage struct{
        collection *mongo.Collection
}

type Zone struct {
        Zoneuid         string `bson:"zoneuid"`
        Serial          int `bson:"serial"`
        Host            string `bson:"host"`
        Timestamp       int `bson:"timestamp"`
        EventTime       time.Time  `bson:"eventtime"`
        Version         int     `bson:"version"`
        Event           int     `bson:"event"`
        Args            string  `bson:"args"`
}

func Init(args ...any) (*MongoStorage, error){
        fmt.Printf("arg[0]: %v",args[0])
        shard,_ := args[0].(int)
        dataRetention, _ := args[1].(int)
        host, _ := args[2].(string)
        port, _ := args[3].(string)
        dbname, _ := args[4].(string)
        username, _ := args[5].(string)
        password, _ := args[6].(string)
//      uri := "mongodb://127.0.0.1:27017/?directConnection=true&serverSelectionTimeoutMS=2000&appName=mongosh+2.3.9"
        uri := "mongodb://"+username+":"+password+"@"+host+":"+port+"/?"+dbname+"?authSource=admin"

        client, err := mongo.Connect(options.Client().
                ApplyURI(uri))
        if err != nil {
                panic(err)
        }
        db := client.Database(dbname)
        var coll *mongo.Collection
        coll = db.Collection("events"+strconv.Itoa(shard))
        ib := &options.IndexOptionsBuilder{}
        partialFilter := bson.D{}
        ib.SetPartialFilterExpression(partialFilter)
        ib.SetExpireAfterSeconds(3600*int32(dataRetention))
        indexModel := mongo.IndexModel{
            Keys: bson.D{{"eventtime", 1}},
            Options: ib,
        }
        _, err = coll.Indexes().CreateOne(context.TODO(), indexModel)
        if err != nil {
            fmt.Println(err)
        }
        ib = &options.IndexOptionsBuilder{}
        indexModel = mongo.IndexModel{
            Keys: bson.D{{"zoneuid", 1}, {"serial", 1}},
            Options: ib,
        }
        _, err = coll.Indexes().CreateOne(context.TODO(), indexModel)
        if err != nil {
            fmt.Println(err)
        }
        return &MongoStorage { collection: coll }, nil
}

func (s *MongoStorage) Store(key string, serial int, key2 string, key3 string, shard int, storedData map[string]string) (bool, error){
//      fmt.Println("Store for ",shard)
        version, _ := strconv.Atoi(storedData["version"])
        timestamp,_ :=  strconv.ParseUint(storedData["timestamp"], 10, 64)
        eventTime := time.Now()
        event, _ :=  strconv.Atoi(key3)
        args := ""
        for i := 1;  ;i++ {
                key := "arg#"+strconv.Itoa(i)
                val, ok := storedData[key]
                if !ok {
                        break
                }
                args = args + key + ":" + val
        }
        zones := []interface{}{
                Zone{
                        Zoneuid:        key,
                        Serial:         serial,
                        Host:           key2,
                        Timestamp:      int(timestamp),
                        EventTime:      eventTime,
                        Version:        version,
                        Event:          event,
                        Args:           args,
                },
        }
        _, err := s.collection.InsertMany(context.TODO(), zones)
        if err != nil{
                fmt.Println(err)
                return false, err
        }
//      fmt.Println(res)
//      fmt.Println(zones)
        return true, err
}

func (s *MongoStorage) Query(key string)([]string,error){
        filter := bson.D{{"zoneuid", key}}
        cursor, err := s.collection.Find(context.TODO(), filter)
        if err != nil {
            return nil, err
        }
        var results []Zone
        if err = cursor.All(context.TODO(), &results); err != nil {
            return nil, err
        }
        for _, result := range results {
//              res, _ := bson.MarshalExtJSON(result, false, false)
                var strResults []string
                strResults = append(strResults, result.Zoneuid)
                strResults = append(strResults, result.Host)
//              strResults = append(strResults, result.Timestamp.String())
//              strResults = append(strResults, strconv.Itoa(int(result.EventTime)))
                strResults = append(strResults, strconv.Itoa(int(result.Version)))
                strResults = append(strResults, strconv.Itoa(int(result.Event)))
                strResults = append(strResults, result.Args)
                return strResults,nil
        }
        return nil,nil
}

func (s *MongoStorage) QueryLeastRecent()([]string,error){
        var strResults []string
        filter := bson.D{{"event", bson.D{{"$eq", 12}}}}
        opts := options.Find().SetLimit(10)
        cursor, err := s.collection.Find(context.TODO(), filter, opts)
        if err != nil {
            return nil, err
        }
        var results []Zone
        if err = cursor.All(context.TODO(), &results); err != nil {
            return nil, err
        }
        for _, result := range results {
//              res, _ := bson.MarshalExtJSON(result, false, false)
                strResults = append(strResults, result.Zoneuid)
        }
        return strResults,nil
}

func (s *MongoStorage) Delete(key string)(int64, error){
        filter := bson.D{
                {"$and",
                        bson.A{
                                bson.D{ {"zoneuid", bson.D{ {"$eq", key}} } },
                                bson.D{ {"key", bson.D{ { "$ne", 12 } } } },
                        },
                },
        }
        opts := options.DeleteMany().SetHint(bson.D{{"zoneuid", 1}})
        count, err := s.collection.DeleteMany(context.TODO(), filter, opts)
        return count.DeletedCount, err
}


