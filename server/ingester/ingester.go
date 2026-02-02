package ingester

import (
	"distributedlogger.com/storage"
	"fmt"
	"time"
)

var channels []chan []byte

func workerMain(s storage.StorageAPI, ch chan []byte){
	var batch [][]byte
	ticker := time.NewTicker(10)
	defer ticker.Stop()
	for ;; {
		select {
		case ev, ok := <- ch:
			if !ok {
				s.Flush(batch)
				fmt.Println("Channel is broken")
				return
			}
			batch = append(batch, ev)
			if len(batch) >= 1024 {
				err := s.Flush(batch)
				if err != nil {
					fmt.Println(err)
				}
				batch = batch[:0]
			}
		case <-ticker.C:
			if len(batch) > 0 {
				err := s.Flush(batch)
				if err != nil {
					fmt.Println(err)
					return
				}
				batch = batch[:0]
			}
		}
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
	channels[workerIdx] <- msg
	workerIdx = workerIdx + 1
	if workerIdx == 10 {
		workerIdx = 0
	}
}
