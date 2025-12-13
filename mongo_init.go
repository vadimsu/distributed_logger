package mongo

import (
        "context"
	"distributedlogger.com/storage"
        "fmt"
        "strconv"
//        "go.mongodb.org/mongo-driver/v2/bson"
        "go.mongodb.org/mongo-driver/v2/mongo"
        "go.mongodb.org/mongo-driver/v2/mongo/options"
)

type MongoStorage struct{
        collection *mongo.Collection
}

func Init(args ...any) (*MongoStorage, error){
        fmt.Printf("arg[0]: %v",args[0])
        shard,_ := args[0].(int)
//        dataRetention, _ := args[1].(int)
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
//        ib := &options.IndexOptionsBuilder{}
  //      partialFilter := bson.D{}
    //    ib.SetPartialFilterExpression(partialFilter)
//        ib.SetExpireAfterSeconds(3600*int32(dataRetention))
//        indexModel := mongo.IndexModel{
//            Keys: bson.D{{"eventtime", 1}},
//            Options: ib,
//        }
//        _, err = coll.Indexes().CreateOne(context.TODO(), indexModel)
//        if err != nil {
//            fmt.Println(err)
//        }
        return &MongoStorage { collection: coll }, nil
}
