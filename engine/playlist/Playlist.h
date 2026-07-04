#pragma once

#include <string>
#include <vector>

#include "PlaylistItem.h"

namespace rav {

class Playlist {
public:
    Playlist() = default;
    explicit Playlist(std::string name);

    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    void add_item(const PlaylistItem& item);
    void remove_item(size_t index);
    void clear();

    const PlaylistItem& item(size_t index) const;
    PlaylistItem& item(size_t index);

    const std::vector<PlaylistItem>& items() const { return items_; }

private:
    std::string name_;
    std::vector<PlaylistItem> items_;
};

} // namespace rav
