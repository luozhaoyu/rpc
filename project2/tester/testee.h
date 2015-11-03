// testee.h
// by: allison morris

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

class Testee {
public:
  class Args {
  public:
    enum SplitType { kThreads, kTrials };
    
    Args() : threads_(1), trials_(1), size_(8) { }

    Args(const std::string& name, int thr, int tr, int sz)
      : filename_(name), threads_(thr), trials_(tr), size_(sz) { }

    const std::string& GetFilename() const { return filename_; }
    int GetSize() const { return size_; }
    int GetThreads() const { return threads_; }
    int GetTrials() const { return trials_; }

    virtual SplitType GetSplitType() const {
      return GetThreads() > 1 ? kThreads : kTrials;
    }
   
    void SetSize(int sz) { size_ = sz; }
    void SetThreads(int thr) { threads_ = thr; }
    void SetTrials(int tr) { trials_ = tr; }
  private:
    std::string filename_;
    int threads_;
    int trials_;
    int size_;
  };

  // expands filename to thread-unique versions. thread i has -ti appended to filename.
  std::string GetFilename(const Args& args, int id) const {
    std::string ret = args.GetFilename();
    ret += "-t";
    ret += std::to_string(id);
    return ret;
  }

  virtual Args* Parse(int argc, const char** argv) = 0;

  virtual int Run(const Args& args, int id) = 0;
};

class BasicAccessTest : public Testee {
public:
  class BasicAccessArgs : public Args {
  public:
    BasicAccessArgs(const char* name, int thr, int tr) : Args(name, thr, tr, 0) { }
  private:
  };


  int Run(const Args& args, int id) {
    const BasicAccessArgs& access_args = *(const BasicAccessArgs*)&args;
    std::string filename = GetFilename(args, id);
    std::ifstream stream(filename);
    return 0;
  }
};

class BasicReadWriteTest : public Testee {
public:
  class BasicReadWriteArgs : public Args {
  public:
    BasicReadWriteArgs(const char* name, char op, int thr, int tr) 
      : Args(name, thr, tr, 0), operation_(op) { }

    char GetOperation() const { return operation_; }
  private:
    char operation_;
  };


  int Run(const Args& args, int id) {
    const BasicReadWriteArgs& rw_args = *(const BasicReadWriteArgs*)&args;
    std::string filename = GetFilename(args, id);
    if (rw_args.GetOperation() == 'r') {
      struct stat st_buf;
      stat(filename.c_str(), &st_buf); // for symmetry with write.
      std::ifstream stream(filename);
      char buffer[4096];
      int count = 0;

      while (stream.good()) {
        stream.read(buffer, 4096);
	count += stream.gcount();
      }

      //std::cout << "Read " << count << " from " << rw_args.GetFilename() << "\n";
    } else {
      struct stat st_buf;
      stat(filename.c_str(), &st_buf);
      std::ofstream stream(filename);
      char buffer[4096];
      int total = st_buf.st_size;
      int count = 0;

      while (stream.good()) {
        int amount = total - count > 4096 ? 4096 : total - count;
	if (amount == 0) { break; }
        stream.write(buffer, amount);
	count += amount;
      }

      //std::cout << "Wrote " << count << " from " << rw_args.GetFilename() << " of size "
      //  << st_buf.st_size << "\n";
    }
    return 0;
  }
};

class MultiAccessTest : public BasicAccessTest {
public:
  Args* Parse(int argc, const char** argv) {
    int threads = 1;
    if (argc >= 3) {
      threads = strtol(argv[2], nullptr, 10);
    }
    if (argc >= 2) {
      return new BasicAccessArgs(argv[1], threads, 1);
    } else {
      return nullptr;
    }
  }
};


class MultiReadWriteTest : public BasicReadWriteTest {
public:
  Args* Parse(int argc, const char** argv) {
    int threads = 1;
    char mode = 'r';
    if (argc >= 4) {
      threads = strtol(argv[3], nullptr, 10);
    }
    if (argc >= 3) {
      if ((argv[2][0] == 'r' || argv[2][0] == 'w') && argv[2][1] == 0) {
        mode = argv[2][0];
      }
    }
    if (argc >= 2) {
      return new BasicReadWriteArgs(argv[1], mode, threads, 1);
    } else {
      return nullptr;
    }
  }

};

class SingleAccessTest : public BasicAccessTest {
public:
  Args* Parse(int argc, const char** argv) {
    int trials = 1;
    if (argc >= 3) {
      trials = strtol(argv[2], nullptr, 10);
    }
    if (argc >= 2) {
      return new BasicAccessArgs(argv[1], 1, trials);
    } else {
      return nullptr;
    }
  }
};

class SingleReadWriteTest : public BasicReadWriteTest {
public:
  Args* Parse(int argc, const char** argv) {
    int trials = 1;
    char mode = 'r';
    if (argc >= 4) {
      trials = strtol(argv[3], nullptr, 10);
    }
    if (argc >= 3) {
      if ((argv[2][0] == 'r' || argv[2][0] == 'w') && argv[2][1] == 0) {
        mode = argv[2][0];
      }
    }
    if (argc >= 2) {
      return new BasicReadWriteArgs(argv[1], mode, 1, trials);
    } else {
      return nullptr;
    }
  }
};
