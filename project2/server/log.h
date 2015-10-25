// log.h : Decalres a logger for file.
// by: allison morris

#include <fstream>
#include <iostream>

namespace File {

// levels for logging output.
enum LogLevel { kErr, kInfo, kDebug, kTrace };

class Logger {
public:
  // this class is used to represent stream for each log level.
  class Channel {
  public:
    Channel(std::ostream& stream) : stream_(stream) { }

    std::ostream& Get() { return stream_; }
  private:
    std::ostream& stream_;
  };

  static const LogLevel kDefaultLevel = kDebug;

  Logger(std::ostream& stream, std::ostream& disabled, LogLevel level) : 
    stream_(stream), disabled_(disabled), level_(level) { }

  static Logger& GetInstance() {
    return *instance_;
  }

  static void Initialize(std::ostream& stream, std::ostream& disabled, 
      LogLevel level) {
    instance_ = new Logger(stream, disabled, level);
  }

  // returns a channel representing the log level level.
  Channel Select(LogLevel level) {
    if (level_ >= level) {
      return Channel(stream_);
    } else {
      return Channel(disabled_);
    }
  }

  void SetLevel(LogLevel l) { level_ = l; }

private:
  static Logger* instance_;
  std::ostream& stream_;
  std::ostream& disabled_;
  LogLevel level_;
};

// returns the stream for the log level level.
inline std::ostream& GetLog(LogLevel level) {
  return Logger::GetInstance().Select(level).Get();
}

// returns the stream for the default level.
inline std::ostream& GetLog() {
  return Logger::GetInstance().Select(Logger::kDefaultLevel).Get();
}

}
