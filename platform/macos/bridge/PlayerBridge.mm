#include <atomic>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "PlayerBridge.h"

#define AVMediaType FF_AVMediaType
#include "../../../engine/core/PlaybackEngine.h"
#include "../../../engine/rendering/MetalRenderer.h"
#include "../../../engine/playlist/PlaybackQueue.h"
#include "../../../engine/playlist/PlaylistItem.h"
#include "../../../engine/rendering/FrameConverter.h"
#undef AVMediaType

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>

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

struct CachedFrame {
    rav::FramePtr frame;
    double pts;
    int width;
    int height;
    rav::VideoFrameFormat format;
};

@interface PlayerBridge () {
    std::unique_ptr<PlaybackEngine> _engine;
    std::unique_ptr<MetalRenderer> _renderer;
    std::unique_ptr<PlaybackQueue> _playbackQueue;
    dispatch_queue_t _renderQueue;
    NSMutableArray<PlayerBridgePlaylistItem*>* _playlistItems;
    NSString* _currentURL;
    std::string _lastError;
    
    BOOL _isPiPActive;
    AVSampleBufferDisplayLayer* _sampleBufferDisplayLayer;
    std::unique_ptr<rav::FrameConverter> _pipConverter;
    CachedFrame _lastPoppedFrame;
    BOOL _hasLastPoppedFrame;
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
        _playbackQueue = std::make_unique<PlaybackQueue>();
        _playlistItems = [NSMutableArray array];
        _renderQueue = dispatch_queue_create("com.ravplayer.render", DISPATCH_QUEUE_SERIAL);
        _isPiPActive = NO;
        _sampleBufferDisplayLayer = [[AVSampleBufferDisplayLayer alloc] init];
        _sampleBufferDisplayLayer.videoGravity = AVLayerVideoGravityResizeAspect;
        _sampleBufferDisplayLayer.frame = CGRectMake(0, 0, 1, 1);
        _pipConverter = std::make_unique<rav::FrameConverter>();
        _hasLastPoppedFrame = NO;
    }
    return self;
}

