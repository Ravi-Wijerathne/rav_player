import SwiftUI

@main
struct RavPlayerApp: App {
    @StateObject private var viewModel = PlayerViewModel()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(viewModel)
                .onAppear {
                    viewModel.setup()
                }
        }
        .windowResizability(.contentMinSize)
        .commands {
            CommandGroup(after: .newItem) {
                Button("Open File...") {
                    viewModel.openFile()
                }
                .keyboardShortcut("o", modifiers: .command)

                Button("Open URL...") {
                    viewModel.openURL()
                }
                .keyboardShortcut("o", modifiers: [.command, .shift])
            }

            CommandMenu("Playback") {
                Button("Play / Pause") {
                    viewModel.togglePlayPause()
                }
                .keyboardShortcut(.space, modifiers: [])

                Button("Stop") {
                    viewModel.stop()
                }
                .keyboardShortcut(".", modifiers: .command)
            }
        }
    }
}
