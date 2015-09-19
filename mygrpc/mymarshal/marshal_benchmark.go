package main

import (
	"fmt"
	"github.com/golang/protobuf/proto"
	"strings"
	"time"
)

const repeat int64 = 100000

func MarshalVariable(sizes []int) {
	var startTime, endTime time.Time
	var du time.Duration
	var s MyString
	var j int64
	var err error
	for _, size := range sizes {
		s.Data = strings.Repeat("1", size)
		startTime = time.Now()
		for j = 0; j < repeat; j++ {
			_, err = proto.Marshal(&s)
		}
		endTime = time.Now()
		du = endTime.Sub(startTime)
		fmt.Printf("%v Marshal %v string: %v\n", err, len(s.Data), du.Nanoseconds()/repeat)
	}
}

func main() {
	var startTime, endTime time.Time
	var du time.Duration
	var i MyInt
	var ii MyDouble
	var c Outer
	var err error
	var j int64
	startTime = time.Now()
	for j = 0; j < repeat; j++ {
		_, err = proto.Marshal(&i)
	}
	endTime = time.Now()
	du = endTime.Sub(startTime)
	fmt.Println("Marshal int", du.Nanoseconds()/repeat, err)

	startTime = time.Now()
	for j = 0; j < repeat; j++ {
		_, err = proto.Marshal(&ii)
	}
	endTime = time.Now()
	du = endTime.Sub(startTime)
	fmt.Println("Marshal double", du.Nanoseconds()/repeat, err)

	MarshalVariable([]int{1, 10, 100, 1000, 10000, 100000})

	startTime = time.Now()
	for j = 0; j < repeat; j++ {
		_, err = proto.Marshal(&c)
	}
	endTime = time.Now()
	du = endTime.Sub(startTime)
	fmt.Println("Marshal complex", du.Nanoseconds()/repeat, err)
}
