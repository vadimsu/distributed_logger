package mongo

import (
        "context"
	"distributedlogger.com/storage"
	"distributedlogger.com/event_decoder"
	"distributedlogger.com/decoder"
//        "go.mongodb.org/mongo-driver/v2/bson"
        "go.mongodb.org/mongo-driver/v2/mongo"
        "go.mongodb.org/mongo-driver/v2/mongo/options"
)

type MongoStorage struct{
        collection *mongo.Collection
}

func Init(args ...any) (*MongoStorage, error){
//        dataRetention, _ := args[0].(int)
        host, _ := args[1].(string)
        port, _ := args[2].(string)
        dbname, _ := args[3].(string)
        username, _ := args[4].(string)
        password, _ := args[5].(string)
//      uri := "mongodb://127.0.0.1:27017/?directConnection=true&serverSelectionTimeoutMS=2000&appName=mongosh+2.3.9"
        uri := "mongodb://"+username+":"+password+"@"+host+":"+port+"/?"+dbname+"?authSource=admin"

        client, err := mongo.Connect(options.Client().
                ApplyURI(uri))
        if err != nil {
                panic(err)
        }
        db := client.Database(dbname)
        var coll *mongo.Collection
        coll = db.Collection("events")
        return &MongoStorage { collection: coll }, nil
}
