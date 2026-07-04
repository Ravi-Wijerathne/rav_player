#pragma once

#include "MediaSource.h"

namespace rav {

class LocalFileSource : public MediaSource {
public:
    explicit LocalFileSource(std::string path)
        : path_(std::move(path)) {}

    bool open() override {
        exists_ = file_exists(path_);
        return exists_;
    }

    void close() override {
        exists_ = false;
    }

    bool is_open() const override { return exists_; }

    const std::string& url() const override { return path_; }

    MediaSourceType type() const override { return MediaSourceType::LocalFile; }

private:
    static bool file_exists(const std::string& path) {
        FILE* f = std::fopen(path.c_str(), "r");
        if (!f) return false;
        std::fclose(f);
        return true;
    }

    std::string path_;
    bool exists_{false};
};

} // namespace rav
