import SwiftUI
import Metal

struct MetalVideoView: NSViewRepresentable {
    @EnvironmentObject var viewModel: PlayerViewModel

    func makeNSView(context: Context) -> MetalVideoNSView {
        let view = MetalVideoNSView()
        view.viewModel = viewModel
        return view
    }

    func updateNSView(_ nsView: MetalVideoNSView, context: Context) {
        nsView.viewModel = viewModel
    }
}

class MetalVideoNSView: NSView {
    let metalLayer = CAMetalLayer()
    weak var viewModel: PlayerViewModel?
    private var setupDone = false

    override init(frame: NSRect) {
        super.init(frame: frame)
        setupLayer()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupLayer()
    }

    private func setupLayer() {
        wantsLayer = true
        metalLayer.device = MTLCreateSystemDefaultDevice()
        metalLayer.pixelFormat = .bgra8Unorm
        metalLayer.framebufferOnly = true
        metalLayer.autoresizingMask = [.layerWidthSizable, .layerHeightSizable]
        layer = metalLayer
    }

    private func ensureMetalSetup() {
        let w = Int(metalLayer.bounds.width * metalLayer.contentsScale)
        let h = Int(metalLayer.bounds.height * metalLayer.contentsScale)
        guard w > 0 && h > 0 else { return }

        metalLayer.drawableSize = CGSize(width: CGFloat(w), height: CGFloat(h))

        if !setupDone {
            setupDone = true
            viewModel?.setupMetalLayer(metalLayer, width: w, height: h)
        } else {
            viewModel?.resizeMetal(width: Int32(w), height: Int32(h))
        }
    }

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()
        if window != nil {
            ensureMetalSetup()
        }
    }

    override func setFrameSize(_ newSize: NSSize) {
        super.setFrameSize(newSize)
        if newSize.width > 0 && newSize.height > 0 {
            if setupDone {
                let w = Int(metalLayer.bounds.width * metalLayer.contentsScale)
                let h = Int(metalLayer.bounds.height * metalLayer.contentsScale)
                viewModel?.resizeMetal(width: Int32(w), height: Int32(h))
            } else if window != nil {
                ensureMetalSetup()
            }
        }
    }
}
