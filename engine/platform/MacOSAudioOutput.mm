#include "MacOSAudioOutput.h"

namespace rav {

bool MacOSAudioOutput::init(const AudioOutputSpec& spec) {
    if (initialized_) shutdown();

    spec_ = spec;

    asbd_.mSampleRate = static_cast<double>(spec.sample_rate);
    asbd_.mFormatID = kAudioFormatLinearPCM;
    asbd_.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    asbd_.mBytesPerPacket = spec.channels * sizeof(float);
    asbd_.mFramesPerPacket = 1;
    asbd_.mBytesPerFrame = spec.channels * sizeof(float);
    asbd_.mChannelsPerFrame = static_cast<UInt32>(spec.channels);
    asbd_.mBitsPerChannel = 32;
    asbd_.mReserved = 0;

    OSStatus status = AudioQueueNewOutput(&asbd_, audio_queue_callback,
                                           this, NULL,
                                           kCFRunLoopCommonModes, 0, &queue_);
    if (status != noErr || !queue_) return false;

    UInt32 bufferSize = spec.frames_per_buffer * asbd_.mBytesPerFrame;
    for (int i = 0; i < 3; ++i) {
        status = AudioQueueAllocateBuffer(queue_, bufferSize, &buffers_[i]);
        if (status != noErr) {
            shutdown();
            return false;
        }
        std::memset(buffers_[i]->mAudioData, 0, buffers_[i]->mAudioDataBytesCapacity);
        buffers_[i]->mAudioDataByteSize = buffers_[i]->mAudioDataBytesCapacity;
    }

    AudioQueueSetParameter(queue_, kAudioQueueParam_Volume, 1.0);
    initialized_ = true;
    return true;
}

void MacOSAudioOutput::shutdown() {
    if (queue_) {
        AudioQueueStop(queue_, true);
        for (int i = 0; i < 3; ++i) {
            if (buffers_[i]) {
                AudioQueueFreeBuffer(queue_, buffers_[i]);
                buffers_[i] = nullptr;
            }
        }
        AudioQueueDispose(queue_, false);
        queue_ = nullptr;
    }
    playing_ = false;
    initialized_ = false;
}

bool MacOSAudioOutput::play() {
    if (!initialized_ || playing_) return false;

    for (int i = 0; i < 3; ++i) {
        if (fill_cb_) {
            int frames = fill_cb_(static_cast<uint8_t*>(buffers_[i]->mAudioData),
                                  spec_.frames_per_buffer);
            buffers_[i]->mAudioDataByteSize = frames * asbd_.mBytesPerFrame;
        }
        AudioQueueEnqueueBuffer(queue_, buffers_[i], 0, nullptr);
    }

    OSStatus status = AudioQueueStart(queue_, nullptr);
    if (status == noErr) {
        playing_ = true;
        return true;
    }
    return false;
}

bool MacOSAudioOutput::pause() {
    if (!playing_) return false;
    OSStatus status = AudioQueuePause(queue_);
    if (status == noErr) {
        playing_ = false;
        return true;
    }
    return false;
}

bool MacOSAudioOutput::resume() {
    if (!initialized_ || playing_) return false;
    OSStatus status = AudioQueueStart(queue_, nullptr);
    if (status == noErr) {
        playing_ = true;
        return true;
    }
    return false;
}

bool MacOSAudioOutput::stop() {
    if (!initialized_) return false;
    AudioQueueStop(queue_, true);
    playing_ = false;
    return true;
}

void MacOSAudioOutput::audio_queue_callback(void* user_data, AudioQueueRef queue,
                                             AudioQueueBufferRef buffer) {
    auto* self = static_cast<MacOSAudioOutput*>(user_data);
    if (!self) return;

    if (self->fill_cb_) {
        int frames = self->fill_cb_(static_cast<uint8_t*>(buffer->mAudioData),
                                     self->spec_.frames_per_buffer);
        buffer->mAudioDataByteSize = frames * self->asbd_.mBytesPerFrame;
    }

    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

} // namespace rav
