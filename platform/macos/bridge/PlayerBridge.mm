#include <atomic>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "PlayerBridge.h"

#include "../../../engine/core/PlaybackEngine.h"
#include "../../../engine/rendering/MetalRenderer.h"
#include "../../../engine/platform/MacOSAudioOutput.h"
#include "../../../engine/playlist/PlaybackQueue.h"
#include "../../../engine/playlist/PlaylistItem.h"

using namespace rav;

@implementation PlayerBridgePlaylistItem
- (instancetype)initWithURI:(NSString*)uri title:(NSString*)title artist:(NSString*)artist duration:(double)duration {
    self = [super init];
    if (self) {
        _uri = uri;
        _title = title;
        _artist = artist;
        _duration = duration;
    }
    return self;
}
@end

@implementation PlayerBridgeSubtitle
- (instancetype)initWithText:(NSString*)text {
    self = [super init];
    if (self) {
        _text = text;
        _isBitmap = NO;
        _image = nil;
    }
    return self;
}

- (instancetype)initWithImage:(NSImage*)image x:(int)x y:(int)y width:(int)width height:(int)height {
    self = [super init];
    if (self) {
        _text = @"";
        _isBitmap = YES;
        _image = image;
        _x = x;
        _y = y;
        _width = width;
        _height = height;
    }
    return self;
}
@end

@interface PlayerBridge () {
    std::unique_ptr<PlaybackEngine> _engine;
    std::unique_ptr<MetalRenderer> _renderer;
    std::unique_ptr<MacOSAudioOutput> _audioOutput;
    std::unique_ptr<PlaybackQueue> _playbackQueue;
    dispatch_queue_t _renderQueue;
    NSMutableArray<PlayerBridgePlaylistItem*>* _playlistItems;
    NSString* _currentURL;
    std::string _lastError;
}

- (void)syncPlaylistItems;
- (void)openCurrentItem;
@end

@implementation PlayerBridge

- (instancetype)init {
    self = [super init];
    if (self) {
        _engine = std::make_unique<PlaybackEngine>();
        _renderer = std::make_unique<MetalRenderer>();
        _audioOutput = std::make_unique<MacOSAudioOutput>();
        _playbackQueue = std::make_unique<PlaybackQueue>();
        _playlistItems = [NSMutableArray array];
        _renderQueue = dispatch_queue_create("com.ravplayer.render", DISPATCH_QUEUE_SERIAL);

        __weak PlayerBridge* weakSelf = self;
        _audioOutput->set_fill_callback([weakSelf](uint8_t* data, int frames) -> int {
            PlayerBridge* strongSelf = weakSelf;
            if (!strongSelf || !strongSelf->_engine) return 0;
            return strongSelf->_engine->fill_audio_buffer(data, frames);
        });
    }
    return self;
}

- (BOOL)open:(NSString*)url {
    std::string path = url.UTF8String ?: "";
    if (path.empty()) return NO;

    _engine->set_video_renderer(_renderer.get());
    _engine->set_audio_output(_audioOutput.get());

    BOOL result = _engine->open(path) ? YES : NO;
    if (result) {
        _currentURL = url;
        _lastError.clear();
    } else {
        _lastError = "Failed to open file or stream";
    }
    return result;
}

- (NSString*)currentURL { return _currentURL ?: @""; }

- (void)play {
    _engine->play();
}

- (void)pause {
    _engine->pause();
}

- (void)seekTo:(double)seconds {
    _engine->seek(seconds);
}

- (void)stop {
    _engine->stop();
}

- (void)close {
    _engine->close();
    _renderer->shutdown();
}

- (nullable NSString*)pollEvent {
    PlayerEvent event;
    if (!_engine->event_bus().poll(event)) return nil;

    std::string eventName;
    std::visit([&eventName, self](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, PlaybackStartedEvent>) eventName = "PlaybackStartedEvent";
        else if constexpr (std::is_same_v<T, PlaybackPausedEvent>) eventName = "PlaybackPausedEvent";
        else if constexpr (std::is_same_v<T, PlaybackEndedEvent>) eventName = "PlaybackEndedEvent";
        else if constexpr (std::is_same_v<T, MediaLoadedEvent>) eventName = "MediaLoadedEvent";
        else if constexpr (std::is_same_v<T, BufferingStartedEvent>) eventName = "BufferingStartedEvent";
        else if constexpr (std::is_same_v<T, BufferingEndedEvent>) eventName = "BufferingEndedEvent";
        else if constexpr (std::is_same_v<T, StateChangedEvent>) eventName = "StateChangedEvent";
        else if constexpr (std::is_same_v<T, ErrorOccurredEvent>) {
            self->_lastError = e.message;
            eventName = "ErrorOccurredEvent";
        }
        else eventName = "Unknown";
    }, event);
    return [NSString stringWithUTF8String:eventName.c_str()];
}

