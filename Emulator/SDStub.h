#ifndef SDStub_H_
#define SDStub_H_

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#define FILE_READ "r"
#define FILE_WRITE "a"

class File {
  public:
    File(int fd) : mfd(fd) {
      if (fd != -1) {
        int end = lseek(mfd, 0L, SEEK_END);
        msize = mfd - fd;
        mavailable = true;
      }
    }
    
    void _write(uint8_t c) {
      write(mfd, &c, sizeof(uint8_t));
    }

    void _write(const uint8_t* str, size_t len) {
      write(mfd, str, sizeof(uint8_t)*len);
    }
    
    void _read(uint8_t* buf, size_t len) {
      mavailable = read(mfd, buf, sizeof(uint8_t)*len);
    }

    uint8_t _read() {
      uint8_t c;
      mavailable = read(mfd, &c, sizeof(uint8_t));
      return c;
    }
  
    bool _available() {
      return mavailable != 0;
    }
  
    void _seek(size_t offset) {
      lseek(mfd, offset, SEEK_SET);
    }

    int size() {
      return msize;
    }

    void _close() {
      if (mfd != -1) close(mfd);
    }
  
    ~File() {
      _close();
      mfd = -1;
      msize = 0;
    }
  private:
    int mfd;
    unsigned int msize;
    bool mavailable;
};

class SDStub {
public:
  static File _open(const char* name, const char* options) {
    int fd = open(name, O_CREAT | O_RDWR, 0666);
    return File(fd);
  }

  static bool _exists(const char* name) {
    DIR* dir = opendir(name);
    if (dir) {
      closedir(dir);
      return true;
    } 
    return false;
  }

  static void _mkdir(const char* name) {
    mkdir(name, (mode_t) 0755);
  }
};

#endif