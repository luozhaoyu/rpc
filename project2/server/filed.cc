// filed.cc : this is the point-of-entry for the file server.
// by: allison morris

#include "arguments.h"
#include "event_log.h"
#include "file_service.h"

using namespace File;
using grpc::ServerBuilder;

// entry point for file server.
int main(int argc, const char** argv) {
  Arguments args(Arguments::kServer);
  args.Parse(argc, argv);
  if (args.ShowHelp()) {
    return 0;
  }

  if (args.ShowError()) {
    return -1;
  }

  EventLog::Initialize(std::cerr, args.GetVerbosity(), args.GetDumpFiles());

  std::string address = "0.0.0.0:";
  address += std::to_string(args.GetPort());

  Log()->StartupEvent(args.GetMountPoint(), address);

  FileService service(args.GetMountPoint(), args.GetPersistentDirectory(),
    args.GetPersistentStoreName());
  if (!service.Initialize()) {
    return -1;
  }

  ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.BuildAndStart()->Wait();
  
  // TODO: store server pointer and implement shutdown.
  // NOTE: this function will not return until shutdown is implemented.
  return 0;
}
