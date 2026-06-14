#include "FindOverlay.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>

namespace c11 {

FindOverlay::FindOverlay(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(36);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    m_searchField = new QLineEdit(this);
    m_searchField->setPlaceholderText("Find...");
    m_searchField->setMinimumWidth(200);
    layout->addWidget(m_searchField);

    m_matchLabel = new QLabel("0/0", this);
    m_matchLabel->setFixedWidth(50);
    m_matchLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_matchLabel);

    auto *prevBtn = new QPushButton("^", this);
    prevBtn->setFixedSize(28, 28);
    prevBtn->setToolTip("Previous match");
    layout->addWidget(prevBtn);

    auto *nextBtn = new QPushButton("v", this);
    nextBtn->setFixedSize(28, 28);
    nextBtn->setToolTip("Next match");
    layout->addWidget(nextBtn);

    auto *closeBtn = new QPushButton("x", this);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setToolTip("Close");
    layout->addWidget(closeBtn);

    connect(m_searchField, &QLineEdit::textChanged, this, &FindOverlay::searchRequested);
    connect(m_searchField, &QLineEdit::returnPressed, this, &FindOverlay::nextRequested);
    connect(nextBtn, &QPushButton::clicked, this, &FindOverlay::nextRequested);
    connect(prevBtn, &QPushButton::clicked, this, &FindOverlay::previousRequested);
    connect(closeBtn, &QPushButton::clicked, this, &FindOverlay::closeRequested);

    hide();
}

QString FindOverlay::searchText() const
{
    return m_searchField->text();
}

void FindOverlay::setSearchText(const QString &text)
{
    m_searchField->setText(text);
}

void FindOverlay::setMatchInfo(int current, int total)
{
    if (total <= 0) {
        m_matchLabel->setText("0/0");
    } else {
        m_matchLabel->setText(QString("%1/%2").arg(current).arg(total));
    }
}

void FindOverlay::focusSearchField()
{
    m_searchField->setFocus();
    m_searchField->selectAll();
}

void FindOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit closeRequested();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace c11
