package main

import (
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"io"
	"log"
	"net"
	"rpc/mygrpc/myproto"
)

type server struct{}

func (s *server) Echo(ctx context.Context, in *myproto.RequestMessage) (*myproto.ResponseMessage, error) {
	return &myproto.ResponseMessage{Message: in.Message}, nil
}

func (s *server) SendClientStream(stream myproto.Echoer_SendClientStreamServer) error {
	//var request myproto.RequestMessage

	for {
		_, err := stream.Recv()
		if err == io.EOF {
			return stream.SendAndClose(&myproto.ResponseMessage{Message: "SUCCESS"})
		}
		if err != nil {
			return err
		}
	}
}

func main() {
	l, err := net.Listen("tcp", ":31350")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	} else {
		log.Printf("I am listening at: 31350")
	}
	s := grpc.NewServer()
	myproto.RegisterEchoerServer(s, &server{})
	s.Serve(l)
}
