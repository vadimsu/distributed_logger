package mongo

import (
        "context"
	"fmt"
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
	uri := ""
	if username != "" && password != ""{
		uri = "mongodb://"+username+":"+password+"@"+host+":"+port+"/"+dbname+"?authSource=admin"
		
	}else{
		uri = "mongodb://"+host+":"+port+"/"+dbname
	}
	fmt.Println("URI ",uri)
        client, err := mongo.Connect(options.Client().
                ApplyURI(uri))
        if err != nil {
                panic(err)
        }
	fmt.Println("Connected")
        db := client.Database(dbname)
        var coll *mongo.Collection
        coll = db.Collection("events")
        return &MongoStorage { collection: coll }, nil
}
