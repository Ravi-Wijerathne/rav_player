# Project Overview

Project Name: rav_player

Mission:

Build a professional-grade media player capable of playing virtually any media format while providing a fully native user experience exclusively on:

* macOS

The architecture prioritizes:

* Maximum codec compatibility
* Native macOS integration
* Metal hardware acceleration
* Long-term maintainability
* Extensibility
* Performance
* Dedicated C++ playback engine

The project follows a:

"C++ Media Engine + SwiftUI Native UI"

architecture.

---

# Architectural Philosophy

The application consists of two major layers:

1. Shared Media Engine
2. Platform-Specific Applications

The media engine contains all playback logic and is shared across every platform.

Each platform provides only:

* User interface
* Window management
* Native integrations
* Platform services

All media functionality remains centralized.

---

# High-Level Architecture

┌─────────────────────────────┐
│ macOS App (SwiftUI)         │
└──────────────┬──────────────┘
│
┌──────────────▼──────────────┐
│ Media Engine API            │
└──────────────┬──────────────┘
│
┌──────────────▼──────────────┐
│ Shared Media Engine (C++)   │
└──────────────┬──────────────┘
│
┌──────────────▼──────────────┐
│ FFmpeg                      │
└─────────────────────────────┘

---

# Technology Stack

## Core Engine

Language:

C++20

Reason:

* Mature multimedia ecosystem
* Excellent FFmpeg integration
* Native interoperability
* Predictable performance

---

## Media Framework

FFmpeg

Responsibilities:

* Demuxing
* Decoding
* Encoding (future)
* Stream parsing
* Metadata extraction

---

## macOS

UI:

SwiftUI

Hardware Decoder:

VideoToolbox

Graphics:

Metal

Bridge:

Objective-C++

---

# Repository Structure

root/

├── engine/
│
├── platform/
│   └── macos/
│
├── docs/
│
├── tests/
│
└── third_party/

---

# Engine Architecture

engine/

├── core/
├── media/
├── ffmpeg/
├── video/
├── audio/
├── subtitles/
├── streaming/
├── playlist/
├── metadata/
├── plugins/
├── platform/
├── rendering/
└── utilities/

---

# Core Layer

Purpose:

Central application logic.

Responsibilities:

* Playback state
* Command handling
* Event system
* Session management

States:

Idle

Loading

Playing

Paused

Seeking

Buffering

Stopped

Error

---

# Media Layer

Represents media resources.

Responsibilities:

* File abstraction
* URL abstraction
* Stream abstraction

Media Sources:

* Local file
* HTTP
* HTTPS
* HLS
* RTSP
* Network streams

---

# FFmpeg Layer

Purpose:

Encapsulate all FFmpeg interactions.

Responsibilities:

* Container reading
* Codec discovery
* Stream extraction
* Packet generation

No other module should communicate directly with FFmpeg.

All FFmpeg logic stays here.

---

# Demuxing Pipeline

Media Source
↓
Container Reader
↓
Demuxer
↓
Packets

Output:

* Video packets
* Audio packets
* Subtitle packets

---

# Decoder Layer

Purpose:

Convert compressed packets into raw media data.

Input:

Compressed packets

Output:

Frames

Components:

VideoDecoder

AudioDecoder

SubtitleDecoder

---

# Video System

video/

Responsibilities:

* Frame queues
* Timing
* Synchronization
* Rendering preparation

Objects:

VideoFrame

FrameQueue

VideoClock

VideoRenderer

---

# Audio System

audio/

Responsibilities:

* Audio buffering
* Audio output
* Audio synchronization

Objects:

AudioFrame

AudioQueue

AudioMixer

AudioOutput

AudioClock

---

# Synchronization System

Critical subsystem.

Audio becomes master clock.

Reason:

Humans tolerate delayed video more easily than delayed audio.

Synchronization Flow:

Audio Clock
↓
Video Timing Adjustment
↓
Frame Presentation

Supported:

* Frame dropping
* Frame duplication
* Clock correction

---

# Subtitle System

Responsibilities:

* Parsing
* Timing
* Rendering

Supported Formats:

* SRT
* ASS
* SSA
* VTT

Future:

PGS

DVD subtitles

Blu-ray subtitles

---

# Rendering System

Purpose:

Display video frames.

Pipeline:

Decoded Frame
↓
Pixel Conversion
↓
GPU Upload
↓
Render Surface
↓
Display

---

# Color Conversion

Video formats:

* YUV420P
* NV12
* YUV422

Display formats:

* RGB
* RGBA

Conversion performed before rendering.

---

# Hardware Acceleration Layer

Abstract interface.

class HardwareDecoder

Platform implementations:

VideoToolboxDecoder

---

# Rendering Backend Abstraction

class Renderer

Implementations:

MetalRenderer

The engine communicates only with Renderer.

Never directly with platform APIs.

---

# Streaming System

streaming/

Responsibilities:

* Network playback
* Live streams
* Adaptive streams

Protocols:

HTTP

HTTPS

HLS

RTSP

MPEG-TS

Future:

DASH

WebRTC

---

# Buffering System

Responsibilities:

* Prevent stuttering
* Smooth playback

Buffers:

Network Buffer

Packet Buffer

Audio Buffer

Video Buffer

---

# Playlist System

Responsibilities:

* Queue management
* Shuffle
* Repeat
* Playlist persistence

Objects:

Playlist

PlaylistItem

PlaybackQueue

---

# Metadata System

Responsibilities:

* Title extraction
* Duration extraction
* Artwork extraction

Data:

Title

Artist

Album

Bitrate

Codec

Resolution

Duration

---

# Plugin System

Goal:

Allow future extensions without modifying engine code.

Plugin Categories:

Visualization

Metadata

Streaming Providers

Subtitle Providers

Playback Enhancements

---

# Event System

Communication model.

PlayerCommand

Examples:

Play

Pause

Seek

Stop

LoadMedia

Next

Previous

---

PlayerEvent

Examples:

PlaybackStarted

PlaybackPaused

MediaLoaded

BufferingStarted

PlaybackEnded

ErrorOccurred

---

# Thread Architecture

Main UI Thread

Playback Thread

Audio Thread

Video Decode Thread

Audio Decode Thread

Subtitle Thread

Network Thread

Metadata Thread

Plugin Thread

Communication:

Message queues

Never direct thread access.

---

# Error Recovery

Engine must assume media files are damaged.

Recoverable:

Corrupted packet

Missing subtitle stream

Network interruption

Unsupported metadata

Fatal:

Unsupported codec

Missing decoder

Invalid container

---

# Logging System

Levels:

Trace

Debug

Info

Warning

Error

Fatal

Every subsystem must log independently.

---

# Testing Architecture

Unit Tests

Integration Tests

Codec Tests

Playback Tests

Streaming Tests

Performance Tests

Cross-Platform Tests

---

# Future Features

HDR

HDR10

HDR10+

Dolby Vision

Spatial Audio

Audio Visualization

Picture-in-Picture

AirPlay

Chromecast

DLNA

IPTV

Media Library

Cloud Sync

Plugin Marketplace

---

# Golden Rule

The UI never performs playback.

The UI never decodes media.

The UI never handles synchronization.

The UI never touches FFmpeg.

The Shared Media Engine owns all media functionality.

Platform applications are thin clients responsible only for presentation and platform integration.

Everything related to media playback lives inside the engine.

This rule must never be violated.
