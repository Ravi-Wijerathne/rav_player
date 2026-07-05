import SwiftUI

struct ContentView: View {
    @EnvironmentObject var viewModel: PlayerViewModel

    var body: some View {
        HStack(spacing: 0) {
            PlayerView()
                .frame(minWidth: 640, minHeight: 480)
                .layoutPriority(1)

            if viewModel.showPlaylist {
                PlaylistView()
                    .environmentObject(viewModel)
            }
        }
        .background(Color.black)
        .preferredColorScheme(.dark)
    }
}
