#include "BrowserPanel.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QLineEdit>
#include <QAction>
#include <QWebEngineView>
#include <QWebEnginePage>

namespace c11 {

BrowserPanel::BrowserPanel(const QUuid &workspaceId,
                           const QUrl &initialUrl,
                           QObject *parent)
    : Panel(PanelType::Browser, parent)
    , m_workspaceId(workspaceId)
{
    setupUI();
    navigate(initialUrl);
}

BrowserPanel::~BrowserPanel()
{
    delete m_container;
}

void BrowserPanel::setupUI()
{
    m_container = new QWidget();
    auto *layout = new QVBoxLayout(m_container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar with back/forward/reload + address bar
    m_toolbar = new QToolBar(m_container);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));

    auto *backAction = m_toolbar->addAction("<", this, &BrowserPanel::goBack);
    backAction->setToolTip("Back");

    auto *forwardAction = m_toolbar->addAction(">", this, &BrowserPanel::goForward);
    forwardAction->setToolTip("Forward");

    auto *reloadAction = m_toolbar->addAction("R", this, &BrowserPanel::reload);
    reloadAction->setToolTip("Reload");

    m_addressBar = new QLineEdit(m_toolbar);
    m_addressBar->setPlaceholderText("Enter URL or search...");
    m_toolbar->addWidget(m_addressBar);

    connect(m_addressBar, &QLineEdit::returnPressed, this, [this]() {
        navigateOrSearch(m_addressBar->text());
    });

    layout->addWidget(m_toolbar);

    // Web view
    m_webView = new QWebEngineView(m_container);
    layout->addWidget(m_webView, 1);

    connect(m_webView, &QWebEngineView::urlChanged, this, &BrowserPanel::onUrlChanged);
    connect(m_webView, &QWebEngineView::titleChanged, this, &BrowserPanel::onTitleChanged);
    connect(m_webView, &QWebEngineView::loadFinished, this, &BrowserPanel::loadFinished);
}

void BrowserPanel::focus()
{
    m_webView->setFocus();
}

void BrowserPanel::unfocus()
{
    // No-op
}

void BrowserPanel::close()
{
    m_webView->stop();
    emit closed();
}

void BrowserPanel::navigate(const QUrl &url)
{
    m_webView->setUrl(url);
    m_addressBar->setText(url.toString());
}

void BrowserPanel::navigateOrSearch(const QString &input)
{
    QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) return;

    QUrl url = QUrl::fromUserInput(trimmed);
    if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https"
                          || url.scheme() == "file" || url.scheme() == "about")) {
        navigate(url);
    } else if (trimmed.contains('.') && !trimmed.contains(' ')) {
        navigate(QUrl("https://" + trimmed));
    } else {
        // Search using DuckDuckGo
        navigate(QUrl("https://duckduckgo.com/?q=" + QUrl::toPercentEncoding(trimmed)));
    }
}

void BrowserPanel::goBack()
{
    m_webView->back();
}

void BrowserPanel::goForward()
{
    m_webView->forward();
}

void BrowserPanel::reload()
{
    m_webView->reload();
}

void BrowserPanel::evaluateJavaScript(const QString &script)
{
    m_webView->page()->runJavaScript(script);
}

void BrowserPanel::findText(const QString &text, bool forward)
{
    if (text.isEmpty()) {
        clearFind();
        return;
    }
    QWebEnginePage::FindFlags flags;
    if (!forward) flags |= QWebEnginePage::FindBackward;
    m_webView->findText(text, flags);
}

void BrowserPanel::clearFind()
{
    m_webView->findText(QString());
}

QUrl BrowserPanel::currentUrl() const
{
    return m_webView->url();
}

void BrowserPanel::onUrlChanged(const QUrl &url)
{
    m_addressBar->setText(url.toString());
    emit urlChanged(url);
}

void BrowserPanel::onTitleChanged(const QString &title)
{
    m_title = title.isEmpty() ? "Browser" : title;
    emit titleChanged(m_title);
}

} // namespace c11
