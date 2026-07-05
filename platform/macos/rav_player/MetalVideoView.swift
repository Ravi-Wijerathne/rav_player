import SwiftUI
import Metal
import QuartzCore

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
    private var displayLink: Any?

    override init(frame: NSRect) {
        super.init(frame: frame)
        setupLayer()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupLayer()
    }

    deinit {
        stopDisplayLink()
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
            startDisplayLink()
        } else {
            viewModel?.resizeMetal(width: Int32(w), height: Int32(h))
        }
    }

    private func startDisplayLink() {
        if #available(macOS 15.0, *) {
            displayLink = self.displayLink(target: self, selector: #selector(renderDisplayLink(_:)))
        } else {
            // Fall back to CVDisplayLink for older macOS
            startCVDisplayLink()
        }
    }

    private func stopDisplayLink() {
        if #available(macOS 15.0, *) {
            if let link = displayLink as? NSObject {
                link.perform(NSSelectorFromString("invalidate"))
            }
            displayLink = nil
        }
    }

    @objc private func renderDisplayLink(_ sender: Any) {
        viewModel?.renderFrame()
    }

    // Legacy CVDisplayLink support for macOS < 15
    private var cvDisplayLink: CVDisplayLink?

    private func startCVDisplayLink() {
        stopCVDisplayLink()
        CVDisplayLinkCreateWithActiveCGDisplays(&cvDisplayLink)
        guard let link = cvDisplayLink else { return }

        let viewPtr = Unmanaged.passUnretained(self).toOpaque()
        CVDisplayLinkSetOutputHandler(link) { _, _, _, _, _ -> CVReturn in
            let view = Unmanaged<MetalVideoNSView>.fromOpaque(viewPtr).takeUnretainedValue()
            view.viewModel?.renderFrame()
            return kCVReturnSuccess
        }
        CVDisplayLinkStart(link)
    }

    private func stopCVDisplayLink() {
        if let link = cvDisplayLink {
            CVDisplayLinkStop(link)
            cvDisplayLink = nil
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
                metalLayer.drawableSize = CGSize(width: CGFloat(w), height: CGFloat(h))
                viewModel?.resizeMetal(width: Int32(w), height: Int32(h))
            } else if window != nil {
                ensureMetalSetup()
            }
        }
    }
}
