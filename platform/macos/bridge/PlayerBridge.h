#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>
#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, PlayerBridgeState) {
    PlayerBridgeStateIdle,
    PlayerBridgeStateLoading,
    PlayerBridgeStatePlaying,
    PlayerBridgeStatePaused,
    PlayerBridgeStateSeeking,
    PlayerBridgeStateStopped,
    PlayerBridgeStateError
};

typedef NS_ENUM(NSInteger, PlayerRepeatMode) {
    PlayerRepeatModeNone,
    PlayerRepeatModeOne,
    PlayerRepeatModeAll
};

@interface PlayerBridgePlaylistItem : NSObject
@property (readonly) NSString* uri;
@property (readonly) NSString* title;
@property (readonly) NSString* artist;
@property (readonly) double duration;
@end

@interface PlayerBridgeSubtitle : NSObject
@property (readonly, copy) NSString* text;
@property (readonly) BOOL isBitmap;
@property (readonly, nullable) NSImage* image;
@property (readonly) int x;
@property (readonly) int y;
@property (readonly) int width;
@property (readonly) int height;
@end

@interface PlayerBridge : NSObject

- (instancetype)init;
- (BOOL)open:(NSString*)url;
- (void)play;
- (void)pause;
- (void)seekTo:(double)seconds;
- (void)stop;
- (void)close;

// Playlist management
- (void)appendToPlaylist:(NSString*)url NS_SWIFT_NAME(appendToPlaylist(_:));
- (void)playPlaylistItemAtIndex:(NSInteger)index;
- (void)nextTrack;
- (void)previousTrack;
- (void)removeFromPlaylistAtIndex:(NSInteger)index;
- (void)clearPlaylist;

@property (readonly) PlayerBridgeState state;
@property (readonly) NSString* currentURL;
@property (readonly) double duration;
@property (readonly) double currentTime;
@property float volume;

@property (readonly) BOOL hasVideo;
@property (readonly) BOOL hasAudio;
@property (readonly) int videoQueueDepth;
@property (readonly) int videoWidth;
@property (readonly) int videoHeight;

@property (readonly) NSArray<PlayerBridgePlaylistItem*>* playlistItems;
@property (readonly) NSInteger currentPlaylistIndex;
@property (readonly) BOOL hasNextTrack;
@property (readonly) BOOL hasPreviousTrack;
@property (readonly) BOOL shuffleEnabled;
@property (readonly) PlayerRepeatMode repeatMode;
- (void)setShuffle:(BOOL)enabled;
- (void)setRepeatMode:(PlayerRepeatMode)mode;

@property (readonly, copy) NSString* mediaTitle;
@property (readonly, copy) NSString* mediaArtist;
@property (readonly, copy) NSString* mediaAlbum;
@property (readonly) int mediaBitrate;
@property (readonly, copy) NSString* videoCodecName;
@property (readonly, copy) NSString* audioCodecName;

- (void)setupMetalLayer:(void*)metalLayer width:(int)width height:(int)height;
- (void)resizeMetal:(int)width height:(int)height;
- (void)renderFrame;

// Event polling & errors
- (nullable NSString*)pollEvent;
- (nullable NSString*)lastErrorMessage;

@property (readonly) BOOL hasSubtitles;
- (NSArray<NSString*>*)currentSubtitleTexts; // Keep for backwards compatibility
- (NSArray<PlayerBridgeSubtitle*>*)currentSubtitles;

@end

NS_ASSUME_NONNULL_END
