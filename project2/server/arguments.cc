// arguments.cc : implementations for argument parsing routines.
// by: allison morris

#include <cassert>
#include <iostream>
#include "arguments.h"

// this function parses command line arguments for both client and server.
File::Arguments::ErrorType File::Arguments::Parse(int argc, const char** argv) {
  assert(IsServer());
  executable_ = argv[0];
  if (IsServer()) {
    if (argc < 2) { return kMissingMountPoint; }
    mount_point_ = argv[1];
    return kSuccess;
  }
}

// prints a brief error message to standard out regarding the error.
void File::Arguments::ShowError(File::Arguments::ErrorType err) const {
  assert(err != kSuccess);
  std::cout << GetExecutable() << ": ";
  switch (err) {
    case kIllegal: std::cout << "illegal arguments."; break;
    case kMissingMountPoint: std::cout << "missing mount point."; break;
    case kMissingServer: std::cout << "missing server."; break;
    default: std::cout << "unknown error."; break;
  }
  std::cout << std::endl;
}

// prints a help message to standard out.
void File::Arguments::ShowHelp() const {
  if (IsClient()) {
    std::cout << "Usage: " << GetExecutable() << "[-p port] [-D cache_dir] "
      "server mount_point";
  } else {
    std::cout << "Usage: " << GetExecutable() << "[-p port] mount_point";
  }
  std::cout << std::endl;
}
