#pragma once

#include "ghostty.h"
#include "GhosttyRuntime.h"
#include "GhosttyKeyMapper.h"

#include <QWidget>
#include <QString>

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
#include "GhosttyGlContext.h"
#include <memory>
#endif

namespace c11 {

// QWidget that hosts a single Ghostty terminal surface.
// On macOS, extracts the native NSView from QWidget::winId() and passes it
// to Ghostty via GHOSTTY_PLATFORM_MACOS. Metal rendering works unchanged.
class GhosttyWidget : public QWidget {
    Q_OBJECT

public:
    explicit GhosttyWidget(GhosttyRuntime &runtime, QWidget *parent = nullptr);
    ~GhosttyWidget() override;

    bool createSurface(const QString &workingDirectory = {},
                       const QString &command = {},
                       ghostty_surface_context_e context = GHOSTTY_SURFACE_CONTEXT_SPLIT);

    void destroySurface();

    ghostty_surface_t surface() const { return m_surface; }
    bool hasSurface() const { return m_surface != nullptr; }

    void setFocused(bool focused);
    void sendText(const QString &text);
    // Synthesize a key press+release. `keycode` is the platform-native scancode
    // ghostty matches to recover the logical key; this drives ghostty's key
    // encoder (the right path for Enter, Ctrl-C, arrows, and other escape
    // sequences — unlike sendText, which is a paste).
    void sendKey(uint32_t keycode, ghostty_input_mods_e mods,
                 uint32_t unshiftedCodepoint = 0);
    // Submit the current line (Enter). Convenience over sendKey; sendText's paste
    // path does NOT execute a trailing newline, so use this to run a command.
    void sendEnter();

    // Surface info
    ghostty_surface_size_s surfaceSize() const;
    bool processExited() const;

    // Read the surface's rendered text. Default: the visible viewport; with
    // scrollback=true, the full screen buffer including history. Returns "" if
    // there is no surface or the read fails.
    QString readScreen(bool scrollback = false) const;

signals:
    void surfaceCreated();
    void surfaceDestroyed();
    void titleChanged(const QString &title);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

    bool event(QEvent *event) override;

private:
    void updateSurfaceSize();
    // Schedule a small burst of deferred redraws so a reparented surface
    // (e.g. after a split) reliably repaints once its window is composited.
    void scheduleSurfaceRepaint();
    ghostty_input_mods_e qtModsToGhostty(Qt::KeyboardModifiers mods) const;
    ghostty_input_mouse_button_e qtButtonToGhostty(Qt::MouseButton button) const;

    GhosttyRuntime &m_runtime;
    ghostty_surface_t m_surface = nullptr;
    void *m_childNSView = nullptr; // Child NSView for Ghostty Metal rendering (macOS)
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    std::unique_ptr<GhosttyGlContext> m_glContext; // Host GL context (Qt platform)
#endif
    GhosttyKeyMapper m_keyMapper;
    bool m_focused = false;
};

} // namespace c11
