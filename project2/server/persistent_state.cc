// persistent_state.cc
// by: allison morris

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <set>
#include <sys/stat.h>
#include "event_log.h"
#include "persistent_state.h"

using namespace File;

std::mutex PersistentState::mutex_;

bool PersistentState::CreatePersistentPath(UpdateToken* token) {
  Lock lock;
  token->SetPersistentPath(root_dir_ + std::to_string(next_id_));
  ++next_id_;
  return true;
}

bool PersistentState::CreateUpdateFile(const std::string& full_path, UpdateToken* token) {
  if (!CreatePersistentPath(token)) {
    return false;
  }

  token->GetStream()->open(token->GetPersistentPath(), std::ios::out);
  if (!token->GetStream()->good()) {
    std::cerr << "WTF CreateUpdateFile fuqqqqq " << token->GetPersistentPath() << "\n";
    return false;
  }

  Lock lock;
  store_ << "START " << token->GetPersistentPath() << std::endl; // includes flush.
  return true;
}

std::istream& operator>>(std::istream& is, PersistentState::Transaction& t) {
  std::string line;
  std::getline(is, line);

  if (line.find("START ") == 0) { 
    t.type = PersistentState::kStart;
    t.SetIdFromPath(std::move(line.substr(6))); // path after "START "
    t.good = true;
    return is;
  } else if (line.find("WRITE ") == 0) {
    t.type = PersistentState::kWrite;
    size_t first_delim = line.find(" /// ");
    // std::cout << "first delim at " << first_delim << " for " << line << "\n";
    if (first_delim == std::string::npos) {
      t.good = false;
      return is;
    }
    t.SetIdFromPath(line.substr(6, first_delim - 6));
    size_t second_delim = line.find(" /// ", first_delim + 5);
    if (second_delim == std::string::npos) {
      t.good = false;
      return is;
    }
    t.target_path = line.substr(first_delim + 5, second_delim - first_delim - 5);
    t.good = true;
    return is;
  } else {
    t.good = false;
    return is;
  }
}

int PersistentState::FinalizeUpdate(UpdateToken* token) {
  token->GetStream()->close();
  struct stat st_buffer;
  int err = stat(token->GetPersistentPath().c_str(), &st_buffer);
  if (err != 0) {
    // std::cerr << "WTF no log for " << token->GetPersistentPath() << " " << -errno << "\n";
    return -errno;
  }

  Lock lock;
  err = std::rename(token->GetPersistentPath().c_str(), token->GetTargetPath().c_str());
  if (err != 0) { err = -errno; }
  store_ << "WRITE " << token->GetPersistentPath() << " /// " << token->GetTargetPath()
    << " /// " << st_buffer.st_size << std::endl; // includes flush.
  return err;
}

void PersistentState::Transaction::SetIdFromPath(std::string path) {
  persistent_path = std::move(path);
  size_t base = persistent_path.find_last_of('/');
  if (base == std::string::npos) { base = 0; }
  else { ++base; }
  std::string base_str = persistent_path.substr(base);
  // std::cout << "get id from " << base_str << " which is part of " << persistent_path << "\n";
  id = std::stoi(base_str);
}

// reads the log, fixes up any file transactions that have not been completed,
// closes and re-opens the log for writing. returns true if start-up has
// completed successfully. 
bool PersistentState::StartAndRecoverState() {
  // check persistent directory exists, or attempt to create it...
  struct stat st_buf;
  int dir_exists = stat(root_dir_.c_str(), &st_buf);
  if (dir_exists != 0 || !S_ISDIR(st_buf.st_mode)) {
    dir_exists = mkdir(root_dir_.c_str(), 0777);
  }
  Log()->PersistentDirectoryEvent(root_dir_, dir_exists == 0, -errno);
  if (dir_exists != 0) {return false; }

  std::ifstream last_log(store_name_);
  if (last_log.bad()) {
    last_log.close();
    store_.open(store_name_, std::ios::out | std::ios::trunc);
    Log()->PersistentStartEvent(false, true, store_.good());
    return store_.good();
  }

  std::set<Transaction> started_transactions;
  Transaction transaction;
  bool bad_entry = false;
  while (last_log >> transaction) {
    if (!transaction.good) {
      bad_entry = true;
      continue;
    }

    if (transaction.type == PersistentState::kStart) {
      started_transactions.insert(transaction);
    } else {
      auto iter = started_transactions.find(transaction);
      struct stat st_buf;
      int ret = stat(transaction.persistent_path.c_str(), &st_buf);
      if (ret != 0) {
        started_transactions.erase(iter);
	continue;
      }
      int err = std::rename(transaction.persistent_path.c_str(),
        transaction.target_path.c_str());
      ret = stat(transaction.persistent_path.c_str(), &st_buf);

      // if persistent file still exists and an error occurred, abort.
      // else something weird happened.
      if (ret == 0 && err == 0) { assert(0 && "rename not finished before stat."); }
      started_transactions.erase(iter);
    }
  }

  // remove any incomplete transactions that do not have corresponding writes.
  for (const Transaction& trans : started_transactions) {
    std::remove(trans.persistent_path.c_str());
  }
  last_log.close();
  store_.open(store_name_, std::ios::out | std::ios::trunc);
  Log()->PersistentStartEvent(true, bad_entry, store_.good());
  return store_.good();
}
