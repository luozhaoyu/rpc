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
#include "log.h"

using namespace File;
using grpc::ServerContext;
using grpc::Status;

Status FileService::CreateDirectory(ServerContext* ctx, const Path* path,
    Result* result) {
  std::string full_path = PromoteToFullPath(path->data());
  if (mkdir(full_path.c_str(), 0755) != 0) {
    GetLog(kInfo) << "could not create dir: " << path->data() << "\n";
    result->set_error_code(-errno);
  } else {
    GetLog(kInfo) << "created dir: " << path->data() << "\n";
    result->set_error_code(0);
  }
  return Status::OK;
}

Status FileService::CreateFile(ServerContext* ctx, const Path* path,
    Result* result) {
  assert(path != nullptr);
  assert(result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());

  // cannot create if file already exists.
  if (FileExists(full_path)) {
    GetLog(kDebug) << "request to re-create file: " << path->data() << "\n";
    result->set_error_code(-errno);
    return Status::OK;
  }

  std::ofstream new_file(full_path, std::ios::out | std::ios::trunc);
  if (new_file.good()) {
    GetLog(kInfo) << "created new file: " << path->data() << "\n";
    result->set_error_code(0);
    return Status::OK;
  } else {
    GetLog(kErr) << "could not create new file: " << path->data() << "\n";
    result->set_error_code(-errno);
    return Status::OK;
  }
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
    GetLog(kDebug) << "cannot send fake file:  " << path->data() << "\n";
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

  GetLog(kInfo) << "sent file: " << path->data() << "\n";
  file->mutable_info()->set_error_code(0);
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
    GetLog(kDebug) << "cannot get info for fake dir: " << path->data() << "\n";
    info->set_error_code(-errno);
    return Status::OK;
  }

  dirent entry;
  dirent* entry_ptr;
  // use readdir_r to support multi-threaded operation. stop iterating
  // through entries when an error occurs (non-zero return) or end of entries
  // (entry_ptr is nullptr).
  while (readdir_r(dir, &entry, &entry_ptr) == 0 && entry_ptr != nullptr) {
    std::string* filename = info->add_contents();
    (GetLog(kDebug) << "writing dir entry: ") << entry.d_name << "\n";
    filename->append(entry.d_name);
  }
  closedir(dir);

  if (entry_ptr != nullptr) {
    GetLog(kErr) << "could not iterate dir: " << path->data() << "\n";
    info->set_error_code(-errno);
  } else {
    GetLog(kInfo) << "iterated dir: " << path->data() << "\n";
    info->set_error_code(0);
  }
  return Status::OK;
}

// fills info with the access, creation, and modification times of full_path.
// returns true on success. this does not send a message!
bool FileService::GetFileInfo(const std::string& full_path, FileInfo* info) const {
  assert(info != nullptr);
  struct stat stat_buffer;
  int stat_result = stat(full_path.c_str(), &stat_buffer);
  
  // return that path is invalid if file cannot be stat'd.
  if (stat_result == -1) {
    info->set_error_code(-errno);
    GetLog(kTrace) << "cannot get info for fake file: " << full_path << "\n";
    return false;
  }

  // otherwise, return the access, mod, and creation times.
  GetLog(kTrace) << "obtained info for file: " << full_path << "\n";
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
  if (!GetFileInfo(full_path, info)) {
    GetLog(kDebug) << "cannot send info for fake file: " << path->data()<< "\n";
  } else {
    GetLog(kInfo) << "sent info for file: " << path->data() << "\n";
  }
  return Status::OK;
}

// opens the ifstream pointed to by stream to full_path and returns
// stream->good().
bool FileService::GetIfstream(const std::string& full_path, std::ifstream* stream) const {
  assert(stream != nullptr);
  GetLog(kTrace) << "opening ifstream: " << full_path << "\n";
  stream->open(full_path, std::ios::in);
  return stream->good();
}

// opens the ofstream pointed to by stream to full_path and returns
// stream->good().
bool FileService::GetOfstream(const std::string& full_path, std::ofstream* stream) const {
  assert(stream != nullptr);
  GetLog(kTrace) << "opening ofstream: " << full_path << "\n";
  stream->open(full_path, std::ios::out);
  return stream->good();
}

// combines suffix with the mount point to obtain the full path.
std::string FileService::PromoteToFullPath(const std::string& suffix) const {
  static const char kSeparator = '/';
  if (GetMountPoint().back() == kSeparator) {
    return GetMountPoint() + suffix;
  } else {
    return GetMountPoint() + kSeparator + suffix;
  }
}

Status FileService::RemoveDirectory(ServerContext* ctx, const Path* path, 
    Result* result) {
  assert(path != nullptr && result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  if (rmdir(full_path.c_str()) != 0) {
    GetLog(kInfo) << "could not remove dir: " << path->data() << "\n";
    result->set_error_code(-errno);
  } else {
    GetLog(kInfo) << "removed dir: " << path->data() << "\n";
    result->set_error_code(0);
  }
  return Status::OK;
}

Status FileService::RemoveFile(ServerContext* ctx, const Path* path, 
    Result* result) {
  assert(path != nullptr && result != nullptr);
  std::string full_path = PromoteToFullPath(path->data());
  if (std::remove(full_path.c_str()) != 0) {
    GetLog(kInfo) << "could not remove file: " << path->data() << "\n";
    result->set_error_code(-errno);
    return Status::OK;
  }

  GetLog(kInfo) << "removed file: " << path->data() << "\n";
  result->set_error_code(0);
  return Status::OK;
}

// saves file to the local mount point and returns up to date time info.
Status FileService::UploadFile(ServerContext* ctx, const FileData* file,
    FileInfo* info) {
  assert(file != nullptr && info != nullptr);
  std::string full_path = PromoteToFullPath(file->path().data());
  std::fstream stream(full_path, std::ios::out | std::ios::trunc);

  if (stream.bad()) {
    GetLog(kErr) << "could not put file: " << file->path().data() << "\n";
    info->set_error_code(-errno);
    return Status::OK;
  }

  stream.write(file->contents().c_str(), file->contents().size());

  if (stream.bad()) {
    GetLog(kErr) << "error writing file: " << file->path().data() << "\n";
    info->set_error_code(-errno);
    return Status::OK;
  }

  GetFileInfo(full_path, info);
  GetLog(kInfo) << "put file: " << file->path().data() << "\n";
  return Status::OK;
}
