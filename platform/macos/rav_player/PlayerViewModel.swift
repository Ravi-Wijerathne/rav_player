import SwiftUI
import UniformTypeIdentifiers

struct PlaylistItemModel: Identifiable {
    let id = UUID()
    let uri: String
    let title: String
    let artist: String
    let duration: Double
    var isCurrent: Bool = false
}

enum RepeatMode: Int, CaseIterable {
    case none = 0
    case one = 1
    case all = 2

    var label: String {
        switch self {
        case .none: return "No Repeat"
        case .one:  return "Repeat One"
        case .all:  return "Repeat All"
        }
    }

    var icon: String {
        switch self {
        case .none: return "repeat"
        case .one:  return "repeat.1"
        case .all:  return "repeat"
        }
    }
}

class PlayerViewModel: ObservableObject {
    @Published var state: PlayerBridgeState = .idle
    @Published var duration: Double = 0.0
    @Published var currentTime: Double = 0.0
    @Published var volume: Float = 1.0
    @Published var hasVideo: Bool = false
    @Published var hasAudio: Bool = false
    @Published var hasSubtitles: Bool = false
    @Published var currentURL: String = ""
    @Published var isPlayable: Bool = false
    @Published var videoWidth: Int = 0
    @Published var videoHeight: Int = 0
    @Published var subtitleTexts: [String] = []
    @Published var mediaTitle: String = ""
    @Published var mediaArtist: String = ""
    @Published var mediaAlbum: String = ""
    @Published var mediaBitrate: Int = 0
    @Published var videoCodecName: String = ""
    @Published var audioCodecName: String = ""
    @Published var playlistItems: [PlaylistItemModel] = []
    @Published var currentPlaylistIndex: Int = -1
    @Published var shuffleEnabled: Bool = false
    @Published var repeatMode: RepeatMode = .none
    @Published var showPlaylist: Bool = false
    @Published var hasNextTrack: Bool = false
    @Published var hasPreviousTrack: Bool = false
    @Published var errorMessage: String?
    @Published var isSeeking: Bool = false
    @Published var isBuffering: Bool = false
    var videoAspectRatio: CGFloat {
        guard videoWidth > 0 && videoHeight > 0 else { return 16.0 / 9.0 }
        return CGFloat(videoWidth) / CGFloat(videoHeight)
    }

    private let bridge = PlayerBridge()
    private var eventTimer: Timer?
    var isPlaying: Bool { state == .playing }
    var isPaused: Bool { state == .paused }

    func setup() {
        updateState()
    }

    func openFile() {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.allowedContentTypes = [.audiovisualContent, .movie, .video, .audio]

        guard panel.runModal() == .OK, let url = panel.url else { return }
        loadMedia(url: url)
    }

    func openURL() {
        let alert = NSAlert()
        alert.messageText = "Open URL"
        alert.informativeText = "Enter a media URL:"
        alert.addButton(withTitle: "Open")
        alert.addButton(withTitle: "Cancel")

        let textField = NSTextField(frame: NSRect(x: 0, y: 0, width: 300, height: 24))
        textField.placeholderString = "https://example.com/video.mp4"
        alert.accessoryView = textField

        guard alert.runModal() == .alertFirstButtonReturn else { return }
        let urlString = textField.stringValue.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !urlString.isEmpty, let url = URL(string: urlString) else { return }
        loadMedia(url: url)
    }

    func loadMedia(url: URL) {
        let pathStr = url.isFileURL ? url.path : url.absoluteString
        currentURL = pathStr
        videoWidth = 0
        videoHeight = 0
        subtitleTexts = []

        // Add to playlist if not already present
        let isInPlaylist = playlistItems.contains(where: { $0.uri == pathStr })
        if !isInPlaylist {
            bridge.appendToPlaylist(pathStr)
        }

        if bridge.open(pathStr) {
            isPlayable = true
            duration = bridge.duration
            hasVideo = bridge.hasVideo
            hasAudio = bridge.hasAudio
            hasSubtitles = bridge.hasSubtitles
            mediaTitle = bridge.mediaTitle
            mediaArtist = bridge.mediaArtist
            mediaAlbum = bridge.mediaAlbum
            mediaBitrate = Int(bridge.mediaBitrate)
            videoCodecName = bridge.videoCodecName
            audioCodecName = bridge.audioCodecName
            syncPlaylist()
            updateState()
            play()
        } else {
            isPlayable = false
            updateState()
        }
    }

    func play() {
        bridge.play()
        startEventPolling()
        updateState()
    }

    func pause() {
        bridge.pause()
        stopEventPolling()
        updateState()
    }

    func togglePlayPause() {
        if isPlaying {
            pause()
        } else if isPaused || state == .stopped {
            play()
        }
    }

    func stop() {
        bridge.stop()
        stopEventPolling()
        currentTime = 0
        subtitleTexts = []
        updateState()
    }

    func seek(to seconds: Double) {
        print("PlayerViewModel: seeking to \(seconds)")
        bridge.seek(to: seconds)
    }

    // ── Playlist ──

