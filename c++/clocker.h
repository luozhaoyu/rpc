// clocker.h : clock class for encapsulating timing measurements.
// by: allison morris

#ifndef CLOCKER_H
#define CLOCKER_H

#include <time.h>
#include <sys/time.h>
#include <iostream>

class clocker {
public:
  enum mode { clock_gettime, gettimeofday };

  clocker() : _mode(clock_gettime) { _cgt_begin.tv_sec = 0; _cgt_begin.tv_nsec = 0; _cgt_end.tv_sec = 0; _cgt_end.tv_nsec = 0; }
  clocker(mode m) : _mode(m) { }

  inline void begin() {
    if (_mode == clock_gettime) {
        ::clock_gettime(CLOCK_MONOTONIC, &_cgt_begin);
      } else {
        ::gettimeofday(&_gtd_begin, nullptr);
      }
  }

  inline long difference() const {
    if (_mode == clock_gettime) {
     // std::cout << "timing check " << (cgt_end_ns() > cgt_begin_ns()) << "\n";
      return cgt_end_ns() - cgt_begin_ns();
    } else {
      return gtd_end_ns() - gtd_begin_ns();
    }
  }

  inline void dump_cgt() const {
    std::cout << "end " << cgt_end_ns() << "\nbeg " << cgt_begin_ns() 
      << "\ndif " << difference() << "\n";
  }

  inline void end() {
    if (_mode == clock_gettime) {
        ::clock_gettime(CLOCK_MONOTONIC, &_cgt_end);
      } else {
        ::gettimeofday(&_gtd_end, nullptr);
      }
  }

  inline void swap() {
    if (_mode == clock_gettime) { _mode = gettimeofday; }
    else { _mode = clock_gettime; }
  }

private:

  inline long cgt_begin_ns() const {
    return (long)_cgt_begin.tv_sec * 1000000000 + (long)_cgt_begin.tv_nsec;
  }

  inline long cgt_end_ns() const {
    return (long)_cgt_end.tv_sec * 1000000000 + (long)_cgt_end.tv_nsec;
  }

  inline long gtd_begin_ns() const {
    return ((long)_gtd_begin.tv_sec * 1000000 + (long)_gtd_begin.tv_usec) * 1000;
  }

  inline long gtd_end_ns() const {
    return ((long)_gtd_end.tv_sec * 1000000 + (long)_gtd_end.tv_usec) * 1000;
  }

  timeval _gtd_begin, _gtd_end;
  timespec _cgt_begin, _cgt_end;
  mode _mode;
};


#endif
