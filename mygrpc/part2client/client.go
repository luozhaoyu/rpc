package main

import (
	"flag"
	"fmt"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"log"
	"math/rand"
	"rpc/mygrpc/myproto"
	"time"
)

func GetRandomString(size int) string {
	var letters = []rune("abcdefghijklmnopqrstuvwxyz")
	r := make([]rune, size)
	for i := range r {
		r[i] = letters[rand.Intn(len(letters))]
	}
	return string(r)
}

func Roundtrip(c myproto.EchoerClient, message string) error {
	var du time.Duration
	var startTime, endTime time.Time
	var total, i int64
	var max int64 = 10000

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
	return nil
}

func Bandwidth(c myproto.EchoerClient, message string) (float64, *myproto.ResponseMessage, error) {
	var totalBytes int = 1024 * len(message)
	var stream myproto.Echoer_SendClientStreamClient
	var err error
	var request myproto.RequestMessage
	var response *myproto.ResponseMessage
	var du time.Duration
	var startTime, endTime time.Time

	if len(message) < 1 {
		log.Fatal("message length < 1", message)
	}

	if stream, err = c.SendClientStream(context.Background()); err != nil {
		log.Fatal(err)
	}
	request = myproto.RequestMessage{Message: message}

	startTime = time.Now()
	for i := 0; i < totalBytes/len(message); i++ {
		if err = stream.Send(&request); err != nil {
			log.Println("stream.Send fail", err)
		}
	}
	// don't include the ack time, since tcp has already acked it for you
	endTime = time.Now()
	if response, err = stream.CloseAndRecv(); err != nil {
		log.Fatal(err)
	}
	du = endTime.Sub(startTime)
	speed := float64(totalBytes) / du.Seconds() / 1024 / 1024
	return speed, response, err
}

func AllBandwidth(c myproto.EchoerClient) error {
	sizes := []int{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144}
	for _, i := range sizes {
		speed, _, err := Bandwidth(c, GetRandomString(i))
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("%v\t%v\n", i, speed)
	}
	return nil
}

func main() {
	var message, address, rpc string
	var size int

	flag.StringVar(&address, "address", "localhost:50051", "Server IP address")
	flag.IntVar(&size, "size", 8, "random message size")
	flag.StringVar(&rpc, "rpc", "roundtrip|bandwidth|allbandwidth", "bandwidth: client stream")
	flag.Parse()
	message = GetRandomString(size)

	conn, err := grpc.Dial(address, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	c := myproto.NewEchoerClient(conn)

	if rpc == "roundtrip" {
		Roundtrip(c, message)
	} else if rpc == "bandwidth" {
		speed, response, err := Bandwidth(c, message)
		log.Println(speed, response, err)
	} else if rpc == "allbandwidth" {
		AllBandwidth(c)
	} else {
		log.Println("Wrong rpc input:", rpc)
	}
}
