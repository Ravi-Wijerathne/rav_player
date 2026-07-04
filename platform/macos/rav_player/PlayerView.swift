import SwiftUI
import UniformTypeIdentifiers

struct PlayerView: View {
    @EnvironmentObject var viewModel: PlayerViewModel
    @State private var isDragging = false
    @State private var dragTime: Double = 0

    var body: some View {
        VStack(spacing: 0) {
            videoArea

            if viewModel.isPlayable {
                controlsArea
            } else {
                dropArea
            }
        }
        .background(Color.black)
    }

    @ViewBuilder
    private var videoArea: some View {
        if viewModel.hasVideo {
            MetalVideoView()
                .aspectRatio(16.0 / 9.0, contentMode: .fit)
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
        }
        .foregroundColor(.white)
    }

    private var playbackButtons: some View {
        HStack(spacing: 20) {
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
}