- (nullable NSString*)lastErrorMessage {
    if (_lastError.empty()) return nil;
    return [NSString stringWithUTF8String:_lastError.c_str()];
}

- (PlayerBridgeState)state {
    switch (_engine->state()) {
        case PlayerState::Idle:      return PlayerBridgeStateIdle;
        case PlayerState::Loading:   return PlayerBridgeStateLoading;
        case PlayerState::Playing:   return PlayerBridgeStatePlaying;
        case PlayerState::Paused:    return PlayerBridgeStatePaused;
        case PlayerState::Seeking:   return PlayerBridgeStateSeeking;
        case PlayerState::Buffering: return PlayerBridgeStateLoading;
        case PlayerState::Stopped:   return PlayerBridgeStateStopped;
        case PlayerState::Error:     return PlayerBridgeStateError;
    }
}

- (double)duration { return _engine->duration(); }
- (double)currentTime { return _engine->current_time(); }
- (float)volume { return _engine->volume(); }
- (void)setVolume:(float)vol { _engine->set_volume(vol); }
- (BOOL)hasVideo { return _engine->has_video() ? YES : NO; }
- (BOOL)hasAudio { return _engine->has_audio() ? YES : NO; }
- (int)videoQueueDepth { return _engine ? _engine->video_queue_size() : 0; }
- (int)videoWidth { return _engine ? _engine->video_width() : 0; }
- (int)videoHeight { return _engine ? _engine->video_height() : 0; }

- (void)setupMetalLayer:(void*)metalLayer width:(int)width height:(int)height {
    if (!_renderer) return;
    _renderer->set_layer(metalLayer);
    _renderer->resize(width, height);
    _renderer->init();
}

- (void)resizeMetal:(int)width height:(int)height {
    if (!_renderer) return;
    _renderer->resize(width, height);
}

- (NSString*)mediaTitle {
    return _engine ? [NSString stringWithUTF8String:_engine->media_title().c_str()] : @"";
}
- (NSString*)mediaArtist {
    return _engine ? [NSString stringWithUTF8String:_engine->media_artist().c_str()] : @"";
}
- (NSString*)mediaAlbum {
    return _engine ? [NSString stringWithUTF8String:_engine->media_album().c_str()] : @"";
}
- (int)mediaBitrate { return _engine ? _engine->media_bitrate() : 0; }
- (NSString*)videoCodecName {
    return _engine ? [NSString stringWithUTF8String:_engine->video_codec_name().c_str()] : @"";
}
- (NSString*)audioCodecName {
    return _engine ? [NSString stringWithUTF8String:_engine->audio_codec_name().c_str()] : @"";
}

- (BOOL)hasSubtitles {
    return _engine ? _engine->has_subtitles() ? YES : NO : NO;
}

- (NSArray<NSString*>*)currentSubtitleTexts {
    if (!_engine) return @[];
    auto subs = _engine->current_subtitles();
    if (subs.empty()) return @[];
    NSMutableArray* result = [NSMutableArray arrayWithCapacity:subs.size()];
    for (const auto& sub : subs) {
        if (!sub.text.empty()) {
            NSString* text = [NSString stringWithUTF8String:sub.text.c_str()];
            if (text) [result addObject:text];
        }
    }
    return result;
}

- (NSArray<PlayerBridgeSubtitle*>*)currentSubtitles {
    if (!_engine) return @[];
    auto subs = _engine->current_subtitles();
    if (subs.empty()) return @[];
    
    NSMutableArray* result = [NSMutableArray arrayWithCapacity:subs.size()];
    for (const auto& sub : subs) {
        if (sub.is_bitmap && !sub.bitmap_data.empty()) {
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef context = CGBitmapContextCreate(
                (void*)sub.bitmap_data.data(),
                sub.width,
                sub.height,
                8,
                sub.width * 4,
                colorSpace,
                (CGBitmapInfo)kCGImageAlphaPremultipliedLast | (CGBitmapInfo)kCGBitmapByteOrder32Big
            );
            if (context) {
                CGImageRef cgImage = CGBitmapContextCreateImage(context);
                if (cgImage) {
                    NSImage* image = [[NSImage alloc] initWithCGImage:cgImage size:NSMakeSize(sub.width, sub.height)];
                    PlayerBridgeSubtitle* objcSub = [[PlayerBridgeSubtitle alloc] initWithImage:image x:sub.x y:sub.y width:sub.width height:sub.height];
                    [result addObject:objcSub];
                    CGImageRelease(cgImage);
                }
                CGContextRelease(context);
            }
            CGColorSpaceRelease(colorSpace);
        } else if (!sub.text.empty()) {
            NSString* text = [NSString stringWithUTF8String:sub.text.c_str()];
            if (text) {
                PlayerBridgeSubtitle* objcSub = [[PlayerBridgeSubtitle alloc] initWithText:text];
                [result addObject:objcSub];
            }
        }
    }
    return result;
}