- (BOOL)open:(NSString*)url {
    std::string path = url.UTF8String ?: "";
    if (path.empty()) return NO;

    _engine->set_video_renderer(_renderer.get());

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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
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
#pragma clang diagnostic pop
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

- (void)setupMetalLayer:(void*)layerPtr width:(int)width height:(int)height {
    if (!_renderer) return;
    CAMetalLayer* layer = (__bridge CAMetalLayer*)layerPtr;
    _renderer->set_layer((__bridge void*)layer);
    _renderer->resize(width, height);
    _renderer->init();
    
    // Ensure PiP layer is in the hierarchy to avoid crashes on restore
    if (_sampleBufferDisplayLayer.superlayer != layer.superlayer) {
        CGRect b = layer.bounds;
        if (isnan(b.origin.x)) b.origin.x = 0;
        if (isnan(b.origin.y)) b.origin.y = 0;
        if (isnan(b.size.width) || b.size.width < 1.0) b.size.width = 1.0;
        if (isnan(b.size.height) || b.size.height < 1.0) b.size.height = 1.0;
        _sampleBufferDisplayLayer.frame = b;
        _sampleBufferDisplayLayer.hidden = YES; // Hide initially until PiP starts
        [layer.superlayer insertSublayer:_sampleBufferDisplayLayer below:layer];
    } else {
        CGRect b = layer.bounds;
        if (isnan(b.origin.x)) b.origin.x = 0;
        if (isnan(b.origin.y)) b.origin.y = 0;
        if (isnan(b.size.width) || b.size.width < 1.0) b.size.width = 1.0;
        if (isnan(b.size.height) || b.size.height < 1.0) b.size.height = 1.0;
        _sampleBufferDisplayLayer.frame = b;
    }
}

- (void)resizeMetal:(int)width height:(int)height {
    if (!_renderer) return;
    _renderer->resize(width, height);
    
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (_sampleBufferDisplayLayer.superlayer) {
        CGRect b = _sampleBufferDisplayLayer.superlayer.bounds;
        if (isnan(b.origin.x)) b.origin.x = 0;
        if (isnan(b.origin.y)) b.origin.y = 0;
        if (isnan(b.size.width) || b.size.width < 1.0) b.size.width = 1.0;
        if (isnan(b.size.height) || b.size.height < 1.0) b.size.height = 1.0;
        _sampleBufferDisplayLayer.frame = b;
    }
    [CATransaction commit];
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

- (CMSampleBufferRef)createSampleBufferFromVideoFrame:(const CachedFrame&)vf {
    if (!vf.frame) return NULL;
    
    CVPixelBufferRef pixelBuffer = NULL;
    BOOL isHardware = (vf.frame->format == AV_PIX_FMT_VIDEOTOOLBOX);
    
    if (isHardware) {
        pixelBuffer = (CVPixelBufferRef)vf.frame->data[3];
        if (pixelBuffer) {
            CFRetain(pixelBuffer);
        }
    } else {
        rav::FrameConverterSpec spec;
        spec.src_width = vf.width;
        spec.src_height = vf.height;
        spec.src_format = (AVPixelFormat)vf.frame->format;
        
        int max_pip_w = 854;
        int dst_w = vf.width;
        int dst_h = vf.height;
        if (dst_w > max_pip_w) {
            dst_h = (int)((float)dst_h * ((float)max_pip_w / (float)dst_w));
            dst_w = max_pip_w;
        }
        spec.dst_width = dst_w;
        spec.dst_height = dst_h;
        spec.dst_format = AV_PIX_FMT_BGRA;
        
        if (_pipConverter->init(spec)) {
            if (_pipConverter->convert_to_buffer(vf.frame.get())) {
                NSDictionary *options = @{
                    (id)kCVPixelBufferCGImageCompatibilityKey: @YES,
                    (id)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
                    (id)kCVPixelBufferIOSurfacePropertiesKey: @{}
                };
                CVReturn err = CVPixelBufferCreate(kCFAllocatorDefault, dst_w, dst_h,
                                                   kCVPixelFormatType_32BGRA,
                                                   (__bridge CFDictionaryRef)options,
                                                   &pixelBuffer);
                if (err == kCVReturnSuccess) {
                    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
                    uint8_t* dst = (uint8_t*)CVPixelBufferGetBaseAddress(pixelBuffer);
                    size_t dstStride = CVPixelBufferGetBytesPerRow(pixelBuffer);
                    uint8_t* src = _pipConverter->data();
                    size_t srcStride = _pipConverter->linesize();
                    for (int y = 0; y < dst_h; ++y) {
                        memcpy(dst + y * dstStride, src + y * srcStride, dst_w * 4);
                    }
                    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
                }
            }
        }
    }
    
    if (!pixelBuffer) return NULL;
    
    CMVideoFormatDescriptionRef videoInfo = NULL;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, &videoInfo);
    
    CMSampleTimingInfo timing = { CMTimeMake(1, 60), CMTimeMake(vf.pts * 1000, 1000), kCMTimeInvalid };
    
    CMSampleBufferRef sampleBuffer = NULL;
    CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault, pixelBuffer, videoInfo, &timing, &sampleBuffer);
    
    if (videoInfo) CFRelease(videoInfo);
    CFRelease(pixelBuffer);
    
    return sampleBuffer;
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

    rav::VideoFrame vf;
    if (_engine->sync_pop_video_frame(vf)) {
        @synchronized(self) {
            _hasLastPoppedFrame = YES;
            _lastPoppedFrame.frame.reset(av_frame_clone(vf.frame.get()));
            _lastPoppedFrame.pts = vf.pts;
            _lastPoppedFrame.width = vf.width;
            _lastPoppedFrame.height = vf.height;
            _lastPoppedFrame.format = vf.format;
        }

        _renderer->present_frame(vf);
        
        if (_isPiPActive && vf.frame) {
            CachedFrame tempFrame;
            tempFrame.frame.reset(av_frame_clone(vf.frame.get()));
            tempFrame.pts = vf.pts;
            tempFrame.width = vf.width;
            tempFrame.height = vf.height;
            tempFrame.format = vf.format;
            
            CMSampleBufferRef sampleBuffer = [self createSampleBufferFromVideoFrame:tempFrame];
            if (sampleBuffer) {
                if ([_sampleBufferDisplayLayer isReadyForMoreMediaData]) {
                    [_sampleBufferDisplayLayer enqueueSampleBuffer:sampleBuffer];
                }
                CFRelease(sampleBuffer);
            }
        }
    } else if (frameCount % 30 == 1) {
        NSLog(@"renderFrame: sync_pop_video_frame returned false");
    }
}

- (void)startPiP {
    _isPiPActive = YES;
    _sampleBufferDisplayLayer.hidden = NO;
    
    CachedFrame vf;
    BOOL hasFrame = NO;
    @synchronized(self) {
        hasFrame = _hasLastPoppedFrame;
        if (hasFrame && _lastPoppedFrame.frame) {
            vf.frame.reset(av_frame_clone(_lastPoppedFrame.frame.get()));
            vf.pts = _lastPoppedFrame.pts;
            vf.width = _lastPoppedFrame.width;
            vf.height = _lastPoppedFrame.height;
            vf.format = _lastPoppedFrame.format;
        } else {
            hasFrame = NO;
        }
    }
    if (hasFrame) {
        CMSampleBufferRef sampleBuffer = [self createSampleBufferFromVideoFrame:vf];
        if (sampleBuffer) {
            [_sampleBufferDisplayLayer enqueueSampleBuffer:sampleBuffer];
            CFRelease(sampleBuffer);
        }
    }
    [CATransaction flush];
}

- (void)stopPiP {
    _isPiPActive = NO;
    [_sampleBufferDisplayLayer flushAndRemoveImage];
    _sampleBufferDisplayLayer.hidden = YES;
    [CATransaction flush];
}

@end
