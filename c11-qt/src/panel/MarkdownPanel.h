#pragma once

#include "Panel.h"

#include <QWidget>
#include <QUuid>
#include <QFileSystemWatcher>

class QWebEngineView;

namespace c11 {

// Markdown panel: renders .md files via QWebEngineView + marked.js.
// Watches the file for changes and auto-reloads.
class MarkdownPanel : public Panel {
    Q_OBJECT

public:
    explicit MarkdownPanel(const QUuid &workspaceId,
                           const QString &filePath = {},
                           QObject *parent = nullptr);
    ~MarkdownPanel() override;

    QString displayTitle() const override { return m_displayTitle; }
    QWidget *contentWidget() override { return m_container; }

    void focus() override;
    void unfocus() override;
    void close() override;

    QUuid workspaceId() const { return m_workspaceId; }

    // Content
    void setFilePath(const QString &path);
    void setContent(const QString &markdown);
    QString filePath() const { return m_filePath; }
    QString content() const { return m_content; }

    // Find
    void findText(const QString &text, bool forward = true);
    void clearFind();

signals:
    void contentChanged();

private:
    void render();
    void loadFile();
    void onFileChanged(const QString &path);
    static QString markdownToHtml(const QString &markdown);
    static QString htmlTemplate(const QString &bodyHtml);

    QUuid m_workspaceId;
    QWidget *m_container = nullptr;
    QWebEngineView *m_webView = nullptr;
    QFileSystemWatcher *m_watcher = nullptr;
    QString m_filePath;
    QString m_content;
    QString m_displayTitle = "Markdown";
};

} // namespace c11
