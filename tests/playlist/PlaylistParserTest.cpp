#include <gtest/gtest.h>
#include <playlist/PlaylistParser.h>

using namespace rav;

TEST(PlaylistParserTest, DetectM3U) {
    EXPECT_EQ(PlaylistParser::detect_format("#EXTM3U"), PlaylistFormat::M3U8);
    EXPECT_EQ(PlaylistParser::detect_format("# comment"), PlaylistFormat::M3U);
}

TEST(PlaylistParserTest, DetectPLS) {
    EXPECT_EQ(PlaylistParser::detect_format("[playlist]"), PlaylistFormat::PLS);
}

TEST(PlaylistParserTest, DetectUnknown) {
    EXPECT_EQ(PlaylistParser::detect_format("some random content"), PlaylistFormat::Unknown);
    EXPECT_EQ(PlaylistParser::detect_format(""), PlaylistFormat::Unknown);
}

TEST(PlaylistParserTest, ParseM3USimple) {
    std::string content =
        "#EXTM3U\n"
        "#EXTINF:123,Song Title\n"
        "/path/to/song.mp3\n"
        "#EXTINF:456,Another Song\n"
        "/path/to/another.mp3\n";

    PlaylistParser parser;
    auto playlist = parser.parse_m3u(content);

    EXPECT_EQ(playlist.size(), 2);
    EXPECT_EQ(playlist.item(0).uri, "/path/to/song.mp3");
    EXPECT_EQ(playlist.item(0).title, "Song Title");
    EXPECT_EQ(playlist.item(1).uri, "/path/to/another.mp3");
    EXPECT_EQ(playlist.item(1).title, "Another Song");
}

TEST(PlaylistParserTest, ParseM3UNoExtinf) {
    std::string content =
        "#EXTM3U\n"
        "/path/to/song1.mp3\n"
        "/path/to/song2.mp3\n";

    PlaylistParser parser;
    auto playlist = parser.parse_m3u(content);

    EXPECT_EQ(playlist.size(), 2);
    EXPECT_EQ(playlist.item(0).title, "/path/to/song1.mp3");
}

TEST(PlaylistParserTest, ParsePLS) {
    std::string content =
        "[playlist]\n"
        "NumberOfEntries=2\n"
        "File1=/path/to/song1.mp3\n"
        "Title1=Song One\n"
        "File2=/path/to/song2.mp3\n"
        "Title2=Song Two\n"
        "Version=2\n";

    PlaylistParser parser;
    auto playlist = parser.parse_pls(content);

    EXPECT_EQ(playlist.size(), 2);
    EXPECT_EQ(playlist.item(0).uri, "/path/to/song1.mp3");
    EXPECT_EQ(playlist.item(0).title, "Song One");
    EXPECT_EQ(playlist.item(1).uri, "/path/to/song2.mp3");
    EXPECT_EQ(playlist.item(1).title, "Song Two");
}

TEST(PlaylistParserTest, ParsePLSNoTitle) {
    std::string content =
        "[playlist]\n"
        "NumberOfEntries=1\n"
        "File1=test.mp3\n"
        "Version=2\n";

    PlaylistParser parser;
    auto playlist = parser.parse_pls(content);
    EXPECT_EQ(playlist.size(), 1);
    EXPECT_EQ(playlist.item(0).uri, "test.mp3");
    EXPECT_EQ(playlist.item(0).title, "test.mp3");
}

TEST(PlaylistParserTest, SerializeM3U) {
    Playlist playlist("Test");
    PlaylistItem item1;
    item1.uri = "song1.mp3";
    item1.title = "Song 1";
    playlist.add_item(item1);

    PlaylistItem item2;
    item2.uri = "song2.mp3";
    item2.title = "Song 2";
    playlist.add_item(item2);

    PlaylistParser parser;
    auto result = parser.serialize(playlist, PlaylistFormat::M3U);

    EXPECT_TRUE(result.find("#EXTM3U") == 0);
    EXPECT_TRUE(result.find("Song 1") != std::string::npos);
    EXPECT_TRUE(result.find("song1.mp3") != std::string::npos);
}

TEST(PlaylistParserTest, SerializePLS) {
    Playlist playlist("Test");
    PlaylistItem item;
    item.uri = "test.mp3";
    item.title = "Test Song";
    playlist.add_item(item);

    PlaylistParser parser;
    auto result = parser.serialize(playlist, PlaylistFormat::PLS);

    EXPECT_TRUE(result.find("[playlist]") != std::string::npos);
    EXPECT_TRUE(result.find("File1=test.mp3") != std::string::npos);
    EXPECT_TRUE(result.find("Title1=Test Song") != std::string::npos);
}

TEST(PlaylistParserTest, IsPlaylistExtension) {
    EXPECT_TRUE(PlaylistParser::is_playlist_extension("file.m3u"));
    EXPECT_TRUE(PlaylistParser::is_playlist_extension("file.m3u8"));
    EXPECT_TRUE(PlaylistParser::is_playlist_extension("file.pls"));
    EXPECT_FALSE(PlaylistParser::is_playlist_extension("file.mp3"));
    EXPECT_FALSE(PlaylistParser::is_playlist_extension("file.mp4"));
}

TEST(PlaylistParserTest, ParseByDetectM3U) {
    std::string content =
        "#EXTM3U\n"
        "#EXTINF:-1,Track\n"
        "track.mp3\n";

    PlaylistParser parser;
    auto playlist = parser.parse(content, PlaylistFormat::Unknown);
    EXPECT_EQ(playlist.size(), 0);

    playlist = parser.parse(content, PlaylistFormat::M3U8);
    EXPECT_EQ(playlist.size(), 1);
}
