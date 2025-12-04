package event_collector

import (
    "encoding/json"
    "encoding/binary"
    "fmt"
    "io/ioutil"
    "os"
    "strconv"
    "net"
    "bytes"
    "errors"
    "distributedlogger.com/storage"
    "crypto/tls"
    "crypto/x509"
//    "bufio"
)

type ListenerConfig struct{
        Port string `json:"port"`
        Ip string `json:"ip"`
}

type Certificate struct{
        Server string `json:"server"`
        Key string `json:"key"`
        Root string `json:"root"`
}

type Config struct{
        Listener ListenerConfig `json:"listener"`
        Certificates Certificate `json:"certificate"`
}

func decodeUint16(packet []byte) (uint16, int, error){
        var value uint16
        value = 0
        if len(packet) < 2{
                return value, 0, errors.New("not enough bytes to decode uint16")
        }
        reader := bytes.NewReader(packet[:2])
        err := binary.Read(reader, binary.LittleEndian, &value)
        if err != nil{
                return 0, 0, err
        }
        return value, 2, nil
}

func decodeUint64(packet []byte) (uint64, int, error){
        var value uint64
        value = 0
        if len(packet) < 8{
                return value, 0, errors.New("not enough bytes to decode uint64")
        }
        reader := bytes.NewReader(packet[:8])
        err := binary.Read(reader, binary.LittleEndian, &value)
        if err != nil{
                return 0, 0, err
        }
        return value, 8, nil
}
func decodeString(packet []byte, skipType bool) (string, int, error){
        var value string
        decoded := 0
        if skipType == false{
                stringType, decodedThisTime, err := decodeUint16(packet)
                if err != nil{
                        fmt.Println(err)
                        return "", 0, errors.New("cannot decode stringType")
                }
                if stringType != 1{
                        fmt.Println("stringType ",stringType," expected 1")
                        return "", 0, errors.New("stringType mismatch")
                }
                decoded += decodedThisTime
        }
        if len(packet) <= decoded{
                return "",0,errors.New("not enough bytes to decode string length")
        }
        stringLength, decodedThisTime, err := decodeUint16(packet[decoded:])
        if err != nil{
                fmt.Println(err)
                return "", 0, errors.New("cannot decode string length")
        }
        value = ""
//      fmt.Println("decoding string ",decoded, " ",stringLength)
        decoded += decodedThisTime
        if len(packet) < decoded + int(stringLength) {
                return "", 0, errors.New("not enough bytes to decode string")
        }
        value = string(packet[decoded:decoded+int(stringLength)])
        return value, decoded + int(stringLength), nil
}

func decodeEvent(packet []byte) (uint16, int, error){
        return decodeUint16(packet)
}

func decodeInt(packet []byte) (uint64, int, error){
        decoded := 0
        tlvType, decodedThisTime, err := decodeUint16(packet)
        if err != nil{
                fmt.Println(err)
                return 0, 0, errors.New("error in decoding tlvType")
        }
        decoded += decodedThisTime
        if tlvType != 2{
                fmt.Println("TLV type ",tlvType," expected 2")
                return 0,0, errors.New("wrong TLV type")
        }
        if len(packet) <= decoded{
                return 0, 0, errors.New("Not enough bytes to decode TS")
        }
        value, decodedThisTime, err := decodeUint64(packet[decoded:])
        if err != nil{
                fmt.Println(err)
                return 0, 0, errors.New("cannot decode TS value")
        }
        return value, decoded + decodedThisTime, err
}

func decodePacket(packet []byte) (string, int, string, string, int, map[string]string, error){
        decoded := 0
        var key string
        var serial int
        var secondaryKey string
        var thirdKey string
        var shard int
        var data map[string]string
        version, decodedThisTime, err := decodeUint16(packet[0:])
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode version")
        }
        data = make(map[string]string)
