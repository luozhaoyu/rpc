// event_log.cc
// by: allison morris

#include <cerrno>
#include <sys/stat.h>
#include "event_log.h"

using namespace File;

EventLog* EventLog::logger_;
std::mutex EventLog::mutex_;

void EventLog::CreateDirectoryEvent(const std::string& full_path, const std::string& path,
    int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK CreateDirectory " << path;
      if (level_ >= kDebug) { out_ << " (" << full_path << ")"; }
      out_ << "\n";
    } else {
      HandleGoodErrors("CreateDirectory", full_path, path, err);
    }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("CreateDirectory", full_path, path, err);
  }
}

void EventLog::CreateFileEvent(const std::string& full_path, const std::string& path,
    int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK CreateFile " << path;
      if (level_ >= kDebug) { out_ << " (" << full_path << ")"; }
      out_ << "\n";
    } else {
      HandleGoodErrors("CreateFile", full_path, path, err);
    }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("CreateFile", full_path, path, err);
  }
}

void EventLog::DownloadFileEvent(const std::string& full_path, const std::string& path,
    const std::string& contents, int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK DownloadFile " << path << " " << contents.size() << " bytes";
      if (level_ >= kDebug) {
        out_ << " (" << full_path << ")";
	DumpFile(contents);
      }
      out_ << "\n";
    } else { HandleGoodErrors("DownloadFile", full_path, path, err); }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("DownloadFile", full_path, path, err);
  }
}

void EventLog::DumpFile(const std::string& contents) {
  if (dump_files_) {
    out_ << "\n   data:";
    auto byte = contents.cbegin(), stop = contents.cend();
    bool binary_abort = false;
    while (byte != stop && !binary_abort) {
      out_ << "\n      ";
      for (int i = 0; i < 70; ++i) {
        if (byte == stop) { break; }

	if (*byte == '\n') {
	  ++byte;
	  break;
	}
        if (*byte == '\t') {
	  out_ << ' ';
	  ++byte;
	  continue;
	}

	if (std::isprint(*byte)) {
	  out_ << *byte;
	} else {
	  binary_abort = true;
	  out_ << "\n   [binary data detected]";
	  break;
	}

	++byte;
      }
    }
    out_ << "\n   [end]\n";
  }
}

void EventLog::FileInfoEvent(const std::string& full_path, const std::string& path, 
    struct stat& info, int err, bool top_level) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << (top_level ? "OK GetFileInfo " : "   info: ") << path;
      if (level_ >= kDebug) {
        out_ << " (" << full_path << ")\n"
	  << "      inode: " << info.st_ino << "\n"
          << "      mode:  " << std::oct << info.st_mode << std::dec << "\n"
	  << "      size:  " << info.st_size << "\n"
	  << "      access time:       " << info.st_atime << "\n"
	  << "      modification time: " << info.st_mtime << "\n"
	  << "      creation time:     " << info.st_ctime;
      }
      out_ << "\n";
    } else { HandleGoodErrors("GetFileInfo", full_path, path, err); }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("GetFileInfo", full_path, path, err);
  }
}

void EventLog::GetDirectoryEvent(const std::string& full_path, const std::string& path, int err) { 
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK GetDirectoryContents " << path;
      if (level_ >= kDebug) {
        out_ << " (" << full_path << ")";
	// TODO list contents.
      }
      out_ << "\n";
    } else { HandleGoodErrors("GetDirectoryContents", full_path, path, err); }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("GetDirectoryContents", full_path, path, err);
  }
}

void EventLog::HandleBadErrors(const std::string& cmd, const std::string& full_path,
    const std::string& path, int err) {
  if (err != -ENOENT && err != -ENOTDIR) {
    out_ << "ERR " << cmd << " " << path;
    if (level_ >= kDebug) {
      out_ << " (" << full_path << ") error code: " << err;
    }
    out_ << "\n";
  }
}

void EventLog::HandleGoodErrors(const std::string& cmd, const std::string& full_path,
    const std::string& path, int err) {
  if (err == -ENOENT) {
    out_ << "USR " << cmd << " @enoent " << path;
    if (level_ >= kDebug) {
      out_ << " (" << full_path << ")";
    }
    out_ << "\n";
  } else if (err == -ENOTDIR) {
    out_ << "USR " << cmd << " @enotdir " << path;
    if (level_ >= kDebug) {
      out_ << " (" << full_path << ")";
    }
    out_ << "\n";
  } else if (err == -EEXIST) {
    out_ << "USR " << cmd << " @eexist " << path;
    if (level_ >= kDebug) {
      out_ << " (" << full_path << ")";
    }
    out_ << "\n";
  }
}

void EventLog::RemoveDirectoryEvent(const std::string& full_path, const std::string& path,int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK RemoveDirectory " << path;
      if (level_ >= kDebug) { out_ << " (" << full_path << ")"; }
      out_ << "\n";
    } else {
      HandleGoodErrors("RemoveDirectory", full_path, path, err);
    }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("RemoveDirectory", full_path, path, err);
  }
}

void EventLog::RemoveFileEvent(const std::string& full_path, const std::string& path,
    int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK RemoveFile " << path;
      if (level_ >= kDebug) { out_ << " (" << full_path << ")"; }
      out_ << "\n";
    } else {
      HandleGoodErrors("RemoveFile", full_path, path, err);
    }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("RemoveFile", full_path, path, err);
  }
}

void EventLog::StartupEvent(const std::string& mount_point, const std::string& address) {
  if (level_ >= kInfo) {
    Lock lock;
    out_ << "OK ServerStartup (" << address << "):" << mount_point << "\n";
  }
}

void EventLog::UploadFileEvent(const std::string& full_path, const std::string& path,
    const std::string& contents, int err) {
  Lock lock;
  if (level_ >= kInfo) {
    if (err == 0) {
      out_ << "OK UploadFile " << path << " " << contents.size() << " bytes";
      if (level_ >= kDebug) {
        out_ << " (" << full_path << ")";
	DumpFile(contents);
      }
      out_ << "\n";
    } else { HandleGoodErrors("UploadFile", full_path, path, err); }
  } else if (level_ >= kError && err != 0) {
    HandleBadErrors("UploadFile", full_path, path, err);
  }
}
