package main

import (
	"flag"
	"fmt"
	"log"
	"net"
	"rpc/part1/goproto"
	"time"
)

func ReliableBlockingCall(conn *net.UDPConn, request *part1.MyRPC) (*part1.MyRPC, float64, int, error) {
	var n int
	var startTime, endTime time.Time
	var err error
	var response part1.MyRPC
	var written int
	buf := make([]byte, 8192)

	startTime = time.Now()
	for {
		err = conn.SetWriteDeadline(time.Now().Add(200 * time.Millisecond))
		part1.CheckError(err)
		data, err := request.MarshalBinary()
		part1.CheckError(err)
		written, err = conn.Write(data)
		//log.Println(request)
		//log.Println("written", written, len(request.Data))
		part1.CheckError(err)

		err = conn.SetReadDeadline(time.Now().Add(200 * time.Millisecond))
		part1.CheckError(err)
		n, _, err = conn.ReadFromUDP(buf)
		//log.Println("readn", n, conn.RemoteAddr())
		if err != nil {
			switch err := err.(type) {
			case net.Error:
				if err.Timeout() {
					continue
				} else {
					log.Fatal(err)
				}
			default:
				log.Fatal(err)
			}
		}
		endTime = time.Now()
		err = response.UnMarshalBinary(buf[0:n])
		break
	}
	du := endTime.Sub(startTime)
	return &response, du.Seconds(), written, err
}

func RoundTripCall(conn *net.UDPConn) error {
	totalTimes := 1000
	var sumTime, sumRPCTime float64
	request := part1.MyRPC{FunctionId: 1, Flag: 0, Data: []byte{}}
	for i := 0; i < totalTimes; i++ {
		startTime := time.Now()
		response, seconds, _, err := ReliableBlockingCall(conn, &request)
		endTime := time.Now()
		sumRPCTime += endTime.Sub(startTime).Seconds()
		if err != nil || response.Flag == 0 {
			log.Fatal(response, err)
		}
		sumTime += seconds
	}
	log.Printf("Tried %v times: %v ms in average\n", totalTimes, 1000*sumTime/float64(totalTimes))
	log.Printf("Used time %v\tRPC time %v\n", sumTime, sumRPCTime)
	return nil
}

func CreateNewRequest(dataSize int) (*part1.MyRPC, error) {
	request := part1.MyRPC{FunctionId: 1, Flag: 0, Data: make([]byte, dataSize)}
	return &request, nil
}

func Bandwidth(conn *net.UDPConn, dataSize int) (float64, error) {
	var sumTime float64
	var sumBytes int
	totalTimes := 1000
	request, err := CreateNewRequest(dataSize)
	if err != nil {
		log.Fatal(err)
	}
	for i := 0; i < totalTimes; i++ {
		response, seconds, written, err := ReliableBlockingCall(conn, request)
		if err != nil || response.Flag == 0 {
			log.Fatal(response, err)
		}
		sumTime += seconds
		sumBytes += written
	}
	return float64(sumBytes) / 1024 / 1024 / sumTime, nil
}

func AllBandwidth(conn *net.UDPConn) error {
	sizes := []int{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 40000, 45000, 50000, 60000, 65535, 70000, 131072}
	for _, size := range sizes {
		speed, err := Bandwidth(conn, size)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("%v\t%v\n", size, speed)
	}
	return nil
}

func main() {
	var action string
	var dataSize int
	var ip string
	flag.StringVar(&action, "a", "roundtrip", "roundtrip|bandwidth|all")
	flag.StringVar(&ip, "ip", "localhost:31349", "server ip and port")
	flag.IntVar(&dataSize, "size", 128, "data size")
	flag.Parse()
	log.Println(ip)

	serverAddr, err := net.ResolveUDPAddr("udp", ip)
	part1.CheckError(err)
	//localAddr, err := net.ResolveUDPAddr("udp", "127.0.0.1:0")
	part1.CheckError(err)

	conn, err := net.DialUDP("udp", nil, serverAddr)
	part1.CheckError(err)
	defer conn.Close()

	if action == "roundtrip" {
		if err := RoundTripCall(conn); err != nil {
			log.Fatal(err)
		}
	} else if action == "bandwidth" {
		speed, err := Bandwidth(conn, dataSize)
		if err != nil {
			log.Fatal(err)
		}
		log.Printf("%v\t%v", dataSize, speed)
	} else if action == "all" {
		AllBandwidth(conn)
	} else {
		log.Println("Wrong Input")
	}
}
