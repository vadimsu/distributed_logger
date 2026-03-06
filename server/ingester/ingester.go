package ingester

import (
	"distributedlogger.com/storage"
	"fmt"
	"time"
	"runtime"
)

var channel chan []byte
var numberOfWorkers int = 0
var workersBufferSize int = 1024

func workerMain(s storage.StorageAPI, ch chan []byte){
	var batch [][]byte
	ticker := time.NewTicker(100000  * time.Millisecond)
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
			if len(batch) >= workersBufferSize {
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

func Init(s storage.StorageAPI, workers int, bufferSize int){
	if workers > 0 {
		numberOfWorkers = workers
	}else{
		numberOfWorkers = runtime.NumCPU()
	}
	if bufferSize > 0 {
		workersBufferSize = bufferSize
	}
	channel = make(chan []byte, workersBufferSize)
	for i := 0;i < numberOfWorkers;i++ {
		go workerMain(s, channel)
	}
}

func Enqueue(msg []byte){
	channel <- msg
}
