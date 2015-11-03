// event_log.h
// by: allison morris

#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <iostream>
#include <mutex>

struct stat;

namespace File {

enum LogLevel { kFatal, kError, kInfo, kDebug, kTrace };

class EventLog {
public:
  EventLog(std::ostream& out, LogLevel lvl, bool dump) : out_(out), level_(lvl),
    dump_files_(dump) { }

  void CreateDirectoryEvent(const std::string& full_path, const std::string& path, int err);
  void CreateFileEvent(const std::string& full_path, const std::string& path, int err);
  // create file exists?
  void DownloadFileEvent(const std::string& full_path, const std::string& path,
    const std::string& contents, int err);
  // file exists?
  void FileInfoEvent(const std::string& full_path, const std::string& path,
    struct stat& info, int err, bool top_level);
  void GetDirectoryEvent(const std::string& full_path, const std::string& path, int err);
  
  static EventLog* GetLog() { return logger_; }

  static void Initialize(std::ostream& out, LogLevel lvl, bool dump) {
    logger_ = new EventLog(out, lvl, dump);
  }

  void PersistentDirectoryEvent(const std::string& path, bool exists, int err);
   
  void PersistentStartEvent(bool old_log, bool bad_entry, bool log_good);

  void RemoveDirectoryEvent(const std::string& full_path, const std::string& path, int err);
  void RemoveFileEvent(const std::string& full_path, const std::string& path, int err);
  void StartupEvent(const std::string& mount_point, const std::string& address);

  static bool ToVerbosity(int v, LogLevel* lvl) {
    switch (v) {
      case 0: *lvl = kFatal; return true;
      case 1: *lvl = kError; return true;
      case 2: *lvl = kInfo; return true;
      case 3: *lvl = kDebug; return true;
      case 4: *lvl = kTrace; return true;
      default: return false;
    }
  }

  void UploadFileEvent(const std::string& full_path, const std::string& path,
    const std::string& contents, int err);
private:
  class Lock : public std::lock_guard<std::mutex> {
  public:
    Lock() : std::lock_guard<std::mutex>(mutex_) { }
  };

  void DumpFile(const std::string& contents);

  void HandleBadErrors(const std::string& cmd, const std::string& full_path,
    const std::string& path, int err);

  void HandleGoodErrors(const std::string& cmd, const std::string& full_path,
    const std::string& path, int err);

  static EventLog* logger_;
  static std::mutex mutex_;
  std::ostream& out_;
  LogLevel level_;
  bool dump_files_;
};

inline EventLog* Log() { return EventLog::GetLog(); }

}

#endif
