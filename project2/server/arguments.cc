// arguments.cc : implementations for argument parsing routines.
// by: allison morris

#include <cassert>
#include <iostream>
#include "arguments.h"

using namespace File;

// this function parses command line arguments for both client and server.
void  Arguments::Parse(int argc, const char** argv) {
  assert(IsServer());
  executable_ = argv[0];
  
  StateType state = kReady;
  bool read_last_position_arg = false;
  for (int i = 1; i < argc; ++i) {
    state = Parse(argv[i], state, &read_last_position_arg);
    if (read_last_position_arg) {
      fuse_args_ = i + 1;
      return;
    }
  }

  if (!read_last_position_arg) {
    errors_.push_back(kMissingMountPoint);
  }
}

Arguments::StateType Arguments::Parse(const char* arg, StateType current_state, bool* done) {
  switch (current_state) {
    case kReady: {
      if (*arg == '-') {
        if (arg[1] == 0 || arg[2] != 0) {
	  errors_.push_back(kInvalidOption);
	  return kReady;
	}
        switch (arg[1]) {
	  case 'h': show_help_ = true; return kReady;
	  case 'p': return kReadPort;
	  case 'D': return kReadPersistentDir;
	  case 'P': return kReadPersistentStore;
	  case 'V': return kReadVerbosity;
	  case 'd': dump_files_ = true; return kReady;
	  case 'q': verbosity_ = kFatal; return kReady;
	  case 'L': verbosity_ = kTrace; return kReady;
	  case 'c': crash_write_ = true; return kReady;
	  default: errors_.push_back(kInvalidOption); return kReady;
	}
      }

        // done parsing positional arguments. end parse.
	mount_point_ = arg;
	while (mount_point_.back() == '/') { mount_point_.pop_back(); }
	*done = true;
	return kReady;
    } break;
    case kReadPort: {
      char* end_ptr;
      int port = std::strtol(arg, &end_ptr, 10);
      if (*end_ptr != 0 || port < 0 || port > 65535) {
        errors_.push_back(kIllegalPort);
	return kReady;
      }

      port_ = port;
      return kReady;
    } break;
    case kReadCacheDir: {
      assert(IsClient());
    } break;
    case kReadPersistentDir: {
      persistent_directory_ = arg;
      return kReady;
    } break;
    case kReadPersistentStore: {
      persistent_store_name_ = arg;
      return kReady;
    } break;
    case kReadVerbosity: {
      char* end_ptr;
      int verbosity = std::strtol(arg, &end_ptr, 10);
      if (*end_ptr != 0 || !EventLog::ToVerbosity(verbosity, &verbosity_)) {
        errors_.push_back(kIllegalVerbosity);
	return kReady;
      }

      return kReady;
    } break;
  }
  assert(0 && "missing parse state.");
  return kReady;
}

// prints a brief error message to standard out regarding the error.
bool Arguments::ShowError() const {
  if (errors_.empty()) { return false; }

  bool invalid_option = false;
  for (auto err : errors_) {
    std::cout << GetExecutable() << ": ";
    switch (err) {
      case kIllegalPort: std::cout << "illegal port. must be in [0, 65535]."; break;
      case kIllegalVerbosity: std::cout << "illegal verbosity. must be in [0, 4]."; break;
      case kInvalidOption:
        if (!invalid_option) {
	  invalid_option = true;
	  std::cout << "invalid option(s). see help, use -h, for details.";
	} break;
      case kMissingMountPoint:
        std::cout << "mount point requires as first positional argument."; break;
      default: assert(0 && "missing error state.");  break;
    }
    std::cout << std::endl;
  }
  return true;
}

// prints a help message to standard out.
bool Arguments::ShowHelp() const {
  assert(IsServer());
  if (!show_help_) { return false; }

  if (IsClient()) {
    std::cout << "Usage: " << GetExecutable() << " [-p port] [-D cache_dir] "
      "server mount_point";
  } else {
    std::cout << "Usage: " << GetExecutable() << " [options] mount_point\nOptions:\n"
      "    -h     Display this message.\n"
      "    -p n   Listen on port n for client connections.\n"
      "    -D s   Use s as the cache directory. This is called the persistent directory.\n"
      "    -P s   Use s as the location of the persistent store log.\n"
      "    -V n   Set verbosity to level n. Levels are [0, 4]. Default is 1.\n"
      "    -d     Dump file contents to logs.\n"
      "    -q     Set verbosity to minimum. Disable all logging excepts errors.\n"
      "    -L     Set verbosity to maximum.\n";
  }
  std::cout << std::endl;
  return true;
}
