package main

import (
    "distributedlogger.com/event_collector"
    "distributedlogger.com/mongo"
    "distributedlogger.com/clickhouse"
    "fmt"
    "os"
    "encoding/json"
    "io/ioutil"
    "distributedlogger.com/storage"
    "strconv"
)

type StorageConfig struct{
        StorageType string `json:"StorageType"`
        Username string `json:"Username"`
        Password string `json:"Password"`
        Host    string `json:"Host"`
        Port    string `json:"Port"`
        Dbname  string `json:"Dbname"`
        DataRetentionPeriod string `json:"DataRetentionPeriod"`
}

type GeneralConfig struct{
        StorageConfigFileName string `json:"StorageConfigFileName"`
        EventCollectorFileName string `json:"EventCollectorFileName"`
        MinShard string `json:"MinShard"`
        MaxShard string `json:"MaxShard"`
}

func main(){
        jsonFile, err := os.Open(os.Args[1])
        if err != nil{
                fmt.Println(err)
                return
        }
        var generalConfig GeneralConfig
        var storageConfig StorageConfig
        byteValue, err := ioutil.ReadAll(jsonFile)
        if err != nil{
                fmt.Println(err)
                return
        }
        json.Unmarshal(byteValue,&generalConfig)
        jsonFile, err = os.Open(generalConfig.StorageConfigFileName)
        if err != nil{
                fmt.Println(err)
                return
        }
        byteValue, err = ioutil.ReadAll(jsonFile)
        if err != nil{
                fmt.Println(err)
                return
        }
        json.Unmarshal(byteValue, &storageConfig)
        dataRetention, err := strconv.Atoi(storageConfig.DataRetentionPeriod)
        if err != nil {
                fmt.Println(err)
                return
        }
        storageApi := make(map[int]storage.StorageAPI)
        minShard, err := strconv.Atoi(generalConfig.MinShard)
        if err != nil{
                fmt.Println(err)
                return
        }
        maxShard, err := strconv.Atoi(generalConfig.MaxShard)
        if err != nil{
                fmt.Println(err)
                return
        }
        fmt.Println(storageConfig)
        if storageConfig.StorageType == "mongo"{
                fmt.Println("Initalizing mongo storage")
                for i:= minShard;i <= maxShard;i++{
                        sApi, _ := mongo.Init(i, dataRetention, storageConfig.Host, storageConfig.Port, storageConfig.Dbname, storageConfig.Username, storageConfig.Password)
                        if err != nil {
                                fmt.Println(err)
                                return
                        }
                        storageApi[i] = sApi
                }
        }else if storageConfig.StorageType == "clickhouse"{
                fmt.Println("Initializing clickhouse storage")
                for i:= minShard;i <= maxShard;i++{
			sApi, err := clickhouse.Init(storageConfig.Username, storageConfig.Password, "default", i)
                        if err != nil{
                                fmt.Println(err)
                                return
                        }else{
                                storageApi[i] = sApi
                        }
                }
        }
        event_collector.LaunchListener(generalConfig.EventCollectorFileName, storageApi)
}
