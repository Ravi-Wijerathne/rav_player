import SwiftUI
import UniformTypeIdentifiers

class PlayerViewModel: ObservableObject {
    @Published var state: PlayerBridgeState = .idle
    @Published var duration: Double = 0.0
    @Published var currentTime: Double = 0.0
    @Published var volume: Float = 1.0
    @Published var hasVideo: Bool = false
    @Published var hasAudio: Bool = false
    @Published var currentURL: String = ""
    @Published var isPlayable: Bool = false

    private let bridge = PlayerBridge()
    private var displayLink: CVDisplayLink?
    private var timeObserver: Timer?
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
        currentURL = url.absoluteString

        if bridge.open(url.absoluteString) {
            isPlayable = true
            duration = bridge.duration
            hasVideo = bridge.hasVideo
            hasAudio = bridge.hasAudio
            updateState()
            play()
        } else {
            isPlayable = false
            updateState()
        }
    }

    func play() {
        bridge.play()
        startTimeUpdates()
        updateState()
    }

    func pause() {
        bridge.pause()
        stopTimeUpdates()
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
        stopTimeUpdates()
        currentTime = 0
        updateState()
    }

    func seek(to seconds: Double) {
        bridge.seek(to: seconds)
    }

    func setVolume(_ vol: Float) {
        volume = vol
        bridge.volume = vol
    }

    func setupMetalLayer(_ layer: CAMetalLayer, width: Int, height: Int) {
        bridge.setupMetalLayer(UnsafeMutableRawPointer(Unmanaged.passUnretained(layer).toOpaque()),
                               width: Int32(width),
                               height: Int32(height))
        startDisplayLink()
    }

    func renderFrame() {
        bridge.renderFrame()
    }

    private func startDisplayLink() {
        stopDisplayLink()
        CVDisplayLinkCreateWithActiveCGDisplays(&displayLink)
        guard let displayLink = displayLink else { return }

        let viewModelPtr = Unmanaged.passUnretained(self).toOpaque()
        CVDisplayLinkSetOutputHandler(displayLink) { _, _, _, _, _ -> CVReturn in
            let vm = Unmanaged<PlayerViewModel>.fromOpaque(viewModelPtr).takeUnretainedValue()
            vm.renderFrame()
            return kCVReturnSuccess
        }
        CVDisplayLinkStart(displayLink)
    }

    private func stopDisplayLink() {
        if let link = displayLink {
            CVDisplayLinkStop(link)
            displayLink = nil
        }
    }

    private func startTimeUpdates() {
        stopTimeUpdates()
        timeObserver = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.currentTime = self.bridge.currentTime
                self.updateState()
            }
        }
    }

    private func stopTimeUpdates() {
        timeObserver?.invalidate()
        timeObserver = nil
    }

    private func updateState() {
        state = bridge.state
    }

    deinit {
        if let link = displayLink {
            CVDisplayLinkStop(link)
        }
        timeObserver?.invalidate()
        bridge.close()
    }
}
