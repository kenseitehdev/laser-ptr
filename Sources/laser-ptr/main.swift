import AppKit

final class LaserPTRAppDelegate: NSObject, NSApplicationDelegate {
    private var statusItem: NSStatusItem!
    private var activity: NSObjectProtocol?
    private var timer: Timer?
    private var endDate: Date?
    private var isActive = false

    func applicationDidFinishLaunching(_ notification: Notification) {
        statusItem = NSStatusBar.system.statusItem(withLength: 28)
        statusItem.button?.image = makeIcon()
        statusItem.button?.image?.isTemplate = false
        statusItem.button?.toolTip = "laser-ptr · Keep the Mac awake"
        statusItem.menu = makeMenu()
    }

    func applicationWillTerminate(_ notification: Notification) {
        deactivate()
    }

    @objc private func toggleAwake() {
        isActive ? deactivate() : activate(for: nil)
    }

    @objc private func selectDuration(_ sender: NSMenuItem) {
        let minutes = (sender.representedObject as? NSNumber)?.intValue ?? -1
        activate(for: minutes > 0 ? TimeInterval(minutes * 60) : nil)
    }

    @objc private func quit() {
        deactivate()
        NSApplication.shared.terminate(nil)
    }

    private func activate(for duration: TimeInterval?) {
        deactivate()
        activity = ProcessInfo.processInfo.beginActivity(
            options: [.userInitiated, .idleSystemSleepDisabled, .idleDisplaySleepDisabled],
            reason: "laser-ptr is keeping the Mac awake"
        )
        isActive = true

        if let duration {
            endDate = Date().addingTimeInterval(duration)
            timer = Timer.scheduledTimer(withTimeInterval: duration, repeats: false) { [weak self] _ in
                self?.deactivate()
            }
        }
        refreshStatusItem()
    }

    private func deactivate() {
        timer?.invalidate()
        timer = nil
        if let activity {
            ProcessInfo.processInfo.endActivity(activity)
            self.activity = nil
        }
        isActive = false
        endDate = nil
        refreshStatusItem()
    }

    private func refreshStatusItem() {
        guard statusItem != nil else { return }
        statusItem.button?.image = makeIcon()
        statusItem.button?.image?.isTemplate = false
        statusItem.menu = makeMenu()
    }

    private func makeMenu() -> NSMenu {
        let menu = NSMenu()
        let toggleTitle = isActive ? "Turn Off" : "Keep Mac Awake"
        let toggle = NSMenuItem(title: toggleTitle, action: #selector(toggleAwake), keyEquivalent: "")
        toggle.target = self
        menu.addItem(toggle)

        let durationItem = NSMenuItem(title: "Keep Awake For", action: nil, keyEquivalent: "")
        let durationMenu = NSMenu()
        addDuration("Indefinitely", minutes: -1, to: durationMenu)
        durationMenu.addItem(.separator())
        addDuration("30 Minutes", minutes: 30, to: durationMenu)
        addDuration("1 Hour", minutes: 60, to: durationMenu)
        addDuration("2 Hours", minutes: 120, to: durationMenu)
        menu.setSubmenu(durationMenu, for: durationItem)
        menu.addItem(durationItem)

        menu.addItem(.separator())
        let status = NSMenuItem(title: statusText, action: nil, keyEquivalent: "")
        status.isEnabled = false
        menu.addItem(status)
        menu.addItem(.separator())

        let quitItem = NSMenuItem(title: "Quit laser-ptr", action: #selector(quit), keyEquivalent: "q")
        quitItem.target = self
        menu.addItem(quitItem)
        return menu
    }

    private func addDuration(_ title: String, minutes: Int, to menu: NSMenu) {
        let item = NSMenuItem(title: title, action: #selector(selectDuration(_:)), keyEquivalent: "")
        item.target = self
        item.representedObject = NSNumber(value: minutes)
        menu.addItem(item)
    }

    private var statusText: String {
        guard isActive else { return "Off" }
        guard let endDate else { return "On · indefinitely" }
        let minutes = max(1, Int(ceil(endDate.timeIntervalSinceNow / 60)))
        return "On · about \(minutes) min left"
    }

    private func makeIcon() -> NSImage {
        let size = NSSize(width: 28, height: 22)
        let dark = NSApp.effectiveAppearance.bestMatch(from: [.darkAqua, .aqua]) == .darkAqua
        let pointerColor: NSColor = dark ? .white : .black
        let image = NSImage(size: size, flipped: false) { _ in
            if self.isActive {
                let beam = NSBezierPath()
                beam.move(to: NSPoint(x: 11, y: 8))
                beam.line(to: NSPoint(x: 27, y: 2))
                beam.line(to: NSPoint(x: 27, y: 20))
                beam.line(to: NSPoint(x: 11, y: 14))
                beam.close()
                NSColor.systemRed.withAlphaComponent(0.35).setFill()
                beam.fill()

                let core = NSBezierPath()
                core.lineWidth = 1.8
                core.move(to: NSPoint(x: 11, y: 11))
                core.line(to: NSPoint(x: 27, y: 11))
                NSColor.systemRed.setStroke()
                core.stroke()
            }

            let pointer = NSBezierPath()
            pointer.move(to: NSPoint(x: 1, y: 6))
            pointer.line(to: NSPoint(x: 8, y: 6))
            pointer.line(to: NSPoint(x: 11, y: 9))
            pointer.line(to: NSPoint(x: 11, y: 13))
            pointer.line(to: NSPoint(x: 8, y: 16))
            pointer.line(to: NSPoint(x: 1, y: 16))
            pointer.curve(to: NSPoint(x: 1, y: 6), controlPoint1: NSPoint(x: -1, y: 14), controlPoint2: NSPoint(x: -1, y: 8))
            pointer.close()
            pointerColor.setFill()
            pointer.fill()

            let button = NSBezierPath(ovalIn: NSRect(x: 2, y: 9, width: 3.5, height: 4))
            (self.isActive ? NSColor.systemRed : NSColor.systemGray).setFill()
            button.fill()
            return true
        }
        image.isTemplate = false
        return image
    }
}

@main
enum LaserPTRMain {
    static func main() {
        let application = NSApplication.shared
        let delegate = LaserPTRAppDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.accessory)
        application.run()
    }
}
