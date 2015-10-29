// basic_client.cc : implements a simple client shell for testing rpc calls.
// by: allison morris

#include <fstream>
#include <iostream>
#include "file_service.h"

using namespace File;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// provides an interface to the File service and some convenience functions.
class FileStub {
public:
  FileStub(std::shared_ptr<Channel> channel) 
    : rpc_(BasicFileService::NewStub(channel)) { }

  bool CreateDirectory(const std::string& path) {
    Path request;
    Result response;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->CreateDirectory(&ctx, request, &response);

    if (!status.ok()) {
      std::cout << "RPC failed for CreateDirectory\n";
      return false;
    }

    return response.error_code() == 0;
  }

  bool CreateFile(const std::string& path) {
    Path request;
    Result response;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->CreateFile(&ctx, request, &response);

    if (!status.ok()) {
      std::cout << "RPC failed for CreateFile\n";
      return false;
    }

    return response.error_code() == 0;
  }

  bool DownloadFile(const std::string& path, std::ostream* dest) {
    Path request;
    File::File reply;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->DownloadFile(&ctx, request, &reply);
    
    if (!status.ok()) {
      std::cout << "RPC failed for DownloadFile\n";
      return false;
    }

    if (reply.info().error_code() != 0) { return false; }

    dest->write(reply.contents().c_str(), reply.contents().size());
    return true;
  }

  // gets the contents of directory path and stores them in the store referenced by i.
  // Iterator supports dereferenced-write operations of type std::string.
  template <class Iterator> bool GetDirectoryContents(const std::string& path, Iterator i) {
    Path request;
    DirInfo response;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->GetDirectoryContents(&ctx, request, &response);

    if (!status.ok()) {
      std::cout << "RPC for GetDirectoryContents failed\n";
      return false;
    }

    if (response.error_code() != 0) { return false; }
    
    for (int j = 0; j < response.contents_size(); ++j) {
      *i = response.contents(j);
      ++i;
    }
    return true;
  }

  bool GetFileInfo(const std::string& path, long* mod_time) {
    Path request;
    FileInfo reply;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->GetFileInfo(&ctx, request, &reply);

    if (!status.ok()) {
      std::cout << "RPC failed for GetFileInfo\n";
      return false;
    }

    if (reply.error_code() != 0) { return false; }

    *mod_time = reply.modification_time();
    return true;
  }

  bool RemoveDirectory(const std::string& path) {
    Path request;
    Result response;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->RemoveDirectory(&ctx, request, &response);

    if (!status.ok()) {
      std::cout << "RPC failed for RemoveDirectory\n";
      return false;
    }

    return response.error_code() == 0;
  }

  bool RemoveFile(const std::string& path) {
    Path request;
    Result response;
    ClientContext ctx;
    request.set_data(path);
    Status status = rpc_->RemoveFile(&ctx, request, &response);

    if (!status.ok()) {
      std::cout << "RPC failed for RemoveFile\n";
      return false;
    }

    return response.error_code() == 0;
  }

  bool UploadFile(const std::string& path, std::istream& src) {
    FileData request;
    FileInfo reply;
    ClientContext ctx;
    request.mutable_path()->set_data(path);

    static const int kMaxSize = 1024;
    bool stop = false;
    do {
      char buffer[kMaxSize];
      src.read(buffer, kMaxSize);
      int read_size = src.gcount();

      if (read_size > 0) {
	request.mutable_contents()->append(buffer, read_size);
      } else {
        stop = true;
      }
    } while (!stop);

    Status status = rpc_->UploadFile(&ctx, request, &reply);

    if (!status.ok()) {
      std::cout << "RPC failed for UploadFile\n";
      return false;
    }

    return reply.error_code() == 0;
  }
private:
  std::unique_ptr<BasicFileService::Stub> rpc_;
};

int main(int argc, const char** argv) {
  FileStub stub(grpc::CreateChannel("localhost:61512", 
    grpc::InsecureCredentials()));

  std::string cmd;
  while (std::getline(std::cin, cmd)) {
    size_t space = cmd.find_first_of(' ');
    if (space == std::string::npos) {
      std::cout << "Error, command must be formatted: cmd arg\n";
      continue;
    }

    std::string cmd_name = cmd.substr(0, space);
    std::string cmd_arg = cmd.substr(space + 1);

    if (cmd_arg.empty()) {
      std::cout << "Error, argument must have value\n";
      continue;
    }

    if (cmd_name == "get") {
      std::fstream stream(cmd_arg, std::ios::out);
      if (!stub.DownloadFile(cmd_arg, &stream)) {
        std::cout << "Could not download file: " << cmd_arg << "\n";
        continue;
      }
      std::cout << "Downloaded " << cmd_arg << "\n";
      continue;
    }

    if (cmd_name == "info") {
      long modification_time;
      if (!stub.GetFileInfo(cmd_arg, &modification_time)) {
        std::cout << "Could not get file info: " << cmd_arg << "\n";
	continue;
      }
      std::cout << cmd_arg << " was last modified at " << modification_time
        << "\n";
      continue;
    }

    if (cmd_name == "ls") {
      std::list<std::string> contents;
      if (!stub.GetDirectoryContents(cmd_arg, std::back_inserter(contents))) {
        std::cout << "could not get contents of directory: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "contents of directory: " << cmd_arg << "\n";
      for (const std::string& entry : contents) {
        std::cout << "    " << entry << "\n";
      }
      continue;
    }

    if (cmd_name == "mkdir") {
      if (!stub.CreateDirectory(cmd_arg)) {
        std::cout << "could not create directory: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "created directory " << cmd_arg << "\n";
      continue;
    }
    
    if (cmd_name == "mknod") {
      if (!stub.CreateFile(cmd_arg)) {
        std::cout << "could not create file: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "created file " << cmd_arg << "\n";
      continue;
    }

    if (cmd_name == "put") {
      std::fstream stream(cmd_arg, std::ios::in);
      if (!stub.UploadFile(cmd_arg, stream)) {
        std::cout << "Could not upload file: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "uploaded " << cmd_arg << "\n";
      continue;
    }

    if (cmd_name == "rm") {
      if (!stub.RemoveFile(cmd_arg)) {
        std::cout << "could not remove file: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "removed file " << cmd_arg << "\n";
      continue;
    }

    if (cmd_name == "rmdir") {
      if (!stub.RemoveDirectory(cmd_arg)) {
        std::cout << "could not remove directory: " << cmd_arg << "\n";
	continue;
      }
      std::cout << "removed directory " << cmd_arg << "\n";
      continue;
    }

    if (cmd_name == "exit") { break; }
    std::cout << "Error, commands are get, info, put, and exit.\n";
  }
  return 0;  
}
