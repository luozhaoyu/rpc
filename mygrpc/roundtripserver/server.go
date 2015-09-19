package main

import (
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"log"
	"net"
	"rpc/mygrpc/myproto"
)

type server struct{}

func (s *server) Echo(ctx context.Context, in *myproto.RequestMessage) (*myproto.ResponseMessage, error) {
	return &myproto.ResponseMessage{Message: in.Message}, nil
}

func main() {
	l, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	} else {
		log.Printf("I am listening to: %v", l)
	}
	s := grpc.NewServer()
	myproto.RegisterEchoerServer(s, &server{})
	s.Serve(l)
}
