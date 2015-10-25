// arguments.h : provides declarations for argument parsing.
// by: allison morris

#include <string>

namespace File {

// this class is used to parse command-line arguments for both client
// and server. 
class Arguments {
public:
  enum ErrorType { kSuccess, kIllegal, kMissingServer, kMissingMountPoint };
  enum ModeType { kClient, kServer };

  Arguments(ModeType mode) : mode_(mode), port_(61512), fuse_args_(2),
    server_("localhost"), mount_point_("/mnt"), cache_directory_("~/.file") { }

  const std::string& GetCacheDirectory() const { return cache_directory_; }

  const std::string& GetExecutable() const { return executable_; }

  int GetFuseArgs() const { return fuse_args_; }

  const std::string& GetMountPoint() const { return mount_point_; }

  int GetPort() const { return port_; }

  const std::string& GetServer() const { return server_; }

  bool IsClient() const { return mode_ == kClient; }

  bool IsServer() const { return mode_ == kServer; }

  ErrorType Parse(int argc, const char** argv);

  void ShowError(ErrorType err) const;

  void ShowHelp() const;

private:
  const ModeType mode_;
  int port_;
  int fuse_args_;
  std::string server_;
  std::string mount_point_;
  std::string cache_directory_;
  std::string executable_;
};

}