// ── Playlist Management ──

- (void)appendToPlaylist:(NSString*)url {
    std::string path = url.UTF8String ?: "";
    if (path.empty()) return;

    PlaylistItem item;
    item.uri = path;

    // Extract a basic title from filename
    auto pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        item.title = path.substr(pos + 1);
    } else {
        item.title = path;
    }
    // Remove file extension for display
    auto dot = item.title.find_last_of('.');
    if (dot != std::string::npos) {
        item.title = item.title.substr(0, dot);
    }

    _playbackQueue->add_item(item);
    [self syncPlaylistItems];
}

- (void)playPlaylistItemAtIndex:(NSInteger)index {
    if (index < 0 || index >= (NSInteger)_playbackQueue->size()) return;
    _playbackQueue->go_to((size_t)index);
    [self openCurrentItem];
}

- (void)nextTrack {
    if (!_playbackQueue->has_next()) return;
    _playbackQueue->next();
    [self openCurrentItem];
}

- (void)previousTrack {
    if (!_playbackQueue->has_previous()) return;
    const auto& current = _playbackQueue->current();
    _playbackQueue->previous();
    [self openCurrentItem];
}

- (void)removeFromPlaylistAtIndex:(NSInteger)index {
    if (index < 0 || index >= (NSInteger)_playbackQueue->size()) return;
    _playbackQueue->remove_item((size_t)index);
    [self syncPlaylistItems];
}

- (void)clearPlaylist {
    _playbackQueue->clear();
    [self syncPlaylistItems];
}

- (void)setShuffle:(BOOL)enabled {
    _playbackQueue->set_shuffle(enabled == YES);
}

- (void)setRepeatMode:(PlayerRepeatMode)mode {
    switch (mode) {
        case PlayerRepeatModeNone: _playbackQueue->set_repeat(rav::RepeatMode::None); break;
        case PlayerRepeatModeOne:  _playbackQueue->set_repeat(rav::RepeatMode::One); break;
        case PlayerRepeatModeAll:  _playbackQueue->set_repeat(rav::RepeatMode::All); break;
    }
}

- (NSArray<PlayerBridgePlaylistItem*>*)playlistItems {
    return _playlistItems;
}

- (NSInteger)currentPlaylistIndex {
    return _playbackQueue ? (NSInteger)_playbackQueue->current_index() : -1;
}

- (BOOL)hasNextTrack {
    return _playbackQueue ? _playbackQueue->has_next() : NO;
}

- (BOOL)hasPreviousTrack {
    return _playbackQueue ? _playbackQueue->has_previous() : NO;
}

- (BOOL)shuffleEnabled {
    return _playbackQueue ? (_playbackQueue->shuffle() ? YES : NO) : NO;
}

- (PlayerRepeatMode)repeatMode {
    if (!_playbackQueue) return PlayerRepeatModeNone;
    switch (_playbackQueue->repeat()) {
        case rav::RepeatMode::None: return PlayerRepeatModeNone;
        case rav::RepeatMode::One:  return PlayerRepeatModeOne;
        case rav::RepeatMode::All:  return PlayerRepeatModeAll;
    }
}

// ── Internal Helpers ──

- (void)syncPlaylistItems {
    [_playlistItems removeAllObjects];
    if (!_playbackQueue) return;

    for (const auto& item : _playbackQueue->items()) {
        PlayerBridgePlaylistItem* objcItem =
            [[PlayerBridgePlaylistItem alloc] initWithURI:[NSString stringWithUTF8String:item.uri.c_str()]
                                                    title:[NSString stringWithUTF8String:item.title.c_str()]
                                                   artist:[NSString stringWithUTF8String:item.artist.c_str()]
                                                 duration:std::chrono::duration<double>(item.duration).count()];
        if (objcItem) {
            [_playlistItems addObject:objcItem];
        }
    }
}

- (void)openCurrentItem {
    if (!_playbackQueue || _playbackQueue->empty()) return;
    const auto& item = _playbackQueue->current();
    NSString* url = [NSString stringWithUTF8String:item.uri.c_str()];
    [self open:url];
    [self play];
}

- (void)renderFrame {
    if (!_engine || !_renderer || !_renderer->is_ready()) return;
    if (_engine->state() != PlayerState::Playing) return;

    static int frameCount = 0;
    if (++frameCount % 30 == 1) {
        NSLog(@"renderFrame: state=%d vq=%d aq=%d vw=%d vh=%d",
              (int)_engine->state(), _engine->video_queue_size(),
              _engine->audio_queue_size(), _engine->video_width(),
              _engine->video_height());
    }

    VideoFrame vf;
    if (_engine->sync_pop_video_frame(vf)) {
        _renderer->present_frame(vf);
    } else if (frameCount % 30 == 1) {
        NSLog(@"renderFrame: sync_pop_video_frame returned false");
    }
}

@end
