// arguments.h : provides declarations for argument parsing.
// by: allison morris

#include <list>
#include <string>
#include "event_log.h"

namespace File {

// this class is used to parse command-line arguments for both client
// and server. 
class Arguments {
public:
  enum ErrorType {
      kIllegalPort
    , kIllegalVerbosity
    , kInvalidOption
    , kMissingMountPoint
  };

  enum ModeType { kClient, kServer };

  enum StateType {
      kReady
    , kReadPort
    , kReadCacheDir
    , kReadPersistentDir
    , kReadPersistentStore
    , kReadVerbosity
  };

  Arguments(ModeType mode)
    : mode_(mode)
    , port_(61512)
    , dump_files_(false)
    , show_help_(false)
    , fuse_args_(2)
    , verbosity_(kInfo)
    , server_name_("localhost")
    , cache_directory_("~/.file")
    , persistent_directory_("~/.filed")
    , persistent_store_name_("~/.filed-store")
    { }

  const std::string& GetCacheDirectory() const { return cache_directory_; }

  const std::string& GetExecutable() const { return executable_; }

  bool GetDumpFiles() const { return dump_files_; }

  int GetFuseArgs() const { return fuse_args_; }

  const std::string& GetMountPoint() const { return mount_point_; }

  const std::string& GetPersistentDirectory() const { return persistent_directory_; }

  const std::string& GetPersistentStoreName() const { return persistent_store_name_; }

  int GetPort() const { return port_; }

  const std::string& GetServerName() const { return server_name_; }

  LogLevel GetVerbosity() const { return verbosity_; }

  bool IsClient() const { return mode_ == kClient; }

  bool IsServer() const { return mode_ == kServer; }

  void Parse(int argc, const char** argv);


  bool ShowError() const;

  bool ShowHelp() const;

private:
  StateType Parse(const char* arg, StateType current_state, bool* done);

  const ModeType mode_;
  int port_;
  bool dump_files_;
  bool show_help_;
  int fuse_args_;
  LogLevel verbosity_;
  std::string server_name_;
  std::string mount_point_;
  std::string cache_directory_;
  std::string executable_;
  std::string persistent_directory_;
  std::string persistent_store_name_;
  std::list<ErrorType> errors_;
};

}
