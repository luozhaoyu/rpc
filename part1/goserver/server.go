package main

import (
	"errors"
	"flag"
	"log"
	"math/rand"
	"net"
	"rpc/part1/goproto"
	"time"
)

func ProcessRequest(request *part1.MyRPC) (*part1.MyRPC, error) {
	var response part1.MyRPC

	if request.Flag == 0 { // this is MSG
		if request.FunctionId == 1 {
			response.Flag = 1 // set ACK flag
			return &response, nil
		} else {
			log.Println("Unsupported Function")
			return nil, errors.New("Unsupported Function")
		}
	} else { // All else is wrong
		log.Println("Wrong Flag")
		return nil, errors.New("Unsupported Function")
	}
}

// This example transmits a value that implements the custom encoding and decoding methods.
func main() {
	var dropRate int

	flag.IntVar(&dropRate, "P", 0, "probability to drop packet")
	flag.Parse()

	serverAddr, err := net.ResolveUDPAddr("udp", ":31349")
	if err != nil {
		log.Fatal(err)
	}
	conn, err := net.ListenUDP("udp", serverAddr)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()
	log.Println("I am UDP server at:", serverAddr)

	var request part1.MyRPC
	total := 0
	dropped := 0

	buf := make([]byte, 81920)

	go func(dropped *int, total *int) {
		c := time.Tick(5 * time.Second)
		for _ = range c {
			log.Printf("Dropped: %v in %v\n", *dropped, *total)
		}
	}(&dropped, &total)

	for {
		n, clientAddr, err := conn.ReadFromUDP(buf)
		part1.CheckError(err)
		err = request.UnMarshalBinary(buf[0:n])
		//log.Println(request)
		part1.CheckError(err)
		seed := rand.New(rand.NewSource(time.Now().UTC().UnixNano()))
		random := seed.Int() % 100
		if random < dropRate { // drop it
			dropped += 1
			total += 1
			//log.Println("Dropped a packet", dropped, total)
			continue
		} else {
			total += 1
			response, err := ProcessRequest(&request)
			//log.Println("read bytes:", n, buf[0:n], request)
			if err != nil {
				log.Println(err)
			} else {
				data, err := response.MarshalBinary()
				part1.CheckError(err)
				n, err = conn.WriteToUDP(data, clientAddr)
				//log.Println("written bytes:", clientAddr, n)
				if err != nil {
					log.Println(err)
				}
			}
		}
	}
}
