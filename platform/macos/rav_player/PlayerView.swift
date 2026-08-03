import SwiftUI
import UniformTypeIdentifiers

struct PlayerView: View {
    @EnvironmentObject var viewModel: PlayerViewModel
    @State private var isDragging = false
    @State private var dragTime: Double = 0
    @State private var osdText: String?
    @State private var osdTimer: Timer?
    @State private var isFullScreen: Bool = false
    @State private var showControls: Bool = true
    @State private var mouseTimer: Timer?
    @State private var showInfo: Bool = false

    var body: some View {
        ZStack {
            VStack(spacing: 0) {
                if viewModel.isPlayable {
                    videoArea
                    if !isFullScreen {
                        controlsArea
                    }
                } else {
                    dropArea
                }
            }
            .background(Color.black)

            if viewModel.isPlayable && isFullScreen {
                VStack {
                    Spacer()
                    if showControls {
                        controlsArea
                            .transition(.opacity)
                    }
                }
            }

            if let osdText = osdText {
                Text(osdText)
                    .font(.system(size: 32, weight: .semibold, design: .rounded))
                    .foregroundColor(.white)
                    .padding(.horizontal, 24)
                    .padding(.vertical, 16)
                    .background(Color.black.opacity(0.6))
                    .cornerRadius(12)
                    .transition(.opacity)
                    .allowsHitTesting(false)
            }

            if let error = viewModel.errorMessage {
                VStack {
                    Text("Error")
                        .font(.headline)
                        .foregroundColor(.red)
                    Text(error)
                        .font(.body)
                        .foregroundColor(.secondary)
                }
                .padding()
                .background(.regularMaterial)
                .cornerRadius(8)
            }

            if showInfo {
                infoOverlay
            }
        }
        .onContinuousHover { phase in
            if isFullScreen {
                switch phase {
                case .active:
                    resetMouseTimer()
                case .ended:
                    break
                }
            }
        }
        .onReceive(NotificationCenter.default.publisher(for: NSWindow.didEnterFullScreenNotification)) { _ in
            isFullScreen = true
            resetMouseTimer()
        }
        .onReceive(NotificationCenter.default.publisher(for: NSWindow.didExitFullScreenNotification)) { _ in
            isFullScreen = false
            showControls = true
            mouseTimer?.invalidate()
        }
        .onAppear {
            setupKeyboardShortcuts()
        }
        .onDisappear {
            KeyboardShortcutManager.shared.stopMonitoring()
        }
    }

    private var logoImage: Image? {
        if let url = Bundle.main.url(forResource: "logo", withExtension: "png"),
           let nsImg = NSImage(contentsOf: url) {
            return Image(nsImage: nsImg)
        }
        return nil
    }

    @ViewBuilder
    private var videoArea: some View {
        if viewModel.hasVideo {
            MetalVideoView()
                .aspectRatio(viewModel.videoAspectRatio, contentMode: .fit)
                .overlay(subtitleOverlay)
                .overlay(headerBrandingOverlay, alignment: .topLeading)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        } else if viewModel.hasAudio {
            audioPlaceholder
        } else {
            emptyArea
        }
    }

    private var headerBrandingOverlay: some View {
        HStack(spacing: 8) {
            if let logo = logoImage {
                logo
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(height: 24)
            }
            if !viewModel.mediaTitle.isEmpty {
                Text(viewModel.mediaTitle)
                    .font(.system(size: 13, weight: .semibold, design: .rounded))
                    .foregroundColor(.white.opacity(0.9))
                    .lineLimit(1)
            }
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 6)
        .background(Color.black.opacity(0.55))
        .cornerRadius(8)
        .padding(12)
        .allowsHitTesting(false)
    }

