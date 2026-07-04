#include <gtest/gtest.h>
#include <playlist/Playlist.h>
#include <playlist/PlaylistItem.h>

using namespace rav;

TEST(PlaylistTest, InitiallyEmpty) {
    Playlist pl;
    EXPECT_TRUE(pl.empty());
    EXPECT_EQ(pl.size(), 0);
}

TEST(PlaylistTest, NamedPlaylist) {
    Playlist pl("My Playlist");
    EXPECT_EQ(pl.name(), "My Playlist");
}

TEST(PlaylistTest, AddItems) {
    Playlist pl;
    PlaylistItem item;
    item.uri = "file1.mp3";
    item.title = "Song 1";
    pl.add_item(item);

    item.uri = "file2.mp3";
    item.title = "Song 2";
    pl.add_item(item);

    EXPECT_EQ(pl.size(), 2);
    EXPECT_FALSE(pl.empty());
}

TEST(PlaylistTest, AccessItems) {
    Playlist pl;
    PlaylistItem item;
    item.uri = "test.mp3";
    item.title = "Test Song";
    pl.add_item(item);

    const auto& retrieved = pl.item(0);
    EXPECT_EQ(retrieved.uri, "test.mp3");
    EXPECT_EQ(retrieved.title, "Test Song");
}

TEST(PlaylistTest, RemoveItem) {
    Playlist pl;
    pl.add_item({"file1.mp3"});
    pl.add_item({"file2.mp3"});
    pl.add_item({"file3.mp3"});
    EXPECT_EQ(pl.size(), 3);
    pl.remove_item(1);
    EXPECT_EQ(pl.size(), 2);
    EXPECT_EQ(pl.item(1).uri, "file3.mp3");
}

TEST(PlaylistTest, Clear) {
    Playlist pl;
    pl.add_item({"file1.mp3"});
    pl.add_item({"file2.mp3"});
    pl.clear();
    EXPECT_TRUE(pl.empty());
}

TEST(PlaylistTest, SetName) {
    Playlist pl;
    pl.set_name("My Favorites");
    EXPECT_EQ(pl.name(), "My Favorites");
}

TEST(PlaylistTest, ItemsAccessor) {
    Playlist pl;
    pl.add_item({"file1.mp3"});
    pl.add_item({"file2.mp3"});
    const auto& items = pl.items();
    EXPECT_EQ(items.size(), 2);
}

TEST(PlaylistTest, ModifiableItem) {
    Playlist pl;
    pl.add_item({"file1.mp3"});
    pl.item(0).title = "Updated Title";
    EXPECT_EQ(pl.item(0).title, "Updated Title");
}
