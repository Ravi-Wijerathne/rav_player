#!/bin/bash
set -e

ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    HOMEBREW_PREFIX="/opt/homebrew"
elif [ "$ARCH" = "x86_64" ]; then
    HOMEBREW_PREFIX="/usr/local"
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi

FFMPEG_DIR="$HOMEBREW_PREFIX"

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
APP_DIR="$ROOT_DIR/platform/macos/rav_player.app"

echo "=== Building Engine ==="
cmake -B "$BUILD_DIR" -S "$ROOT_DIR" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR"

ENGINE_LIB="$BUILD_DIR/engine/librav_engine.a"
if [ ! -f "$ENGINE_LIB" ]; then
    echo "ERROR: Engine library not found at $ENGINE_LIB"
    exit 1
fi

echo "=== Building macOS App ==="
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

MACOS_DIR="$ROOT_DIR/platform/macos"

cp "$MACOS_DIR/rav_player/Info.plist" "$APP_DIR/Contents/"

# Compile Metal shaders
echo "  Compiling Metal shaders..."
xcrun -sdk macosx metal -c "$ROOT_DIR/engine/rendering/shaders.metal" \
    -o "$BUILD_DIR/shaders.air" -std=macos-metal2.3 2>&1
xcrun -sdk macosx metallib "$BUILD_DIR/shaders.air" \
    -o "$APP_DIR/Contents/Resources/shaders.metallib" 2>&1

FFMPEG_PREFIX="$HOMEBREW_PREFIX"

# Compile Obj-C++ bridge separately with clang++
BRIDGE_OBJ="$BUILD_DIR/PlayerBridge.o"
xcrun clang++ -x objective-c++ -std=c++20 \
    -I "$ROOT_DIR/engine" \
    -I "$ROOT_DIR/engine/core" \
    -I "$ROOT_DIR/engine/utilities" \
    -I "$ROOT_DIR/engine/ffmpeg" \
    -I "$ROOT_DIR/engine/media" \
    -I "$ROOT_DIR/engine/decoder" \
    -I "$ROOT_DIR/engine/video" \
    -I "$ROOT_DIR/engine/audio" \
    -I "$ROOT_DIR/engine/subtitles" \
    -I "$ROOT_DIR/engine/rendering" \
    -I "$ROOT_DIR/engine/platform" \
    -I "$ROOT_DIR/engine/playlist" \
    -I "$FFMPEG_PREFIX/include" \
    -fobjc-arc \
    -c "$MACOS_DIR/bridge/PlayerBridge.mm" \
    -o "$BRIDGE_OBJ" 2>&1

# Use xcrun swiftc with Swift files + precompiled bridge object
xcrun swiftc \
    "$MACOS_DIR/rav_player/rav_playerApp.swift" \
    "$MACOS_DIR/rav_player/ContentView.swift" \
    "$MACOS_DIR/rav_player/PlayerView.swift" \
    "$MACOS_DIR/rav_player/KeyboardShortcutManager.swift" \
    "$MACOS_DIR/rav_player/MetalVideoView.swift" \
    "$MACOS_DIR/rav_player/PlayerViewModel.swift" \
    "$MACOS_DIR/rav_player/PlaylistView.swift" \
    -import-objc-header "$MACOS_DIR/RavPlayer-Bridging-Header.h" \
    "$BRIDGE_OBJ" \
    -lc++ \
    -L "$BUILD_DIR/engine" -lrav_engine \
    -L "$FFMPEG_PREFIX/lib" \
    -lswresample -lavutil -lavcodec -lavformat -lswscale \
    -Xlinker -rpath -Xlinker "$FFMPEG_PREFIX/lib" \
    -Xlinker -rpath -Xlinker "$BUILD_DIR/engine" \
    -framework Metal -framework QuartzCore \
    -framework AudioToolbox -framework CoreAudio \
    -framework CoreVideo -framework AppKit \
    -framework AVFoundation -framework CoreMedia \
    -o "$APP_DIR/Contents/MacOS/rav_player" \
    2>&1

echo ""
echo "=== Build Complete ==="
echo "App bundle: $APP_DIR"
echo ""
echo "Run with: open '$APP_DIR'"

if [ -n "${CODESIGN_IDENTITY}" ]; then
    echo "Signing with identity: $CODESIGN_IDENTITY"
    codesign --deep --force --verify --timestamp \
        --options=runtime \
        --sign "$CODESIGN_IDENTITY" \
        "$APP_DIR/RavPlayer.app"
fi