    private var audioPlaceholder: some View {
        ZStack {
            LinearGradient(
                colors: [Color(red: 0.05, green: 0.07, blue: 0.15), Color.black],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            
            VStack(spacing: 16) {
                if let logo = logoImage {
                    logo
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .frame(maxWidth: 280, maxHeight: 180)
                        .shadow(color: Color.blue.opacity(0.4), radius: 20, x: 0, y: 10)
                } else {
                    Image(systemName: "music.note")
                        .font(.system(size: 64))
                        .foregroundColor(.cyan)
                }
                
                if !viewModel.mediaTitle.isEmpty {
                    Text(viewModel.mediaTitle)
                        .font(.title2.bold())
                        .foregroundColor(.white)
                }
                if !viewModel.mediaArtist.isEmpty {
                    Text(viewModel.mediaArtist)
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
            }
            .padding()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    private var emptyArea: some View {
        ZStack {
            Color(white: 0.05)
            
            VStack(spacing: 24) {
                if let logo = logoImage {
                    logo
                        .resizable()
                        .aspectRatio(contentMode: .fit)
                        .frame(maxWidth: 340, maxHeight: 200)
                        .shadow(color: Color(red: 0.2, green: 0.4, blue: 0.9).opacity(0.35), radius: 25, x: 0, y: 12)
                } else {
                    Image(systemName: "film")
                        .font(.system(size: 64))
                        .foregroundColor(.gray)
                }

                VStack(spacing: 8) {
                    Text("Drop media file here")
                        .font(.system(size: 18, weight: .semibold, design: .rounded))
                        .foregroundColor(.white.opacity(0.9))

                    Text("or use File > Open (⌘O)")
                        .font(.system(size: 13, weight: .regular))
                        .foregroundColor(.gray)
                }
            }
            .padding(40)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .onDrop(of: [.fileURL], isTargeted: nil) { providers in
            handleDrop(providers)
            return true
        }
    }

    private var dropArea: some View {
        emptyArea
    }

    private var controlsArea: some View {
        VStack(spacing: 8) {
            seekBar

            HStack {
                timeDisplay
                Spacer()
                playbackButtons
                Spacer()
                volumeControl
            }
            .padding(.horizontal, 16)
            .padding(.bottom, 12)
        }
        .background(Color(white: 0.1))
    }

    private var seekBar: some View {
        Slider(
            value: Binding(
                get: { isDragging ? dragTime : viewModel.currentTime },
                set: { newValue in
                    if isDragging {
                        dragTime = newValue
                    } else {
                        // For a single click on the track without dragging
                        viewModel.seek(to: newValue)
                    }
                }
            ),
            in: 0...max(viewModel.duration, 1),
            onEditingChanged: { editing in
                isDragging = editing
                if editing {
                    dragTime = viewModel.currentTime
                } else {
                    viewModel.seek(to: dragTime)
                }
            }
        )
        .padding(.horizontal, 16)
        .padding(.top, 8)
        .accentColor(.white)
    }

    private var timeDisplay: some View {
        HStack(spacing: 4) {
            Text(formatTime(isDragging ? dragTime : viewModel.currentTime))
                .font(.system(.caption, design: .monospaced))
            Text("/")
                .font(.system(.caption, design: .monospaced))
                .foregroundColor(.gray)
            Text(formatTime(viewModel.duration))
                .font(.system(.caption, design: .monospaced))
                .foregroundColor(.gray)
            if viewModel.isSeeking {
                Text("Seeking...")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .foregroundColor(.white)
    }

    private var playbackButtons: some View {
        HStack(spacing: 16) {
            Button(action: { viewModel.previousTrack() }) {
                Image(systemName: "backward.fill")
                    .font(.title3)
            }
            .buttonStyle(.plain)
            .disabled(!viewModel.hasPreviousTrack)

            Button(action: { viewModel.stop() }) {
                Image(systemName: "stop.fill")
                    .font(.title3)
            }
            .buttonStyle(.plain)
            .disabled(!viewModel.isPlayable)

            Button(action: { viewModel.togglePlayPause() }) {
                Image(systemName: viewModel.isPlaying ? "pause.fill" : "play.fill")
                    .font(.title)
            }
            .buttonStyle(.plain)
            .disabled(!viewModel.isPlayable)

            Button(action: { viewModel.nextTrack() }) {
                Image(systemName: "forward.fill")
                    .font(.title3)
            }
            .buttonStyle(.plain)
            .disabled(!viewModel.hasNextTrack)

            Button(action: { viewModel.togglePlaylist() }) {
                Image(systemName: "list.bullet")
                    .font(.title3)
                    .foregroundColor(viewModel.showPlaylist ? .green : .white)
            }
            .buttonStyle(.plain)

            if viewModel.canStartPiP {
                Button(action: { viewModel.togglePiP() }) {
                    Image(systemName: viewModel.isPiPActive ? "pip.exit" : "pip.enter")
                        .font(.title3)
                        .foregroundColor(viewModel.isPiPActive ? .blue : .white)
                }
                .buttonStyle(.plain)
            }
        }
        .foregroundColor(.white)
    }

    private var volumeControl: some View {
        HStack(spacing: 6) {
            Image(systemName: viewModel.volume > 0 ? "speaker.wave.2.fill" : "speaker.slash.fill")
                .font(.caption)
                .foregroundColor(.gray)

            Slider(value: $viewModel.volume, in: 0...1, onEditingChanged: { _ in
                viewModel.setVolume(viewModel.volume)
            })
            .frame(width: 80)
            .accentColor(.white)
        }
    }

    private func formatTime(_ seconds: Double) -> String {
        guard seconds.isFinite && !seconds.isNaN else { return "0:00" }
        let total = Int(seconds)
        let h = total / 3600
        let m = (total % 3600) / 60
        let s = total % 60
        if h > 0 {
            return String(format: "%d:%02d:%02d", h, m, s)
        }
        return String(format: "%d:%02d", m, s)
    }

    @ViewBuilder
    private var infoOverlay: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text("Media Info")
                    .font(.headline)
                    .foregroundColor(.white)
                Spacer()
                Button(action: { showInfo = false }) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(.gray)
                }
                .buttonStyle(.plain)
            }
            .padding(.bottom, 4)

            if !viewModel.mediaTitle.isEmpty {
                infoRow(label: "Title", value: viewModel.mediaTitle)
            }
            if !viewModel.mediaArtist.isEmpty {
                infoRow(label: "Artist", value: viewModel.mediaArtist)
            }
            if !viewModel.mediaAlbum.isEmpty {
                infoRow(label: "Album", value: viewModel.mediaAlbum)
            }
            if !viewModel.videoCodecName.isEmpty {
                infoRow(label: "Video", value: viewModel.videoCodecName)
            }
            if !viewModel.audioCodecName.isEmpty {
                infoRow(label: "Audio", value: viewModel.audioCodecName)
            }
            if viewModel.mediaBitrate > 0 {
                infoRow(label: "Bitrate", value: "\(viewModel.mediaBitrate / 1000) kbps")
            }
            infoRow(label: "Duration", value: formatTime(viewModel.duration))
            infoRow(label: "Resolution",
                    value: "\(viewModel.videoWidth)×\(viewModel.videoHeight)")
        }
        .padding(16)
        .background(Color.black.opacity(0.75))
        .cornerRadius(12)
        .padding(40)
        .allowsHitTesting(true)
    }

    private func infoRow(label: String, value: String) -> some View {
        HStack(spacing: 8) {
            Text(label + ":")
                .font(.system(.caption, design: .monospaced))
                .foregroundColor(.gray)
                .frame(width: 60, alignment: .trailing)
            Text(value)
                .font(.system(.caption, design: .monospaced))
                .foregroundColor(.white)
            Spacer()
        }
    }

    @ViewBuilder
    private var subtitleOverlay: some View {
        if !viewModel.subtitles.isEmpty {
            GeometryReader { geo in
                let scaleX = geo.size.width / CGFloat(max(1, viewModel.videoWidth))
                let scaleY = geo.size.height / CGFloat(max(1, viewModel.videoHeight))
                
                ZStack(alignment: .topLeading) {
                    // 1. Bitmap subtitles mapped by absolute coordinates
                    ForEach(viewModel.subtitles.filter { $0.isBitmap }) { sub in
                        if let img = sub.image {
                            Image(nsImage: img)
                                .resizable()
                                .frame(width: CGFloat(sub.width) * scaleX, height: CGFloat(sub.height) * scaleY)
                                .offset(x: CGFloat(sub.x) * scaleX, y: CGFloat(sub.y) * scaleY)
                        }
                    }
                    
                    // 2. Text subtitles stacked at the bottom
                    let textSubs = viewModel.subtitles.filter { !$0.isBitmap && !$0.text.isEmpty }
                    if !textSubs.isEmpty {
                        VStack {
                            Spacer()
                            VStack(spacing: 4) {
                                ForEach(textSubs) { sub in
                                    Text(sub.text)
                                        .font(.system(size: 24, weight: .semibold))
                                        .foregroundColor(.white)
                                        .multilineTextAlignment(.center)
                                        .shadow(color: .black.opacity(0.9), radius: 3, x: 0, y: 1)
                                }
                            }
                            .padding(.horizontal, 24)
                            .padding(.vertical, 8)
                            .padding(.bottom, 24)
                        }
                        .frame(width: geo.size.width, height: geo.size.height)
                    }
                }
            }
            .allowsHitTesting(false)
        }
    }

    private func handleDrop(_ providers: [NSItemProvider]) {
        guard let provider = providers.first else { return }
        provider.loadItem(forTypeIdentifier: UTType.fileURL.identifier, options: nil) { item, _ in
            if let data = item as? Data,
               let url = URL(dataRepresentation: data, relativeTo: nil) {
                Task { @MainActor in
                    self.viewModel.loadMedia(url: url)
                }
            }
        }
    }

    private func setupKeyboardShortcuts() {
        KeyboardShortcutManager.shared.onPlayPause = {
            viewModel.togglePlayPause()
        }
        KeyboardShortcutManager.shared.onSeekForward = {
            let target = min(viewModel.currentTime + 10, viewModel.duration)
            viewModel.seek(to: target)
            showOSD(text: "Forward 10s")
        }
        KeyboardShortcutManager.shared.onSeekBackward = {
            let target = max(viewModel.currentTime - 10, 0)
            viewModel.seek(to: target)
            showOSD(text: "Backward 10s")
        }
        KeyboardShortcutManager.shared.onToggleInfo = {
            showInfo.toggle()
        }
        KeyboardShortcutManager.shared.onTogglePlaylist = {
            viewModel.togglePlaylist()
        }
        KeyboardShortcutManager.shared.startMonitoring()
    }

    private func resetMouseTimer() {
        if !showControls {
            withAnimation(.easeInOut(duration: 0.3)) {
                showControls = true
            }
            NSCursor.unhide()
        }
        mouseTimer?.invalidate()
        
        if isFullScreen {
            mouseTimer = Timer.scheduledTimer(withTimeInterval: 2.5, repeats: false) { _ in
                guard !isDragging else {
                    resetMouseTimer()
                    return
                }
                withAnimation(.easeInOut(duration: 0.5)) {
                    showControls = false
                }
                NSCursor.setHiddenUntilMouseMoves(true)
            }
        }
    }

    private func showOSD(text: String) {
        osdTimer?.invalidate()
        withAnimation(.easeIn(duration: 0.1)) {
            osdText = text
        }
        osdTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: false) { _ in
            withAnimation(.easeOut(duration: 0.5)) {
                osdText = nil
            }
        }
    }
}
