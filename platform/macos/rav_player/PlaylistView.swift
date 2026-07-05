import SwiftUI

struct PlaylistView: View {
    @EnvironmentObject var viewModel: PlayerViewModel

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()
                .background(Color.gray.opacity(0.3))
            if viewModel.playlistItems.isEmpty {
                emptyState
            } else {
                listContent
            }
            Divider()
                .background(Color.gray.opacity(0.3))
            controls
        }
        .background(Color(white: 0.08))
        .frame(width: 280)
    }

    private var header: some View {
        HStack {
            Text("Playlist")
                .font(.headline)
                .foregroundColor(.white)
            Spacer()
            Text("\(viewModel.playlistItems.count) items")
                .font(.caption)
                .foregroundColor(.gray)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
    }

    private var emptyState: some View {
        VStack(spacing: 8) {
            Spacer()
            Image(systemName: "music.note.list")
                .font(.system(size: 32))
                .foregroundColor(.gray)
            Text("No items in playlist")
                .font(.caption)
                .foregroundColor(.gray)
            Spacer()
        }
        .frame(maxWidth: .infinity)
    }

    private var listContent: some View {
        ScrollView {
            LazyVStack(spacing: 2) {
                ForEach(Array(viewModel.playlistItems.enumerated()), id: \.element.id) { index, item in
                    PlaylistRowView(
                        index: index,
                        item: item,
                        isCurrent: index == viewModel.currentPlaylistIndex
                    )
                    .onTapGesture(count: 2) {
                        viewModel.playPlaylistItem(at: index)
                    }
                    .contextMenu {
                        Button(action: { viewModel.playPlaylistItem(at: index) }) {
                            Label("Play", systemImage: "play")
                        }
                        Divider()
                        Button(role: .destructive,
                               action: { viewModel.removePlaylistItem(at: index) }) {
                            Label("Remove", systemImage: "trash")
                        }
                    }
                }
            }
            .padding(.vertical, 4)
        }
    }

    private var controls: some View {
        HStack(spacing: 12) {
            Button(action: { viewModel.toggleShuffle() }) {
                Image(systemName: "shuffle")
                    .font(.title3)
                    .foregroundColor(viewModel.shuffleEnabled ? .green : .gray)
            }
            .buttonStyle(.plain)
            .help("Shuffle")

            Button(action: { viewModel.cycleRepeatMode() }) {
                Image(systemName: viewModel.repeatMode.icon)
                    .font(.title3)
                    .foregroundColor(viewModel.repeatMode != .none ? .green : .gray)
            }
            .buttonStyle(.plain)
            .help(viewModel.repeatMode.label)

            Spacer()

            Button(action: { viewModel.clearPlaylist() }) {
                Image(systemName: "trash")
                    .font(.caption)
                    .foregroundColor(.gray)
            }
            .buttonStyle(.plain)
            .help("Clear Playlist")
            .disabled(viewModel.playlistItems.isEmpty)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
    }
}

struct PlaylistRowView: View {
    let index: Int
    let item: PlaylistItemModel
    let isCurrent: Bool

    var body: some View {
        HStack(spacing: 8) {
            HStack(spacing: 6) {
                Text("\(index + 1)")
                    .font(.caption)
                    .foregroundColor(.gray)
                    .frame(width: 20, alignment: .trailing)

                VStack(alignment: .leading, spacing: 1) {
                    Text(item.title)
                        .font(.system(.body, design: .default))
                        .foregroundColor(isCurrent ? .green : .white)
                        .lineLimit(1)
                    if !item.artist.isEmpty {
                        Text(item.artist)
                            .font(.caption)
                            .foregroundColor(.gray)
                            .lineLimit(1)
                    }
                }
            }

            Spacer()

            Text(formatTime(item.duration))
                .font(.system(.caption, design: .monospaced))
                .foregroundColor(.gray)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 4)
        .background(isCurrent ? Color.green.opacity(0.1) : Color.clear)
        .contentShape(Rectangle())
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
}
