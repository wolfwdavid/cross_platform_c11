#pragma once

#include "Panel.h"

#include <QWidget>
#include <QUuid>
#include <QUrl>

class QWebEngineView;
class QLineEdit;
class QToolBar;

namespace c11 {

// Browser panel embedding a QWebEngineView with address bar and navigation.
class BrowserPanel : public Panel {
    Q_OBJECT

public:
    explicit BrowserPanel(const QUuid &workspaceId,
                          const QUrl &initialUrl = QUrl("about:blank"),
                          QObject *parent = nullptr);
    ~BrowserPanel() override;

    QString displayTitle() const override { return m_title; }
    QWidget *contentWidget() override { return m_container; }

    void focus() override;
    void unfocus() override;
    void close() override;

    QUuid workspaceId() const { return m_workspaceId; }

    // Navigation
    void navigate(const QUrl &url);
    void navigateOrSearch(const QString &input);
    void goBack();
    void goForward();
    void reload();

    // JavaScript evaluation
    void evaluateJavaScript(const QString &script);

    // Find in page
    void findText(const QString &text, bool forward = true);
    void clearFind();

    QUrl currentUrl() const;

signals:
    void urlChanged(const QUrl &url);
    void loadFinished(bool ok);

private:
    void setupUI();
    void onUrlChanged(const QUrl &url);
    void onTitleChanged(const QString &title);

    QUuid m_workspaceId;
    QWidget *m_container = nullptr;
    QWebEngineView *m_webView = nullptr;
    QLineEdit *m_addressBar = nullptr;
    QToolBar *m_toolbar = nullptr;
    QString m_title = "Browser";
};

} // namespace c11