    func addToPlaylist(url: URL) {
        bridge.appendToPlaylist(url.isFileURL ? url.path : url.absoluteString)
        syncPlaylist()
    }

    func playPlaylistItem(at index: Int) {
        bridge.playPlaylistItem(at: index)
        syncPlaylist()
    }

    func nextTrack() {
        bridge.nextTrack()
        syncNowPlaying()
        syncPlaylist()
    }

    func previousTrack() {
        bridge.previousTrack()
        syncNowPlaying()
        syncPlaylist()
    }

    func removePlaylistItem(at index: Int) {
        bridge.removeFromPlaylist(at: index)
        syncPlaylist()
    }

    func clearPlaylist() {
        bridge.clearPlaylist()
        syncPlaylist()
    }

    func toggleShuffle() {
        let new = !shuffleEnabled
        bridge.setShuffle(new)
        shuffleEnabled = new
        syncPlaylist()
    }

    func cycleRepeatMode() {
        let allCases = RepeatMode.allCases
        let current = repeatMode
        let next = allCases[(current.rawValue + 1) % allCases.count]
        bridge.setRepeatMode(PlayerRepeatMode(rawValue: next.rawValue)!)
        repeatMode = next
    }

    func togglePlaylist() {
        withAnimation(.easeInOut(duration: 0.2)) {
            showPlaylist.toggle()
        }
    }

    private func syncPlaylist() {
        playlistItems = bridge.playlistItems.enumerated().map { i, item in
            PlaylistItemModel(
                uri: item.uri,
                title: item.title,
                artist: item.artist,
                duration: item.duration,
                isCurrent: i == bridge.currentPlaylistIndex
            )
        }
        currentPlaylistIndex = bridge.currentPlaylistIndex
        hasNextTrack = bridge.hasNextTrack
        hasPreviousTrack = bridge.hasPreviousTrack
        shuffleEnabled = bridge.shuffleEnabled
        repeatMode = RepeatMode(rawValue: Int(bridge.repeatMode.rawValue)) ?? .none
    }

    private func syncNowPlaying() {
        currentURL = bridge.currentURL as String? ?? ""
        duration = bridge.duration
        hasVideo = bridge.hasVideo
        hasAudio = bridge.hasAudio
        hasSubtitles = bridge.hasSubtitles
        mediaTitle = bridge.mediaTitle
        mediaArtist = bridge.mediaArtist
        mediaAlbum = bridge.mediaAlbum
        mediaBitrate = Int(bridge.mediaBitrate)
        videoCodecName = bridge.videoCodecName
        audioCodecName = bridge.audioCodecName
        videoWidth = Int(bridge.videoWidth)
        videoHeight = Int(bridge.videoHeight)
    }

    func setVolume(_ vol: Float) {
        volume = vol
        bridge.volume = vol
    }

    func setupMetalLayer(_ layer: CAMetalLayer, width: Int, height: Int) {
        bridge.setupMetalLayer(UnsafeMutableRawPointer(Unmanaged.passUnretained(layer).toOpaque()),
                               width: Int32(width),
                               height: Int32(height))
    }

    func resizeMetal(width: Int32, height: Int32) {
        bridge.resizeMetal(width, height: height)
    }

    func renderFrame() {
        bridge.renderFrame()
    }

    private func startEventPolling() {
        stopEventPolling()
        eventTimer = Timer.scheduledTimer(withTimeInterval: 0.2, repeats: true) { [weak self] _ in
            DispatchQueue.main.async {
                guard let self = self else { return }
                while let event = self.bridge.pollEvent() {
                    self.handleEvent(event)
                }
                self.currentTime = self.bridge.currentTime
                let w = Int(self.bridge.videoWidth)
                let h = Int(self.bridge.videoHeight)
                if w > 0 && h > 0 && (w != self.videoWidth || h != self.videoHeight) {
                    self.videoWidth = w
                    self.videoHeight = h
                }
                self.subtitleTexts = self.bridge.currentSubtitleTexts()
                self.updateState()
                self.currentPlaylistIndex = self.bridge.currentPlaylistIndex
                self.hasNextTrack = self.bridge.hasNextTrack
                self.hasPreviousTrack = self.bridge.hasPreviousTrack
                self.isSeeking = self.bridge.state == .seeking
                if self.bridge.state == .error {
                    self.errorMessage = self.bridge.lastErrorMessage() ?? "Unknown error"
                } else if self.errorMessage != nil {
                    self.errorMessage = nil
                }
            }
        }
    }

    private func stopEventPolling() {
        eventTimer?.invalidate()
        eventTimer = nil
    }

    private func handleEvent(_ eventName: String) {
        switch eventName {
        case "PlaybackEndedEvent":
            nextTrack()
        case "ErrorOccurredEvent":
            errorMessage = bridge.lastErrorMessage() ?? "Playback error"
        case "BufferingStartedEvent":
            isBuffering = true
        case "BufferingEndedEvent":
            isBuffering = false
        default:
            break
        }
    }

    private func updateState() {
        state = bridge.state
    }

    deinit {
        eventTimer?.invalidate()
        bridge.close()
    }
}
