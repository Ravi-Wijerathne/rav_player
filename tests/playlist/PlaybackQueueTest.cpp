#include <gtest/gtest.h>
#include <playlist/PlaybackQueue.h>

#include <stdexcept>

using namespace rav;

static PlaylistItem make_item(const std::string& uri) {
    PlaylistItem item;
    item.uri = uri;
    item.title = uri;
    return item;
}

TEST(PlaybackQueueTest, InitiallyEmpty) {
    PlaybackQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST(PlaybackQueueTest, AddItems) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.add_item(make_item("c"));
    EXPECT_EQ(q.size(), 3);
}

TEST(PlaybackQueueTest, SetItems) {
    PlaybackQueue q;
    q.set_items({make_item("a"), make_item("b"), make_item("c")});
    EXPECT_EQ(q.size(), 3);
}

TEST(PlaybackQueueTest, CurrentItem) {
    PlaybackQueue q;
    q.add_item(make_item("first"));
    EXPECT_EQ(q.current().uri, "first");
}

TEST(PlaybackQueueTest, NextItem) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    EXPECT_EQ(q.current().uri, "a");
    EXPECT_EQ(q.next().uri, "b");
}

TEST(PlaybackQueueTest, PreviousItem) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.next();
    EXPECT_EQ(q.current().uri, "b");
    EXPECT_EQ(q.previous().uri, "a");
}

TEST(PlaybackQueueTest, HasNext) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    EXPECT_TRUE(q.has_next());
    q.next();
    EXPECT_FALSE(q.has_next());
}

TEST(PlaybackQueueTest, HasPrevious) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    EXPECT_FALSE(q.has_previous());
    q.add_item(make_item("b"));
    q.next();
    EXPECT_TRUE(q.has_previous());
}

TEST(PlaybackQueueTest, Shuffle) {
    PlaybackQueue q;
    q.set_items({make_item("a"), make_item("b"), make_item("c"),
                 make_item("d"), make_item("e")});
    q.set_shuffle(true);
    EXPECT_TRUE(q.shuffle());
    EXPECT_FALSE(q.empty());
}

TEST(PlaybackQueueTest, RepeatNone) {
    PlaybackQueue q;
    q.set_repeat(RepeatMode::None);
    EXPECT_EQ(q.repeat(), RepeatMode::None);
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    EXPECT_TRUE(q.has_next());
    q.next();
    EXPECT_FALSE(q.has_next());
}

TEST(PlaybackQueueTest, RepeatAll) {
    PlaybackQueue q;
    q.set_repeat(RepeatMode::All);
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.next();
    EXPECT_TRUE(q.has_next());
}

TEST(PlaybackQueueTest, RepeatOne) {
    PlaybackQueue q;
    q.set_repeat(RepeatMode::One);
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    EXPECT_TRUE(q.has_next());
}

TEST(PlaybackQueueTest, GoTo) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.add_item(make_item("c"));
    q.go_to(2);
    EXPECT_EQ(q.current().uri, "c");
}

TEST(PlaybackQueueTest, Clear) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.clear();
    EXPECT_TRUE(q.empty());
}

TEST(PlaybackQueueTest, RemoveItem) {
    PlaybackQueue q;
    q.add_item(make_item("a"));
    q.add_item(make_item("b"));
    q.add_item(make_item("c"));
    q.remove_item(1);
    EXPECT_EQ(q.size(), 2);
}

TEST(PlaybackQueueTest, EmptyQueueThrows) {
    PlaybackQueue q;
    EXPECT_THROW(q.current(), std::out_of_range);
    EXPECT_THROW(q.next(), std::out_of_range);
    EXPECT_THROW(q.previous(), std::out_of_range);
}
