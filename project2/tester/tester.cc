// tester.cc
// by: allison morris


#include "testee.h"
#include <map>
#include <time.h>
#include <thread>
#include <vector>

class Tester {
public:
  struct Result {
    long time;
    int error;
  };

  class TimeVector : private std::vector<Result> {
  public:
    typedef std::vector<Result> Vector;
    typedef Testee::Args::SplitType SplitType;

    long GetAvg() const {
      long sum = 0;
      for (auto result : *this) {
        sum += result.time;
      }
      return sum / size();
    }

    long GetMax() const {
      long max = 0;
      for (auto result : *this) {
        max = (result.time > max) ? result.time : max;
      }
      return max;
    }
    
    long GetMin() const {
      long min = 0xffffffff;
      for (auto result : *this) {
        min = (result.time < min) ? result.time : min;
      }
      return min;
    }

    Result* NewResult() {
      push_back(Result());
      return &back();
    }

    using Vector::push_back;
    using Vector::size;
  };
  
  static void Execute(const Testee::Args& args, Testee* testee, int id, Result* result) {
    long time = GetTime();
    int err = testee->Run(args, id);
    time = GetTime() - time;
    result->time = time;
    result->error = err;
  }

  int ExpandThreads(const Testee::Args& args, Testee* testee, TimeVector* out) {
    std::vector<std::thread> threads;
    for (int i = 0; i < args.GetThreads(); ++i) {
      threads.push_back(std::thread(Execute, std::ref(args), testee, i, out->NewResult()));
    }
    for (int i = 0; i < args.GetThreads(); ++i) {
      threads[i].join();
    }
    return 0;
  }

  int ExpandTrials(const Testee::Args& args, Testee* testee, TimeVector* out) {
    for (int i = 0; i < args.GetTrials(); ++i) {
      Execute(args, testee, i, out->NewResult());
    }
    return 0;
  }

  Testee* FindTestee(const std::string& name) {
    return testees_[name];
  }

  static long GetTime() {
    timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_nsec + ((long)time.tv_sec * 1000000000);
  }

  void Initialize() {
    testees_["MultiAccess"] = new MultiAccessTest();
    testees_["MultiReadWrite"] = new MultiReadWriteTest();
    testees_["SingleAccess"] = new SingleAccessTest();
    testees_["SingleReadWrite"] = new SingleReadWriteTest();
  }

  void Run(const Testee::Args& args, const std::string& name, TimeVector* out) {
    Testee* testee = FindTestee(name);
    if (args.GetSplitType() == Testee::Args::kThreads) {
      ExpandThreads(args, testee, out);
    } else {
      ExpandTrials(args, testee, out);
    }
  }
private:
  std::map<std::string, Testee*> testees_;
};

int main(int argc, const char** argv) {
  Tester tester;
  tester.Initialize();

  if (argc < 2) {
    std::cout << "need test name followed by args, if any\n";
    return 1;
  }

  std::string name = argv[1];
  Testee* testee = tester.FindTestee(name);
  Testee::Args* args = testee->Parse(argc - 1, argv + 1);
  Tester::TimeVector results;

  tester.Run(*args, name, &results);

  std::cout << "Completed " << args->GetTrials() << " trials across " << args->GetThreads()
    << " threads for test ";
  for (int i = 1; i < argc; ++i) {
    std::cout << argv[i] << " ";
  }
  std::cout << std::endl
    << "  Avg time: " << results.GetAvg() << std::endl
    << "  Max time: " << results.GetMax() << std::endl
    << "  Min time: " << results.GetMin() << std::endl;
  return 0;
}
