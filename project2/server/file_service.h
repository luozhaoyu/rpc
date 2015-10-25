// file_service.h : declares FileService, an implementation of the grpc service
// by: allison morris


#include <grpc++/grpc++.h>

#include "file.grpc.pb.h"

namespace File {

class FileService : public BasicFileService::Service {
public:
  FileService(std::string mount_point) : mount_point_(mount_point) { }

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

  grpc::Status RemoveDirectory(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status RemoveFile(grpc::ServerContext* ctx, const Path* path,
    Result* result) override;

  grpc::Status UploadFile(grpc::ServerContext* ctx, const FileData* file,
    FileInfo* info) override;
private:
  bool FileExists(const std::string& full_path) const;

  bool GetFileInfo(const std::string& full_path, FileInfo* info) const;

  bool GetIfstream(const std::string& full_path, std::ifstream* stream) const;

  const std::string& GetMountPoint() const { return mount_point_; }

  bool GetOfstream(const std::string& full_path, std::ofstream* stream) const;

  std::string PromoteToFullPath(const std::string& suffix) const;

  std::string mount_point_;
};

}