//      fmt.Println("Version ",version, " decoded ",decodedThisTime)
        data["version"] = strconv.Itoa(int(version))
        decoded += decodedThisTime
        if len(packet) <= decoded{
                fmt.Println("Not enough bytes to decode Event")
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("not enough bytes to decode event")
        }
        event, decodedThisTime, err := decodeEvent(packet[decoded:])
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode event")
        }
//      fmt.Println("Event ",event, " decoded ",decodedThisTime)
        thirdKey = strconv.Itoa(int(event))
        decoded += decodedThisTime
        if len(packet) <= decoded{
                fmt.Println("Not enough bytes to decode TS")
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("not enough bytes to decode timestamp")
        }
        decodeTs, decodedThisTime, err := decodeInt(packet[decoded:])
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode timestamp")
        }
//      fmt.Println("TS ",decodeTs, " decoded ",decodedThisTime)
        data["timestamp"] = strconv.Itoa(int(decodeTs))
        decoded += decodedThisTime
        if len(packet) <= decoded{
                fmt.Println("Not enough bytes to decode host")
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("not enough bytes to decode host")
        }
        host, decodedThisTime, err := decodeString(packet[decoded:], false)
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode host")
        }
//      fmt.Println("Host ",host, " decoded ",decodedThisTime)
        secondaryKey = host
        decoded += decodedThisTime
        if len(packet) <= decoded{
                fmt.Println("Not enough bytes to decode zoneuid")
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("not enough bytes to decode zoneuid")
        }
        zoneuid, decodedThisTime, err := decodeString(packet[decoded:], false)
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode zoneuid")
        }
//      fmt.Println("ZoneUID ",zoneuid, " decoded ",decodedThisTime)
        key = zoneuid
        decoded += decodedThisTime
        serial_64, decodedThisTime, err := decodeInt(packet[decoded:])
	if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode serial")
        }
        serial = int(serial_64)
        decoded += decodedThisTime
        shard_64, decodedThisTime, err := decodeInt(packet[decoded:])
        if err != nil{
                fmt.Println(err)
                return key, serial, secondaryKey, thirdKey, shard, data, errors.New("Cannot decode shard")
        }
        shard = int(shard_64)
        decoded += decodedThisTime
//      fmt.Println("decoded ",decoded," len ",len(packet))
        argIdx := 1
        for decoded < len(packet) {
                if len(packet) <= decoded{
                        fmt.Println("Not enough bytes to decode TLV type")
                        break
                }
                tlvType, decodedThisTime, err := decodeUint16(packet[decoded:])
                if err != nil{
                        fmt.Println(err)
                        break
                }
                decoded += decodedThisTime
                if len(packet) <= decoded{
                        fmt.Println("Not enough bytes to decode TLV value")
                        break
                }
                if tlvType == 1{
                        strArg, decodedThisTime, err := decodeString(packet[decoded:], true)
                        decoded += decodedThisTime
                        if err != nil{
                                fmt.Println(err)
                                break
                        }else{
//                              fmt.Println("decoded str argument ",strArg)
                                data["arg#"+strconv.Itoa(argIdx)] = strArg
                                argIdx = argIdx + 1
                        }
                }else if tlvType == 2{
                        value, decodedThisTime, err := decodeUint64(packet[decoded:])
                        decoded += decodedThisTime
                        if err != nil{
                                fmt.Println(err)
                                break
                        }else{
//                              fmt.Println("decoded uint64 argument ",value)
                                data["arg#"+strconv.Itoa(argIdx)] = strconv.Itoa(int(value))
                                argIdx = argIdx + 1
                        }
                }else{
                        fmt.Println("Wrong tlv type ",tlvType)
                        break
                }
        }
        return key, serial, secondaryKey, thirdKey, shard, data, nil
}

