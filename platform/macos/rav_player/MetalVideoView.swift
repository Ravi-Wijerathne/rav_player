import SwiftUI
import Metal

struct MetalVideoView: NSViewRepresentable {
    @EnvironmentObject var viewModel: PlayerViewModel

    func makeNSView(context: Context) -> MetalVideoNSView {
        let view = MetalVideoNSView()
        view.viewModel = viewModel
        viewModel.setupMetalLayer(view.metalLayer, width: Int(view.frame.width), height: Int(view.frame.height))
        return view
    }

    func updateNSView(_ nsView: MetalVideoNSView, context: Context) {
        nsView.viewModel = viewModel
    }
}

class MetalVideoNSView: NSView {
    let metalLayer = CAMetalLayer()
    weak var viewModel: PlayerViewModel?

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
        metalLayer.needsDisplayOnBoundsChange = true
        layer = metalLayer
    }

    override func viewDidEndLiveResize() {
        super.viewDidEndLiveResize()
        viewModel?.setupMetalLayer(metalLayer,
                                    width: Int(metalLayer.bounds.width * metalLayer.contentsScale),
                                    height: Int(metalLayer.bounds.height * metalLayer.contentsScale))
    }
}
