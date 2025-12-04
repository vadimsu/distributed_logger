package storage

type StoredData struct{
        Version uint16
        Event uint16
        Timestamp uint64
        Host string
        Zoneuid string
        Args []string
}

type StorageAPI interface{
        Store(key string, serial int, secondaryKey string, thirdKey string, shard int, storedData map[string]string)(bool,error)
        Query(key string)([]string,error)
        QueryLeastRecent()([]string,error)
        Delete(key string)(int64, error)
}
