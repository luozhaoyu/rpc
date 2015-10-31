// persistent_state.h: provides utilities for crash resilience.
// by: allison morris

#ifndef PERSISTENT_STATE_H
#define PERSISTENT_STATE_H

#include <fstream>
#include <mutex>

namespace File {

class PersistentState {
public:
  enum TransactionType { kStart, kWrite };
  
  struct Transaction {
    std::string persistent_path;
    std::string target_path;
    TransactionType type;
    int id;
    bool good;
    
    bool operator<(const Transaction& t2) const {
      return id < t2.id;
    }

    void SetIdFromPath(std::string path);
  };
  
  class UpdateToken {
  public:
    UpdateToken(const std::string& fp) : target_path_(fp) { }

    const std::string& GetPersistentPath() const { return persistent_path_; }

    std::ofstream* GetStream() { return &stream_; }

    const std::string& GetTargetPath() const { return target_path_; }
  
    void SetPersistentPath(std::string str) {
      persistent_path_ = std::move(str);
    }
  private:

    const std::string& target_path_;
    std::ofstream stream_;
    std::string persistent_path_;
  };

  PersistentState(const std::string& root, const std::string& store_path) : 
    root_dir_(root), store_name_(store_path), 
    store_(store_path, std::ios::out | std::ios::trunc), next_id_(0) { }

  bool CreatePersistentPath(UpdateToken* token);
  
  bool CreateUpdateFile(const std::string& full_path, UpdateToken* token);

  bool FinalizeUpdate(UpdateToken* token);

  bool StartAndRecoverState();
private:

  class Lock : public std::lock_guard<std::mutex> {
  public:
    Lock() : std::lock_guard<std::mutex>(mutex_) { }
  };

  static std::mutex mutex_;
  std::string root_dir_;
  std::string store_name_;
  std::ofstream store_;
  int next_id_;
};

}

#endif
