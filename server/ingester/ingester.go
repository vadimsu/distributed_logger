package ingester

import (
	"distributedlogger.com/storage"
	"fmt"
	"time"
)

type eventMessage struct {
	event uint64
	msg []byte
}

var channels []chan []byte

func workerMain(s storage.StorageAPI, ch chan []byte){
	var batch [][]byte
	ticker := time.NewTicker(10)
	for ;; {
		select {
		case ev, ok := <- ch:
			if !ok {
				s.Flush(batch)
				return
			}
			batch = append(batch, ev)
			if len(batch) >= 1024 {
				s.Flush(batch)
				batch = batch[:0]
			}
		case <-ticker.C:
			if len(batch) > 0 {
				s.Flush(batch)
				batch = batch[:0]
			}
		}
//		msg := <-ch
//		dispatch(msg,s)
	}
}

func Init(s storage.StorageAPI){
	for i := 0;i < 10;i++ {
		ch := make(chan []byte, 1024)
		channels = append(channels, ch)
		go workerMain(s, ch)
	}
}

var workerIdx int = 0

func Enqueue(msg []byte){
//	var min int = -1
//	for i := 0;i < 10;i++ {
//		if min == -1 || len(channels[i]) < min {
//			min = i
//		}
//	}
//	fmt.Println("queueing to ",min," ",len(channels[min]))
//	channels[min] <- msg
	fmt.Println("queueing to ",workerIdx)
	channels[workerIdx] <- msg
	workerIdx = workerIdx + 1
	if workerIdx == 10 {
		workerIdx = 0
	}
}

//func dispatch(msg []byte, s storage.StorageAPI){
//
//	event, decodedThisTime, err := decoder.DecodeUint64(msg)
//	if err == nil {
////		fmt.Println("decoding event ",event)
//		event_decoder.Event_dispatch(event, msg[decodedThisTime:], s)
//	}else{
//		fmt.Println("error in decoding ",err)
//	}
//}
