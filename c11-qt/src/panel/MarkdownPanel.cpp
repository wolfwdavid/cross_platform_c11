#include "MarkdownPanel.h"

#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>

namespace c11 {

MarkdownPanel::MarkdownPanel(const QUuid &workspaceId,
                             const QString &filePath,
                             QObject *parent)
    : Panel(PanelType::Markdown, parent)
    , m_workspaceId(workspaceId)
{
    m_container = new QWidget();
    auto *layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(m_container);
    layout->addWidget(m_webView);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &MarkdownPanel::onFileChanged);

    if (!filePath.isEmpty()) {
        setFilePath(filePath);
    }
}

MarkdownPanel::~MarkdownPanel()
{
    delete m_container;
}

void MarkdownPanel::focus()
{
    m_webView->setFocus();
}

void MarkdownPanel::unfocus() {}

void MarkdownPanel::close()
{
    emit closed();
}

void MarkdownPanel::setFilePath(const QString &path)
{
    if (!m_filePath.isEmpty()) {
        m_watcher->removePath(m_filePath);
    }

    m_filePath = path;

    if (!path.isEmpty()) {
        m_watcher->addPath(path);
        QFileInfo fi(path);
        m_displayTitle = fi.fileName();
        emit titleChanged(m_displayTitle);
    }

    loadFile();
}

void MarkdownPanel::setContent(const QString &markdown)
{
    m_content = markdown;
    render();
    emit contentChanged();
}

void MarkdownPanel::loadFile()
{
    if (m_filePath.isEmpty()) return;

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "MarkdownPanel: cannot open" << m_filePath;
        return;
    }

    QTextStream stream(&file);
    m_content = stream.readAll();
    render();
    emit contentChanged();
}

void MarkdownPanel::onFileChanged(const QString &path)
{
    Q_UNUSED(path);
    loadFile();
    // Re-watch in case the file was replaced (atomic save)
    if (!m_filePath.isEmpty() && !m_watcher->files().contains(m_filePath)) {
        m_watcher->addPath(m_filePath);
    }
}

void MarkdownPanel::render()
{
    QString html = htmlTemplate(markdownToHtml(m_content));
    m_webView->setHtml(html, QUrl("about:markdown"));
}

void MarkdownPanel::findText(const QString &text, bool forward)
{
    if (text.isEmpty()) {
        clearFind();
        return;
    }
    QWebEnginePage::FindFlags flags;
    if (!forward) flags |= QWebEnginePage::FindBackward;
    m_webView->findText(text, flags);
}

void MarkdownPanel::clearFind()
{
    m_webView->findText(QString());
}

QString MarkdownPanel::markdownToHtml(const QString &markdown)
{
    // Simple markdown-to-HTML conversion for common elements.
    // For production, marked.js runs in the WebEngine; this is the fallback.
    QString html;
    QStringList lines = markdown.split('\n');
    bool inCodeBlock = false;
    bool inList = false;

    for (const auto &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("```")) {
            if (inCodeBlock) {
                html += "</code></pre>\n";
                inCodeBlock = false;
            } else {
                if (inList) { html += "</ul>\n"; inList = false; }
                html += "<pre><code>";
                inCodeBlock = true;
            }
            continue;
        }

        if (inCodeBlock) {
            html += line.toHtmlEscaped() + "\n";
            continue;
        }

        if (trimmed.isEmpty()) {
            if (inList) { html += "</ul>\n"; inList = false; }
            html += "<br>\n";
            continue;
        }

        // Headers
        if (trimmed.startsWith("### ")) {
            if (inList) { html += "</ul>\n"; inList = false; }
            html += "<h3>" + trimmed.mid(4).toHtmlEscaped() + "</h3>\n";
        } else if (trimmed.startsWith("## ")) {
            if (inList) { html += "</ul>\n"; inList = false; }
            html += "<h2>" + trimmed.mid(3).toHtmlEscaped() + "</h2>\n";
        } else if (trimmed.startsWith("# ")) {
            if (inList) { html += "</ul>\n"; inList = false; }
            html += "<h1>" + trimmed.mid(2).toHtmlEscaped() + "</h1>\n";
        }
        // List items
        else if (trimmed.startsWith("- ") || trimmed.startsWith("* ")) {
            if (!inList) { html += "<ul>\n"; inList = true; }
            html += "<li>" + trimmed.mid(2).toHtmlEscaped() + "</li>\n";
        }
        // Horizontal rule
        else if (trimmed == "---" || trimmed == "***") {
            if (inList) { html += "</ul>\n"; inList = false; }
            html += "<hr>\n";
        }
        // Paragraph
        else {
            if (inList) { html += "</ul>\n"; inList = false; }
            // Inline formatting
            QString formatted = trimmed.toHtmlEscaped();
            // Bold: **text**
            formatted.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
            // Italic: *text*
            formatted.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");
            // Code: `text`
            formatted.replace(QRegularExpression("`(.+?)`"), "<code>\\1</code>");
            html += "<p>" + formatted + "</p>\n";
        }
    }

    if (inCodeBlock) html += "</code></pre>\n";
    if (inList) html += "</ul>\n";

    return html;
}

QString MarkdownPanel::htmlTemplate(const QString &bodyHtml)
{
    return QStringLiteral(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
    font-size: 14px;
    line-height: 1.6;
    padding: 20px 32px;
    max-width: 800px;
    margin: 0 auto;
    color: #e0e0e0;
    background: #1e1e1e;
  }
  h1, h2, h3 { color: #ffffff; margin-top: 1.2em; }
  h1 { font-size: 1.8em; border-bottom: 1px solid #444; padding-bottom: 0.3em; }
  h2 { font-size: 1.4em; border-bottom: 1px solid #333; padding-bottom: 0.2em; }
  h3 { font-size: 1.1em; }
  code { background: #2d2d2d; padding: 2px 6px; border-radius: 3px; font-size: 0.9em; }
  pre { background: #2d2d2d; padding: 12px 16px; border-radius: 6px; overflow-x: auto; }
  pre code { padding: 0; background: none; }
  a { color: #58a6ff; text-decoration: none; }
  a:hover { text-decoration: underline; }
  hr { border: none; border-top: 1px solid #444; margin: 1.5em 0; }
  ul, ol { padding-left: 24px; }
  li { margin: 4px 0; }
  strong { color: #ffffff; }
  @media (prefers-color-scheme: light) {
    body { color: #24292e; background: #ffffff; }
    h1, h2, h3, strong { color: #000000; }
    h1 { border-bottom-color: #ddd; }
    h2 { border-bottom-color: #eee; }
    code { background: #f0f0f0; }
    pre { background: #f6f8fa; }
    a { color: #0366d6; }
    hr { border-top-color: #ddd; }
  }
</style>
</head>
<body>)") + bodyHtml + QStringLiteral("</body></html>");
}

} // namespace c11
