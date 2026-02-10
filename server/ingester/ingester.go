package ingester

import (
	"distributedlogger.com/storage"
	"fmt"
	"time"
	"runtime"
)

var channels []chan []byte
var workerIdx int = 0
var numberOfWorkers int = 0
var workersBufferSize int = 1024

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
	for i := 0;i < numberOfWorkers;i++ {
		ch := make(chan []byte, workersBufferSize)
		channels = append(channels, ch)
		go workerMain(s, ch)
	}
}

func Enqueue(msg []byte){
	channels[workerIdx] <- msg
	workerIdx = workerIdx + 1
	if workerIdx == numberOfWorkers {
		workerIdx = 0
	}
}
