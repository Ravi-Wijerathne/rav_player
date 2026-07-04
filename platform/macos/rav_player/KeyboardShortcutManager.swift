import AppKit
import Carbon.HIToolbox

class KeyboardShortcutManager {
    static let shared = KeyboardShortcutManager()
    private var eventMonitor: Any?

    var onPlayPause: (() -> Void)?
    var onSeekForward: (() -> Void)?
    var onSeekBackward: (() -> Void)?

    private init() {}

    func startMonitoring() {
        if eventMonitor != nil { return }
        // Local monitor captures events only when the app is active
        eventMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            return self?.handleKeyEvent(event)
        }
    }

    func stopMonitoring() {
        if let monitor = eventMonitor {
            NSEvent.removeMonitor(monitor)
            eventMonitor = nil
        }
    }

    private func handleKeyEvent(_ event: NSEvent) -> NSEvent? {
        // Ignore key events if modifier keys (Command, Control, Option) are held
        guard !event.modifierFlags.contains(.command) && 
              !event.modifierFlags.contains(.control) &&
              !event.modifierFlags.contains(.option) else {
            return event
        }

        switch Int(event.keyCode) {
        case kVK_Space:
            onPlayPause?()
            return nil // Consume event
        case kVK_LeftArrow:
            onSeekBackward?()
            return nil // Consume event
        case kVK_RightArrow:
            onSeekForward?()
            return nil // Consume event
        default:
            return event
        }
    }
}
