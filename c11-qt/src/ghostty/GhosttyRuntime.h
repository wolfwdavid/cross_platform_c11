#pragma once

#include "ghostty.h"
#include "GhosttyConfig.h"

#include <QObject>
#include <QTimer>
#include <mutex>

namespace c11 {

// Singleton wrapping ghostty_app_t. Owns the Ghostty runtime lifecycle,
// wakeup timer, and dispatches actions from Ghostty back to Qt.
class GhosttyRuntime : public QObject {
    Q_OBJECT

public:
    explicit GhosttyRuntime(QObject *parent = nullptr);
    ~GhosttyRuntime() override;

    GhosttyRuntime(const GhosttyRuntime &) = delete;
    GhosttyRuntime &operator=(const GhosttyRuntime &) = delete;

    bool initialize(const GhosttyConfig &config);
    void shutdown();

    ghostty_app_t app() const { return m_app; }
    bool isInitialized() const { return m_app != nullptr; }

    void updateConfig(const GhosttyConfig &config);
    void setFocus(bool focused);
    void tick();

signals:
    void wakeupRequested();
    void actionReceived(ghostty_target_s target, ghostty_action_s action);
    void surfaceCloseRequested(ghostty_surface_t surface, bool processAlive);

private:
    // Ghostty C callbacks — static trampolines to instance methods
    static void onWakeup(void *userdata);
    static bool onAction(ghostty_app_t app, ghostty_target_s target, ghostty_action_s action);
    static void onReadClipboard(void *userdata, ghostty_clipboard_e location, void *state);
    static void onConfirmReadClipboard(void *userdata, const char *contents,
                                        void *state, ghostty_clipboard_request_e request);
    static void onWriteClipboard(void *userdata, ghostty_clipboard_e location,
                                  const ghostty_clipboard_content_s *content,
                                  size_t count, bool confirm);
    static void onCloseSurface(void *userdata, bool processAlive);

    void handleWakeup();
    bool handleAction(ghostty_target_s target, ghostty_action_s action);
    void handleReadClipboard(ghostty_clipboard_e location, void *state);
    void handleWriteClipboard(ghostty_clipboard_e location,
                               const ghostty_clipboard_content_s *content,
                               size_t count);

    ghostty_app_t m_app = nullptr;
    ghostty_config_t m_ghosttyConfig = nullptr;
    QTimer m_tickTimer;
};

} // namespace c11
