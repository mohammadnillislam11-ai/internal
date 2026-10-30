#pragma once

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>

class KittyIOFile {
private:
    std::string _filePath;
    int _flags;
    mode_t _mode;
    int _fd;
    std::string _lastError;

public:
    KittyIOFile() : _flags(0), _mode(0644), _fd(-1) {}

    KittyIOFile(const std::string& path, int flags, mode_t mode = 0644)
        : _filePath(path), _flags(flags), _mode(mode), _fd(-1) {}

    ~KittyIOFile() {
        close();
    }

    bool open() {
        close();
        _fd = ::open(_filePath.c_str(), _flags, _mode);
        if (_fd == -1) {
            _lastError = strerror(errno);
            return false;
        }
        return true;
    }

    bool open(const std::string& path, int flags, mode_t mode = 0644) {
        _filePath = path;
        _flags = flags;
        _mode = mode;
        return open();
    }

    void close() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    int fd() const {
        return _fd;
    }

    bool isOpen() const {
        return _fd != -1;
    }

    const std::string& filePath() const {
        return _filePath;
    }

    const std::string& lastStrError() const {
        return _lastError;
    }

    struct stat info() const {
        struct stat st{};
        if (_fd != -1) {
            fstat(_fd, &st);
        } else if (!_filePath.empty()) {
            stat(_filePath.c_str(), &st);
        }
        return st;
    }

    ssize_t read(void* buffer, size_t size) {
        if (_fd == -1) return -1;
        return ::read(_fd, buffer, size);
    }

    ssize_t pread(uintptr_t offset, void* buffer, size_t size) {
        if (_fd == -1) return -1;
        return ::pread(_fd, buffer, size, static_cast<off_t>(offset));
    }

    ssize_t write(const void* buffer, size_t size) {
        if (_fd == -1) return -1;
        return ::write(_fd, buffer, size);
    }

    ssize_t pwrite(uintptr_t offset, const void* buffer, size_t size) {
        if (_fd == -1) return -1;
        return ::pwrite(_fd, buffer, size, static_cast<off_t>(offset));
    }
};
