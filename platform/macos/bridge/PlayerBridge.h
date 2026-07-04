#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, PlayerBridgeState) {
    PlayerBridgeStateIdle,
    PlayerBridgeStateLoading,
    PlayerBridgeStatePlaying,
    PlayerBridgeStatePaused,
    PlayerBridgeStateStopped,
    PlayerBridgeStateError
};

@interface PlayerBridge : NSObject

- (instancetype)init;
- (BOOL)open:(NSString*)url;
- (void)play;
- (void)pause;
- (void)seekTo:(double)seconds;
- (void)stop;
- (void)close;

@property (readonly) PlayerBridgeState state;
@property (readonly) double duration;
@property (readonly) double currentTime;
@property float volume;

@property (readonly) BOOL hasVideo;
@property (readonly) BOOL hasAudio;
@property (readonly) int videoQueueDepth;
@property (readonly) int videoWidth;
@property (readonly) int videoHeight;

- (void)setupMetalLayer:(void*)metalLayer width:(int)width height:(int)height;
- (void)resizeMetal:(int)width height:(int)height;
- (void)renderFrame;

@end

NS_ASSUME_NONNULL_END
