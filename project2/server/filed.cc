// filed.cc : this is the point-of-entry for the file server.
// by: allison morris

#include "arguments.h"
#include "log.h"
#include "file_service.h"

using namespace File;
using grpc::ServerBuilder;

// entry point for file server.
int main(int argc, const char** argv) {
  Arguments args(Arguments::kServer);
  Arguments::ErrorType err = args.Parse(argc, argv);
  if (err != Arguments::kSuccess) {
    args.ShowError(err);
    return -1;
  }

  std::fstream disabled_stream;
  Logger::Initialize(std::cerr, disabled_stream, kInfo);

  std::string address = "0.0.0.0:";
  address += std::to_string(args.GetPort());

  GetLog() << "Starting service for " << args.GetMountPoint() << " on " 
    << address << "\n";

  FileService service(args.GetMountPoint());
  ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.BuildAndStart()->Wait();
  
  // TODO: store server pointer and implement shutdown.
  // NOTE: this function will not return until shutdown is implemented.
  return 0;
}