func handleConnection(conn net.Conn, storageAPI map[int]storage.StorageAPI){
        var readBuf []byte
        var headerBuf []byte
        var packetLength uint32
        packetLength = 0
        alreadyRead := 0
        headerBuf = make([]byte, 4)
        fmt.Println("handling connection")
        for{
                if packetLength == 0{
                        fmt.Println("read offset ",alreadyRead)
                        numOfBytes, err := conn.Read(headerBuf[alreadyRead:])
                        if err != nil{
                                fmt.Println("error on read ",err)
                                break
                        }
                        fmt.Println("read ",numOfBytes)
                        alreadyRead = alreadyRead + numOfBytes
                        if alreadyRead == 4{
                                reader := bytes.NewReader(headerBuf)
                                _ = binary.Read(reader, binary.BigEndian, &packetLength)
                                alreadyRead = 0
                                //fmt.Println("packet length ",packetLength)
                                readBuf = make([]byte, packetLength)
                        }
                }else{
                        fmt.Println("waiing for data ",alreadyRead, " ",packetLength)
                        if alreadyRead < int(packetLength){
                                numOfBytes, err := conn.Read(readBuf[alreadyRead:])
                                if err != nil{
                                        break
                                }
                                alreadyRead = alreadyRead + numOfBytes
                                if alreadyRead == int(packetLength){
                                        if len(readBuf) > 6 {
                                                key, serial, key2, key3, shard, data, err := decodePacket(readBuf[6:])
                                                if err != nil{
                                                        fmt.Println(err)
                                                }else{
                                                        _, ok := storageAPI[shard]
                                                        if ok {
                                                                storageAPI[shard].Store(key, serial, key2, key3, shard, data)
                                                        }else{
                                                                fmt.Println("not storageAPI for ",shard)
                                                        }
                                                }
                                        }else{
                                                fmt.Println("unexpectedly short packet ",len(readBuf))
                                        }
                                        packetLength = 0
                                        alreadyRead = 0
                                }
                        }
                }
        }
        defer conn.Close()
}

func LaunchListener(confPath string, storageAPI map[int]storage.StorageAPI){
        jsonFile, err := os.Open(confPath)
        if err != nil{
                fmt.Println(err)
                return
        }else{
                fmt.Println("opened")
        }
        var config Config
        var ln net.Listener
        byteValue, _ := ioutil.ReadAll(jsonFile)
        json.Unmarshal(byteValue,&config)
        fmt.Println(config.Listener.Port)
        fmt.Println(config.Listener.Ip)
        fmt.Println(config.Certificates.Server)
        fmt.Println(config.Certificates.Root)
        fmt.Println(config.Certificates.Key)
        if config.Certificates.Server == "" ||
        config.Certificates.Key == "" {
                ip, _,_ := net.ParseCIDR(config.Listener.Ip)
                port, err := strconv.Atoi(config.Listener.Port)
                if err != nil {
                        fmt.Println(err)
                        return
                }
                tcpAddr := net.TCPAddr { IP: ip, Port:  port}
                ln, err = net.ListenTCP("tcp",&tcpAddr)
        }else {
                cer, err := tls.LoadX509KeyPair(config.Certificates.Server, config.Certificates.Key)
                if err != nil {
                        fmt.Println(err)
                        return
                }

                certs, err := ioutil.ReadFile(config.Certificates.Root)
                if err != nil {
                        fmt.Println("Invalid CACert: ", err)
                        return
                }
                rootCAs := x509.NewCertPool()
                ok := rootCAs.AppendCertsFromPEM(certs)

                if !ok {
                        fmt.Println("Taking default certificates")
                }
                tlsConfig := &tls.Config{Certificates: []tls.Certificate{cer}, ClientAuth: tls.RequireAndVerifyClientCert, ClientCAs: rootCAs}
                ln, err = tls.Listen("tcp", config.Listener.Ip+":"+config.Listener.Port, tlsConfig)
        }
        if err != nil {
                fmt.Println(err)
                return
        }
        for{
                fmt.Println("Accepting")
                conn, err := ln.Accept()
                if err != nil {
                        fmt.Println(err)
                        return
                }
                fmt.Println("Accepted")
//              conn.SetReadBuffer(1024*1024*10)
                go handleConnection(conn, storageAPI)
        }
        defer jsonFile.Close()
}
