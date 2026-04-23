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
	"distributedlogger.com/ingester"
	"distributedlogger.com/config"
	"crypto/tls"
	"crypto/x509"
	"bufio"
)

func handleConnection(conn net.Conn){
        var readBuf []byte
        var headerBuf []byte
        var packetLength uint32
        packetLength = 0
        alreadyRead := 0
        headerBuf = make([]byte, 4)
        for{
                if packetLength == 0{
                        numOfBytes, err := conn.Read(headerBuf[alreadyRead:])
			if numOfBytes > 0 {
				alreadyRead = alreadyRead + numOfBytes
	                        if alreadyRead == 4{
					reader := bytes.NewReader(headerBuf)
					err := binary.Read(reader, binary.BigEndian, &packetLength)
					if err != nil {
						fmt.Println(err)
						break
					}
					alreadyRead = 0
					readBuf = make([]byte, packetLength)
				}
			}
			if err != nil{
                                break
                        }
                }else{
                        if alreadyRead < int(packetLength){
                                numOfBytes, err := conn.Read(readBuf[alreadyRead:])
                                if numOfBytes > 0 {
					alreadyRead = alreadyRead + numOfBytes
	                                if alreadyRead == int(packetLength){
						if len(readBuf) > 12 {
							ingester.Enqueue(readBuf)
						}else{
							fmt.Println("unexpectedly short packet ",len(readBuf))
							break
						}
						packetLength = 0
						alreadyRead = 0
					}
				}
				if err != nil{
                                        break
                                }
                        }
                }
        }
        defer conn.Close()
}

func LaunchListener(confPath string){
        jsonFile, err := os.Open(confPath)
        if err != nil{
                fmt.Println(err)
                return
        }else{
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
                conn, err := ln.Accept()
                if err != nil {
                        fmt.Println(err)
                        return
                }
		bufio.NewReaderSize(conn,1024*1024*1024)
		bufio.NewWriterSize(conn,1024*1024*100)
                go handleConnection(conn)
        }
        defer jsonFile.Close()
}
