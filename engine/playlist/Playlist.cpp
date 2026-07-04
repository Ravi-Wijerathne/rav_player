#include "Playlist.h"

namespace rav {

Playlist::Playlist(std::string name)
    : name_(std::move(name))
{
}

void Playlist::add_item(const PlaylistItem& item) {
    items_.push_back(item);
}

void Playlist::remove_item(size_t index) {
    if (index < items_.size()) {
        items_.erase(items_.begin() + static_cast<ptrdiff_t>(index));
    }
}

void Playlist::clear() {
    items_.clear();
}

const PlaylistItem& Playlist::item(size_t index) const {
    return items_[index];
}

PlaylistItem& Playlist::item(size_t index) {
    return items_[index];
}

} // namespace rav
