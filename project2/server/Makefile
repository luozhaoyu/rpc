# Makefile
# # by: allison morris

GRPC=../../../grpc
#GRPC=$(HOME)/install/grpc
FLAGS=-g --std=c++11
INCLUDE=-I$(GRPC)/third_party/protobuf/src -I$(GRPC)/include \
 -L$(GRPC)/libs/opt/protobuf -L$(GRPC)/libs/opt -Wl,-rpath $(GRPC)/libs/opt
LIBS=-lgrpc++_unsecure -lgrpc -lgpr -lprotobuf
GRPC_PLUGIN=$(GRPC)/bins/opt/grpc_cpp_plugin
PB=file.pb.o file.grpc.pb.o

all: basic_client filed

arguments.o: arguments.cc arguments.h
	g++ $(FLAGS) -c arguments.cc

basic_client: basic_client.cc file_service.o event_log.o $(PB)
	g++ $(FLAGS) $(INCLUDE) -o basic_client basic_client.cc event_log.o file_service.o $(PB) $(LIBS)

clean:
	rm -rf *.o basic_client filed *.dummy *pb*

event_log.o: event_log.cc event_log.h
	g++ -c event_log.cc $(FLAGS)

persistent_state.o: persistent_state.cc persistent_state.h
	g++ -c persistent_state.cc $(FLAGS)

proto.dummy: ../proto/file.proto
	protoc -I../proto --cpp_out=. ../proto/file.proto
	protoc -I../proto --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_PLUGIN) \
	 ../proto/file.proto
	touch proto.dummy

filed: filed.cc arguments.o file_service.o $(PB) event_log.o
	g++ -o filed filed.cc arguments.o file_service.o event_log.o $(PB) $(FLAGS) $(INCLUDE) $(LIBS)

file.pb.o: proto.dummy
	g++ -c -o file.pb.o file.pb.cc $(FLAGS) $(INCLUDE)

file.grpc.pb.o: proto.dummy
	g++ -c -o file.grpc.pb.o file.grpc.pb.cc $(FLAGS) $(INCLUDE)

file_service.o: file_service.cc file_service.h proto.dummy
	g++ $(FLAGS) $(INCLUDE) -c file_service.cc

.PHONY: all clean
