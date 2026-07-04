#include <atomic>
#include <memory>

#include "PlayerBridge.h"

#include "../../../engine/core/PlaybackEngine.h"
#include "../../../engine/rendering/MetalRenderer.h"
#include "../../../engine/platform/MacOSAudioOutput.h"

using namespace rav;

@interface PlayerBridge () {
    std::unique_ptr<PlaybackEngine> _engine;
    std::unique_ptr<MetalRenderer> _renderer;
    std::unique_ptr<MacOSAudioOutput> _audioOutput;
    dispatch_queue_t _renderQueue;
}
@end

@implementation PlayerBridge

- (instancetype)init {
    self = [super init];
    if (self) {
        _engine = std::make_unique<PlaybackEngine>();
        _renderer = std::make_unique<MetalRenderer>();
        _audioOutput = std::make_unique<MacOSAudioOutput>();
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
    return result;
}

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

- (PlayerBridgeState)state {
    switch (_engine->state()) {
        case PlayerState::Idle:    return PlayerBridgeStateIdle;
        case PlayerState::Loading: return PlayerBridgeStateLoading;
        case PlayerState::Playing: return PlayerBridgeStatePlaying;
        case PlayerState::Paused:  return PlayerBridgeStatePaused;
        case PlayerState::Stopped: return PlayerBridgeStateStopped;
        default:                   return PlayerBridgeStateError;
    }
}

- (double)duration { return _engine->duration(); }
- (double)currentTime { return _engine->current_time(); }
- (float)volume { return _engine->volume(); }
- (void)setVolume:(float)vol { _engine->set_volume(vol); }
- (BOOL)hasVideo { return _engine->has_video() ? YES : NO; }
- (BOOL)hasAudio { return _engine->has_audio() ? YES : NO; }

- (void)setupMetalLayer:(void*)metalLayer width:(int)width height:(int)height {
    if (!_renderer) return;
    _renderer->set_layer(metalLayer);
    _renderer->resize(width, height);
    _renderer->init();
}

- (void)renderFrame {
    if (!_engine || !_renderer || !_renderer->is_ready()) return;
    if (_engine->state() != PlayerState::Playing) return;

    VideoFrame vf;
    if (_engine->try_pop_video_frame(vf, 0)) {
        _renderer->present_frame(vf);
    }
}

@end
