package main

import (
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"log"
	"os"
	"rpc/mygrpc/myproto"
	"time"
)

func main() {
	var startTime, endTime time.Time
	var total, i, max int64
	var du time.Duration
	var message, address string
	max = 10000

	if len(os.Args) > 2 {
		address = os.Args[1]
		message = os.Args[2]
	} else {
		address = "localhost:50051"
		message = "default message"
		log.Println("default args:", address, message)
	}

	conn, err := grpc.Dial(address, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	c := myproto.NewEchoerClient(conn)

	for i = 0; i < max; i++ {
		startTime = time.Now()
		r, err := c.Echo(context.Background(), &myproto.RequestMessage{Message: message})

		endTime = time.Now()
		du = endTime.Sub(startTime)
		total += du.Nanoseconds()
		if i*100%max == 0 {
			log.Printf("%v%% Echo: %v %v", i*100/max, r.Message, total/(i+1))
		}
		if err != nil {
			log.Fatalf("could not echo: %v", err)
		}
	}
}
