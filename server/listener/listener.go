package listener

import (
	"encoding/json"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
	"net"
	"bytes"
	"distributedlogger.com/storage"
	"distributedlogger.com/decoder"
	"distributedlogger.com/config"
	"crypto/tls"
	"crypto/x509"
)

func handleConnection(conn net.Conn, storageAPI storage.StorageAPI){
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
//                                        if len(readBuf) > 12 {
						event, decodedThisTime, err := decoder.DecodeUint64(readBuf)
						if err == nil {
							fmt.Println("decoding event ",event)
							decoder.Event_dispatch(event, readBuf[decodedThisTime:], storageAPI)
						}else{
							fmt.Println("error in decoding ",err)
						}
  //                                      }else{
    //                                            fmt.Println("unexpectedly short packet ",len(readBuf))
      //                                  }
                                        packetLength = 0
                                        alreadyRead = 0
                                }
                        }
                }
        }
        defer conn.Close()
}

func LaunchListener(confPath string, storageAPI storage.StorageAPI){
        jsonFile, err := os.Open(confPath)
        if err != nil{
                fmt.Println(err)
                return
        }else{
                fmt.Println("opened")
        }
        var config config.Config
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
