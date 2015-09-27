// roundtrip.cc : tests roundtrip rpc.
// by: allison morris

#include "basic_service.h"
#include "measure.h"

int run_client(std::string);
int run_server(std::string);

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
    
    tiny_message request;
    tiny_message response;
    grpc::ClientContext context;
    request.set_data(13);
    
    clocker clk;
    clk.begin();
    grpc::Status status = stub->int_echo(&context, request, &response);
    clk.end();
    
    if (status.ok()) {
        std::cout << "Sent " << request.data() << " and received " << response.data() << " in " << clk.difference() << std::endl;
    } else {
        std::cout << "Failed in " << clk.difference() << std::endl;
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
