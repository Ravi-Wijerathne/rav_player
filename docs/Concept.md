# Concepts.md

# Media Player Knowledge Roadmap

## Purpose

This document defines the concepts that must be understood in order to build a modern VLC-class media player.

The goal is not only to build a media player but to understand how multimedia systems work internally.

---

# Learning Order

Study concepts in this exact order.

1. Media Fundamentals
2. Containers
3. Codecs
4. FFmpeg Basics
5. Demuxing
6. Decoding
7. Rendering
8. Audio Playback
9. Synchronization
10. Subtitles
11. Hardware Acceleration
12. Streaming
13. Playlists
14. Plugins
15. Professional Media Features

---

# 1. Media Fundamentals

Before writing any code, understand what a video actually is.

A video is not a single thing.

A video contains:

* Video stream
* Audio stream
* Subtitle stream
* Metadata

Example:

Movie.mkv

Contains:

Video:

* H.264

Audio:

* AAC

Subtitles:

* English
* Spanish

Metadata:

* Title
* Duration

---

# 2. Containers

## What Is A Container?

A container stores streams together.

Think of it as a box.

Container:

* MP4
* MKV
* AVI
* MOV
* WebM

Inside:

* Video stream
* Audio stream
* Subtitle stream

---

## Important Understanding

MP4 is NOT a codec.

MP4 is a container.

Many beginners confuse containers and codecs.

Incorrect:

MP4 Video

Correct:

MP4 Container
containing
H.264 Video Codec

---

# 3. Codecs

## What Is A Codec?

Codec means:

Coder / Decoder

Purpose:

Compress media.

Without codecs:

A movie could consume hundreds of gigabytes.

---

## Common Video Codecs

H.264

Most common video codec.

Learn first.

---

H.265 / HEVC

Successor to H.264.

Better compression.

---

VP9

Google codec.

Used by YouTube.

---

AV1

Modern royalty-free codec.

Growing rapidly.

---

## Common Audio Codecs

AAC

Most common.

---

MP3

Classic audio codec.

---

FLAC

Lossless codec.

---

Opus

Modern internet audio codec.

---

# 4. FFmpeg

## What Is FFmpeg?

FFmpeg is a multimedia framework.

Responsibilities:

* Read media files
* Parse containers
* Decode streams
* Encode streams
* Convert formats

FFmpeg is the heart of many media applications.

Your player will rely heavily on FFmpeg.

---

# 5. Demuxing

## What Is Demuxing?

Demuxing means:

Separating streams from a container.

Example:

Movie.mkv

Contains:

* Video stream
* Audio stream
* Subtitle stream

Demuxer extracts them.

Result:

Video packets
Audio packets
Subtitle packets

---

Pipeline:

Container
↓
Demuxer
↓
Packets

---

# 6. Packets

## What Is A Packet?

Compressed media data.

Packets are not directly displayable.

Example:

H.264 packet

Cannot be rendered.

Must first be decoded.

---

Pipeline:

Packet
↓
Decoder
↓
Frame

---

# 7. Decoding

## What Is Decoding?

Converting compressed packets into raw data.

Example:

H.264 Packet
↓
Decoder
↓
Raw Video Frame

---

AAC Packet
↓
Decoder
↓
Raw Audio Samples

---

# 8. Frames

## Video Frame

Single image.

Example:

60 FPS video

Means:

60 frames per second.

---

# Frame Timing

Every frame has:

PTS

Presentation Timestamp

Tells player when frame should appear.

Critical concept.

Must understand deeply.

---

# 9. Rendering

## What Is Rendering?

Displaying decoded frames.

Pipeline:

Frame
↓
GPU Upload
↓
Renderer
↓
Screen

---

Topics:

* Textures
* GPU memory
* Frame buffers
* Scaling

---

# 10. Audio Playback

Audio is played using audio samples.

Pipeline:

Audio Packet
↓
Decode
↓
Samples
↓
Audio Device

---

Topics:

* Sample rate
* Channels
* Buffering

---

# 11. Audio Video Synchronization

