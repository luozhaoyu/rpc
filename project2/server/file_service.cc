// file_service.cc
// by: allison morris

// this file implements the server-side of the rpc calls. these functions
// prioritize using standard c++ streams for simplicity and portability at
// the expense of performance. however, performance is appropriate for our
// needs. 

#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include "file_service.h"
#include "event_log.h"

using namespace File;
using grpc::ServerContext;
using grpc::Status;

Status FileService::CreateDirectory(ServerContext* ctx, const Path* path,
    Result* result) {
  assert(path != nullptr && result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  int ret = mkdir(full_path.c_str(), 0755);
  int err = GetError(ret);
  Log()->CreateDirectoryEvent(full_path, path->data(), err);
  result->set_error_code(err);
  return Status::OK;
}

Status FileService::CreateFile(ServerContext* ctx, const Path* path,
    Result* result) {
  assert(path != nullptr);
  assert(result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());

  // cannot create if file already exists.
  if (FileExists(full_path)) {
    Log()->CreateFileEvent(full_path, path->data(), -EEXIST);
    result->set_error_code(-EEXIST);
    return Status::OK;
  }

  std::ofstream new_file(full_path, std::ios::out | std::ios::trunc);
  int ret = new_file.good() ? 0 : -1;
  int err = GetError(ret);
  result->set_error_code(err);
  Log()->CreateFileEvent(full_path, path->data(), err);
  return Status::OK;
}

// returns the file located by path to the client.
Status FileService::DownloadFile(ServerContext* ctx, const Path* path,
    File* file) {
  assert(path != nullptr && file != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  std::ifstream stream;

  // return invalid if file could not be opened.
  if (!GetIfstream(full_path, &stream)) {
    file->mutable_info()->set_error_code(-errno);
    Log()->DownloadFileEvent(full_path, path->data(), std::string(), -errno);
    return Status::OK;
  }

  // read file in chunks of up to 1024 bytes.
  static const int kMaxSize = 1024;
  bool stop = false;
  do {
    char buffer[kMaxSize];
    stream.read(buffer, kMaxSize);
    int read_size = stream.gcount();
    if (read_size > 0) {
      file->mutable_contents()->append(buffer, read_size);
    } else {
      stop = true;
    }
  } while (!stop);

  // FIXME should check that data was written.
  Log()->DownloadFileEvent(full_path, path->data(), file->contents(), 0);
  GetFileInfo(full_path, path->data(), false, file->mutable_info());
  return Status::OK;
}

// returns true if the file exists.
bool FileService::FileExists(const std::string& full_path) const {
  std::ifstream stream;
  return GetIfstream(full_path, &stream);
}

Status FileService::GetDirectoryContents(ServerContext* ctx, const Path* path,
    DirInfo* info) {
  assert(path != nullptr && info != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  DIR* dir;
  dir = opendir(full_path.c_str());
  if (dir == nullptr) {
    int err = -errno;
    Log()->GetDirectoryEvent(full_path, path->data(), err);
    info->set_error_code(err);
    return Status::OK;
  }

  dirent entry;
  dirent* entry_ptr;
  // use readdir_r to support multi-threaded operation. stop iterating
  // through entries when an error occurs (non-zero return) or end of entries
  // (entry_ptr is nullptr).
  while (readdir_r(dir, &entry, &entry_ptr) == 0 && entry_ptr != nullptr) {
    std::string* filename = info->add_contents();
    filename->append(entry.d_name);
  }
  // closedir(dir);

  int ret = entry_ptr == nullptr ? 0 : -1;
  int err = GetError(ret);
  closedir(dir);
  Log()->GetDirectoryEvent(full_path, path->data(), err);
  info->set_error_code(err);
  return Status::OK;
}

inline int FileService::GetError(int ret) const {
  if (ret == 0) { return 0; }
  return -errno;
}

// fills info with the access, creation, and modification times of full_path.
// returns true on success. this does not send a message!
bool FileService::GetFileInfo(const std::string& full_path, const std::string& path, 
    bool top_level, FileInfo* info) const {
  assert(info != nullptr);
  struct stat stat_buffer;
  int stat_result = stat(full_path.c_str(), &stat_buffer);
  
  // return that path is invalid if file cannot be stat'd.
  if (stat_result == -1) {
    info->set_error_code(-errno);
    Log()->FileInfoEvent(full_path, path, stat_buffer, -errno, top_level);
    return false;
  }

  // otherwise, return the access, mod, and creation times.
  Log()->FileInfoEvent(full_path, path, stat_buffer, 0, top_level);
  info->set_error_code(0);
  info->set_mode(stat_buffer.st_mode);
  info->set_access_time(stat_buffer.st_atime);
  info->set_creation_time(stat_buffer.st_ctime);
  info->set_modification_time(stat_buffer.st_mtime);
  info->set_size(stat_buffer.st_size);
  info->set_inode(stat_buffer.st_ino);
  return true;
}

// returns the time info of the file pointed to by path.
Status FileService::GetFileInfo(ServerContext* ctx, const Path* path,
    FileInfo* info) {
  assert(path != nullptr && info != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  GetFileInfo(full_path, path->data(), true, info);
  return Status::OK;
}

// opens the ifstream pointed to by stream to full_path and returns
// stream->good().
bool FileService::GetIfstream(const std::string& full_path, std::ifstream* stream) const {
  assert(stream != nullptr);
  stream->open(full_path, std::ios::in);
  return stream->good();
}

// opens the ofstream pointed to by stream to full_path and returns
// stream->good().
bool FileService::GetOfstream(const std::string& full_path, std::ofstream* stream) const {
  assert(stream != nullptr);
  stream->open(full_path, std::ios::out);
  return stream->good();
}

// initializes the service. particularly, ensures persistent state is up.
bool FileService::Initialize() {
  return persistence_.StartAndRecoverState();
}

// combines suffix with the mount point to obtain the full path.
std::string FileService::PromoteToFullPath(const std::string& suffix) const {
  static const char kSeparator = '/';
  if (suffix.front() == kSeparator) {
    return GetMountPoint() + suffix;
  } else {
    return GetMountPoint() + kSeparator + suffix;
  }
}

Status FileService::RemoveDirectory(ServerContext* ctx, const Path* path, 
    Result* result) {
  assert(path != nullptr && result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  int ret = rmdir(full_path.c_str());
  int err = GetError(ret);
  Log()->RemoveDirectoryEvent(full_path, path->data(), err);
  result->set_error_code(err);
  return Status::OK;
}

Status FileService::RemoveFile(ServerContext* ctx, const Path* path, 
    Result* result) {
  assert(path != nullptr && result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  int ret = std::remove(full_path.c_str());
  int err = GetError(ret);
  Log()->RemoveFileEvent(full_path, path->data(), err);
  result->set_error_code(err);
  return Status::OK;
}

// saves file to the local mount point and returns up to date time info.
Status FileService::UploadFile(ServerContext* ctx, const FileData* file,
    FileInfo* info) {
  assert(file != nullptr && info != nullptr);
  std::string full_path = PromoteToFullPath(file->path().data());
  PersistentState::UpdateToken token(full_path);

  if (!persistence_.CreateUpdateFile(full_path, &token)) {
    int err = -errno;
    Log()->UploadFileEvent(full_path, file->path().data(), file->contents(), err);
    info->set_error_code(err);
    return Status::OK;
  }

  if (crash_write_ && file->path().data() == "/crash-me") {
    int crash_size = file->contents().size() > 2048 ? 1024 : file->contents().size() / 2;
    token.GetStream()->write(file->contents().c_str(), crash_size);
    token.GetStream()->flush();
    assert(0 && "crash me detected");
  }

  token.GetStream()->write(file->contents().c_str(), file->contents().size());

  if (token.GetStream()->bad()) {
    int err = -errno;
    Log()->UploadFileEvent(full_path, file->path().data(), file->contents(), err);
    info->set_error_code(err);
    return Status::OK;
  }

  int err = persistence_.FinalizeUpdate(&token);

  Log()->UploadFileEvent(full_path, file->path().data(), file->contents(), err);
  GetFileInfo(full_path, file->path().data(), false, info);
  return Status::OK;
}
