package main

import (
    "distributedlogger.com/listener"
    "distributedlogger.com/mongo"
    "distributedlogger.com/clickhouse"
    "fmt"
    "os"
    "encoding/json"
    "io/ioutil"
    "distributedlogger.com/storage"
    "distributedlogger.com/ingester"
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
        var storageApi storage.StorageAPI
        fmt.Println(storageConfig)
        if storageConfig.StorageType == "mongo"{
                fmt.Println("Initalizing mongo storage")
                storageApi, _ = mongo.Init(dataRetention, storageConfig.Host, storageConfig.Port, storageConfig.Dbname, storageConfig.Username, storageConfig.Password)
                if err != nil {
                	fmt.Println(err)
                	return
                }
        }else if storageConfig.StorageType == "clickhouse"{
                fmt.Println("Initializing clickhouse storage")
                storageApi, err = clickhouse.Init(dataRetention, storageConfig.Host, storageConfig.Port, storageConfig.Dbname, storageConfig.Username, storageConfig.Password)
                if err != nil{
                        fmt.Println(err)
                        return
                }
        }
	ingester.Init(storageApi)
        listener.LaunchListener(generalConfig.EventCollectorFileName)
}
