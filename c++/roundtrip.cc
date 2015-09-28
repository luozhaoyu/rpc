// roundtrip.cc : tests roundtrip rpc.
// by: allison morris

#include "basic_service.h"
#include "measure.h"

int run_client(std::string);
int run_server(std::string);

// data to be passed to measurement functions.
struct message_data {
    tiny_message request;
    tiny_message response;
    grpc::ClientContext* context;
    basic_service::Stub* stub;
};

// predicate function executed on each iteration of the tester.
struct roundtrip {
    void run(message_data& data) {
        data.stub->int_echo(data.context, data.request, &data.response);
        data.request.set_data(data.response.data());
        std::cout << "sent" << std::endl;
    }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Need SERVER or CLIENT followed by ADDRESS.\n";
        return 1;
    }
    
    std::string mode = argv[1];
    if (mode == "SERVER") {
        return run_server(argv[2]); 
    } else if (mode == "CLIENT") {
        return run_client(argv[2]);
    } else {
        std::cerr << "Need SERVER or CLIENT followed by ADDRESS.\n";
        return 1;
    }
}

// runs the client side of the test.
int run_client(std::string address) {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address, grpc::InsecureCredentials());
    std::shared_ptr<basic_service::Stub> stub = basic_service::NewStub(channel);
    clocker::mode mode = clocker::clock_gettime;
    int count = 15;    

    message_data data;
    grpc::ClientContext context;
    data.request.set_data(13);
    data.context = &context;
    data.stub = stub.get();
    measure("Average echo time", roundtrip(), data, mode, count);
   
    grpc::Status status = stub->int_echo(data.context, data.request, &data.response);
    
    if (status.ok()) {
        std::cout << "Sent " << data.request.data() << " and received " << data.response.data() << std::endl;
    } else {
        std::cout << "Failed to send. " << std::endl;
    }
}

// runs the server side of the test.
int run_server(std::string address) {
    grpc::ServerBuilder builder;
    basic_service_impl service;
    
    // start service.
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    // launch.
    std::cout << "Launching server on " << address << std::endl;
    builder.BuildAndStart()->Wait();
    
    // note: this is never reached.
    return 0;
}
