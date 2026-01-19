package decoder

import (
	"errors"
	"encoding/binary"
	"bytes"
	"fmt"
//	"distributedlogger.com/event_decoders"
)

func DecodeUint16(packet []byte) (uint16, int, error){
        var value uint16
        value = 0
        if len(packet) < 2{
                return value, 0, errors.New("not enough bytes to decode uint16")
        }
        reader := bytes.NewReader(packet[:2])
        err := binary.Read(reader, binary.BigEndian, &value)
        if err != nil{
                return 0, 0, err
        }
        return value, 2, nil
}

func DecodeUint64(packet []byte) (uint64, int, error){
        var value uint64
        value = 0
        if len(packet) < 8{
                return value, 0, errors.New("not enough bytes to decode uint64")
        }
        reader := bytes.NewReader(packet[:8])
        err := binary.Read(reader, binary.BigEndian, &value)
        if err != nil{
                return 0, 0, err
        }
	fmt.Println("decoded uint64 ",value)
        return value, 8, nil
}

func DecodeString(packet []byte) (string, int, error){
        var value string
        decoded := 0
        stringLength, decodedThisTime, err := DecodeUint16(packet[decoded:])
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
	fmt.Println("decoded string ",value)
        return value, decoded + int(stringLength), nil
}


