import SwiftUI
import UniformTypeIdentifiers

struct PlayerView: View {
    @EnvironmentObject var viewModel: PlayerViewModel
    @State private var isDragging = false
    @State private var dragTime: Double = 0
    @State private var osdText: String?
    @State private var osdTimer: Timer?
    @State private var showInfo: Bool = false

    var body: some View {
        ZStack {
            VStack(spacing: 0) {
                videoArea

                if viewModel.isPlayable {
                    controlsArea
                } else {
                    dropArea
                }
            }
            .background(Color.black)

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
        .onAppear {
            setupKeyboardShortcuts()
        }
        .onDisappear {
            KeyboardShortcutManager.shared.stopMonitoring()
        }
    }

    @ViewBuilder
    private var videoArea: some View {
        if viewModel.hasVideo {
            ZStack {
                MetalVideoView()
                    .aspectRatio(viewModel.videoAspectRatio, contentMode: .fit)

                subtitleOverlay
            }
        } else if viewModel.hasAudio {
            audioPlaceholder
        } else {
            emptyArea
        }
    }

    private var audioPlaceholder: some View {
        ZStack {
            Color.black
            Image(systemName: "music.note")
                .font(.system(size: 64))
                .foregroundColor(.gray)
        }
        .aspectRatio(16.0 / 9.0, contentMode: .fit)
    }

    private var emptyArea: some View {
        ZStack {
            Color.black
            Text("Drop media file here")
                .foregroundColor(.gray)
                .font(.title2)
        }
        .aspectRatio(16.0 / 9.0, contentMode: .fit)
        .onDrop(of: [.fileURL], isTargeted: nil) { providers in
            handleDrop(providers)
            return true
        }
    }

    private var dropArea: some View {
        ZStack {
            Color.black
            VStack(spacing: 16) {
                Image(systemName: "film")
                    .font(.system(size: 48))
                    .foregroundColor(.gray)
                Text("Drop media file or use File > Open")
                    .foregroundColor(.gray)
                    .font(.title3)
            }
        }
        .aspectRatio(16.0 / 9.0, contentMode: .fit)
        .onDrop(of: [.fileURL], isTargeted: nil) { providers in
            handleDrop(providers)
            return true
        }
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
            value: isDragging ? $dragTime : $viewModel.currentTime,
            in: 0...max(viewModel.duration, 1),
            onEditingChanged: { editing in
                if editing {
                    isDragging = true
                    dragTime = viewModel.currentTime
                } else {
                    isDragging = false
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
        if !viewModel.subtitleTexts.isEmpty {
            VStack {
                Spacer()
                VStack(spacing: 4) {
                    ForEach(viewModel.subtitleTexts, id: \.self) { text in
                        Text(text)
                            .font(.system(size: 18, weight: .semibold))
                            .foregroundColor(.white)
                            .multilineTextAlignment(.center)
                            .shadow(color: .black.opacity(0.9), radius: 3, x: 0, y: 1)
                    }
                }
                .padding(.horizontal, 24)
                .padding(.vertical, 8)
                .padding(.bottom, 24)
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
