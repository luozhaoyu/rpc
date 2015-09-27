# Makefile
# by: allison morris

FLAGS=--std=c++11
GDIR=-I../../grpc/third_party/protobuf/src -L../../grpc/libs/opt
GDIR+= -I../../grpc/include -L../../grpc/libs/opt/protobuf
GLIBS=-lgrpc++_unsecure -lgrpc -lgpr -lprotobuf

marshalling: marshalling.cc measure.h clocker.h grpc.proto.dummy
	g++ -o marshalling $(FLAGS) marshalling.cc grpc.pb.cc $(GDIR) $(GLIBS)

roundtrip: roundtrip.cc measure.h clocker.h grpc.proto.dummy
	g++ -o roundtrip $(FLAGS) roundtrip.cc basic_service.cc grpc.grpc.pb.cc grpc.pb.cc $(GDIR) $(GLIBS)

grpc.proto.dummy: grpc.proto
	protoc --cpp_out=. grpc.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=../../grpc/bins/opt/grpc_cpp_plugin grpc.proto
	touch grpc.proto.dummy
