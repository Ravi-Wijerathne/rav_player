#include <gtest/gtest.h>
#include <streaming/HLSPlaylist.h>

using namespace rav;

TEST(HLSPlaylistParserTest, DetectHLSPlaylist) {
    HLSPlaylistParser parser;
    EXPECT_TRUE(parser.is_hls_playlist("#EXTM3U\n"));
    EXPECT_FALSE(parser.is_hls_playlist("# Not a playlist\n"));
}

TEST(HLSPlaylistParserTest, ParseSimplePlaylist) {
    std::string content =
        "#EXTM3U\n"
        "#EXT-X-VERSION:3\n"
        "#EXT-X-TARGETDURATION:10\n"
        "#EXT-X-MEDIA-SEQUENCE:0\n"
        "#EXTINF:9.009,\n"
        "segment0.ts\n"
        "#EXTINF:10.010,\n"
        "segment1.ts\n"
        "#EXT-X-ENDLIST\n";

    HLSPlaylistParser parser;
    auto playlist = parser.parse(content);

    EXPECT_EQ(playlist.version, 3);
    EXPECT_EQ(playlist.target_duration, 10.0);
    EXPECT_EQ(playlist.media_sequence, 0);
    EXPECT_FALSE(playlist.is_endless);
    EXPECT_EQ(playlist.segments.size(), 2);
    EXPECT_DOUBLE_EQ(playlist.segments[0].duration, 9.009);
    EXPECT_EQ(playlist.segments[0].uri, "segment0.ts");
    EXPECT_DOUBLE_EQ(playlist.segments[1].duration, 10.010);
    EXPECT_EQ(playlist.segments[1].uri, "segment1.ts");
}

TEST(HLSPlaylistParserTest, ParseMasterPlaylist) {
    std::string content =
        "#EXTM3U\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,RESOLUTION=640x360\n"
        "low.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=2560000,RESOLUTION=1280x720\n"
        "mid.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=8000000,RESOLUTION=1920x1080\n"
        "high.m3u8\n";

    HLSPlaylistParser parser;
    auto playlist = parser.parse(content);

    EXPECT_TRUE(playlist.is_master);
    EXPECT_EQ(playlist.variants.size(), 3);
    EXPECT_EQ(playlist.variants[0].bandwidth, 1280000);
    EXPECT_EQ(playlist.variants[0].width, 640);
    EXPECT_EQ(playlist.variants[0].height, 360);
    EXPECT_EQ(playlist.variants[0].uri, "low.m3u8");
    EXPECT_EQ(playlist.variants[2].bandwidth, 8000000);
    EXPECT_EQ(playlist.variants[2].width, 1920);
    EXPECT_EQ(playlist.variants[2].height, 1080);
}

TEST(HLSPlaylistParserTest, ParseEmptyContent) {
    HLSPlaylistParser parser;
    auto playlist = parser.parse("");
    EXPECT_EQ(playlist.segments.size(), 0);
    EXPECT_EQ(playlist.version, 1);
}

TEST(HLSPlaylistParserTest, IsHLSURL) {
    EXPECT_TRUE(HLSPlaylistParser::is_hls_url("http://example.com/stream.m3u8"));
    EXPECT_TRUE(HLSPlaylistParser::is_hls_url("http://example.com/stream.m3u"));
    EXPECT_FALSE(HLSPlaylistParser::is_hls_url("http://example.com/stream.mp4"));
    EXPECT_FALSE(HLSPlaylistParser::is_hls_url("file.mp4"));
}

TEST(HLSPlaylistParserTest, ParseWithoutEndlist) {
    std::string content =
        "#EXTM3U\n"
        "#EXTINF:5.0,\n"
        "seg0.ts\n"
        "#EXTINF:5.0,\n"
        "seg1.ts\n";

    HLSPlaylistParser parser;
    auto playlist = parser.parse(content);
    EXPECT_EQ(playlist.segments.size(), 2);
    EXPECT_TRUE(playlist.is_endless);
}
