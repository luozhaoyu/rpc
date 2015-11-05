// file_service.h : declares FileService, an implementation of the grpc service
// by: allison morris


#include <grpc++/grpc++.h>

#include "file.grpc.pb.h"
#include "persistent_state.h"

namespace File {

class FileService : public BasicFileService::Service {
public:
  FileService(const std::string& mount_point, const std::string& persistent_dir,
    const std::string& persistent_store, bool crash)
    : mount_point_(mount_point)
    , persistence_(persistent_dir, persistent_store), crash_write_(crash) { }

  grpc::Status CreateDirectory(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status CreateFile(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status DownloadFile(grpc::ServerContext* ctx, const Path* path,
    File* file) override;

  grpc::Status GetDirectoryContents(grpc::ServerContext* ctx, const Path* path,
    DirInfo* info) override;

  grpc::Status GetFileInfo(grpc::ServerContext* ctx, const Path* path,
    FileInfo* info) override;

  bool Initialize();

  grpc::Status RemoveDirectory(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status RemoveFile(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status UploadFile(grpc::ServerContext* ctx, const FileData* file,
    FileInfo* info) override;
private:
  bool FileExists(const std::string& full_path) const;

  int GetError(int ret) const;

  bool GetFileInfo(const std::string& full_path, const std::string& path, 
    bool top_level, FileInfo* info) const;

  bool GetIfstream(const std::string& full_path, std::ifstream* stream) const;

  const std::string& GetMountPoint() const { return mount_point_; }

  bool GetOfstream(const std::string& full_path, std::ofstream* stream) const;

  std::string PromoteToFullPath(const std::string& suffix) const;

  std::string mount_point_;
  PersistentState persistence_;
  bool crash_write_;
};

}
