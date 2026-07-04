import SwiftUI

struct ContentView: View {
    @EnvironmentObject var viewModel: PlayerViewModel

    var body: some View {
        PlayerView()
            .frame(minWidth: 640, minHeight: 480)
            .background(Color.black)
            .preferredColorScheme(.dark)
    }
}
