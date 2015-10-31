// persistent_state.cc
// by: allison morris

#include <cassert>
#include <cstdio>
#include <set>
#include <sys/stat.h>
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

  token->GetStream()->open(token->GetPersistentPath(), std::ios::in);
  if (token->GetStream()->bad()) {
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
    if (first_delim == std::string::npos) {
      t.good = false;
      return is;
    }
    t.SetIdFromPath(line.substr(6, first_delim));
    size_t second_delim = line.find(" /// ", first_delim + 5);
    if (second_delim == std::string::npos) {
      t.good = false;
      return is;
    }
    t.target_path = line.substr(first_delim + 5, second_delim);
    t.good = true;
    return is;
  } else {
    t.good = false;
    return is;
  }
}

bool PersistentState::FinalizeUpdate(UpdateToken* token) {
  token->GetStream()->close();
  struct stat st_buffer;
  int err = stat(token->GetPersistentPath().c_str(), &st_buffer);
  if (err != 0) {
    return false;
  }

  Lock lock;
  err = std::rename(token->GetPersistentPath().c_str(), token->GetTargetPath().c_str());
  
  store_ << "WRITE " << token->GetPersistentPath() << " /// " << token->GetTargetPath()
    << " /// " << st_buffer.st_size << std::endl; // includes flush.

  return err == 0;
}

void PersistentState::Transaction::SetIdFromPath(std::string path) {
  persistent_path = std::move(path);
  size_t base = persistent_path.find_last_of('/');
  if (base == std::string::npos) { base = 0; }
  else { ++base; }
  std::string base_str = persistent_path.substr(base);
  id = std::stoi(base_str);
}

// reads the log, fixes up any file transactions that have not been completed,
// closes and re-opens the log for writing. returns true if log parsing
// is performed successfully, or if there is no log.
bool PersistentState::StartAndRecoverState() {
  std::ifstream last_log(store_name_);
  if (last_log.bad()) {
    last_log.close();
    store_.open(store_name_, std::ios::out | std::ios::trunc);
    return true;
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
      std::rename(transaction.persistent_path.c_str(),
        transaction.target_path.c_str());
      ret = stat(transaction.persistent_path.c_str(), &st_buf);
      if (ret == 0) { assert(0 && "rename not finished before stat."); }
      started_transactions.erase(iter);
    }
  }

  // remove any incomplete transactions that do not have corresponding writes.
  for (const Transaction& trans : started_transactions) {
    std::remove(trans.persistent_path.c_str());
  }
  last_log.close();
  store_.open(store_name_, std::ios::out | std::ios::trunc);
  return bad_entry;
}
