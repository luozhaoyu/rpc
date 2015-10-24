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
    basic_service::Stub* stub;
};

// predicate function executed on each iteration of the tester.
struct roundtrip {
    void run(message_data& data) {
        grpc::ClientContext ctx;
        data.stub->int_echo(&ctx, data.request, &data.response);
        data.request.set_data(data.response.data());
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
    int count = basic_service_impl::count;    

    // create clock.
    clocker clk;
    std::cout << "client\n";

    grpc::ClientContext context;
    bulk_message msg;
    tiny_message req;
    clk.begin();
    std::unique_ptr<grpc::ClientReader<bulk_message>> reader =
      stub->pull_list(&context, req);
    while (reader->Read(&msg)) { }
    grpc::Status stat =  reader->Finish();
    clk.end();

    std::cout << std::left << std::setw(60) << std::setfill(' ')
      << "streaming bandwidth" << (count * 2048 * 8 / 1024) << " " << ((double)clk.difference() / 1000000000) << "\n";
    //std::cout << count * 65 * 8 << " " << clk.difference() << "\n"; 
    //clk.dump_cgt();
    return 0;
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