One of the hardest topics.

---

Problem

Audio and video run independently.

Need synchronization.

---

Solution

Use audio as master clock.

Audio Time
↓
Video follows

---

Common Issues

* Audio ahead
* Video ahead
* Drift
* Stuttering

---

# 12. Seeking

User drags timeline.

Player must:

* Jump position
* Flush buffers
* Find nearest keyframe
* Continue playback

---

Topics:

* Keyframes
* Seek tables
* Timestamp conversion

---

# 13. Keyframes

Important concept.

A video cannot always start decoding from any frame.

Needs a keyframe.

---

Types:

I-frame
P-frame
B-frame

Understand differences.

Essential for seeking.

---

# 14. Subtitles

Types:

SRT
ASS
SSA
WebVTT

---

Tasks:

* Parse
* Synchronize
* Render

---

Advanced:

ASS subtitle styling.

---

# 15. Hardware Acceleration

Software decoding:

CPU

Hardware decoding:

GPU

---

Benefits:

* Lower CPU usage
* Lower power consumption
* Better battery life

---

Platforms

macOS:

* VideoToolbox

---

# 16. GPU Rendering

Topics:

* Textures
* Shaders
* Frame uploads
* Color conversion

Future library:

wgpu

---

# 17. Color Spaces

Video often uses:

YUV

Displays use:

RGB

Need conversion.

---

Important Formats

YUV420P
NV12
RGB24

---

# 18. Streaming

Media doesn't always come from files.

Can come from networks.

---

Protocols

HTTP

HTTPS

HLS

RTSP

MPEG-TS

---

Topics

* Buffering
* Latency
* Reconnection

---

# 19. Buffering

Purpose:

Prevent playback interruptions.

---

Buffer Stores:

Future audio data
Future video data

---

Problems:

* Network jitter
* Slow connection

---

# 20. Playlists

Concepts:

* Queue
* Repeat
* Shuffle
* Next
* Previous

---

# 21. Metadata

Examples:

Title
Artist
Duration
Resolution
Bitrate

---

Extraction via FFmpeg.

---

# 22. Plugin Systems

Purpose:

Extend functionality.

Possible Plugins:

* Visualizers
* Subtitle Providers
* Metadata Sources
* Streaming Sources

---

Topics:

* Dynamic libraries
* ABI compatibility
* Sandboxing

---

# 23. Threading

Media players are heavily multithreaded.

Possible Threads:

UI Thread

Audio Thread

Video Thread

Network Thread

Subtitle Thread

Decode Thread

---

Topics:

* Channels
* Message passing
* Synchronization

---

# 24. Error Handling

Media files are often corrupted.

Player must handle:

* Broken containers
* Missing codecs
* Invalid timestamps
* Network failures

Gracefully.

---

# 25. VLC-Class Features

Long-Term Topics

* HDR
* Dolby Vision
* AirPlay
* Chromecast
* IPTV
* DLNA
* Audio Visualization
* Plugin Marketplace
* Media Library

---

# Mastery Checklist

Media Fundamentals       [ ]
Containers               [ ]
Codecs                   [ ]
FFmpeg                   [ ]
Demuxing                 [ ]
Packets                  [ ]
Decoding                 [ ]
Frames                   [ ]
Rendering                [ ]
Audio Playback           [ ]
AV Synchronization       [ ]
Seeking                  [ ]
Keyframes                [ ]
Subtitles                [ ]
Hardware Acceleration    [ ]
GPU Rendering            [ ]
Color Spaces             [ ]
Streaming                [ ]
Buffering                [ ]
Playlists                [ ]
Metadata                 [ ]
Plugin Systems           [ ]
Threading                [ ]
Error Handling           [ ]
Professional Features    [ ]

---

Final Goal:

Understand the complete pipeline:

Media File
↓
Container
↓
Demuxer
↓
Packets
↓
Decoder
↓
Frames
↓
Renderer
↓
Screen

When this pipeline becomes intuitive, building a VLC-like media player becomes a software engineering challenge rather than a mystery.
