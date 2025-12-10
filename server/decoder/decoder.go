
import (
	"errors"
	"encoding/binary"
	"bytes"
	"fmt"
	"event_decoders"
)

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


