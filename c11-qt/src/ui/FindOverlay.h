#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>

namespace c11 {

// Floating find-in-page overlay, used for both terminal and browser search.
class FindOverlay : public QWidget {
    Q_OBJECT

public:
    explicit FindOverlay(QWidget *parent = nullptr);

    QString searchText() const;
    void setSearchText(const QString &text);
    void setMatchInfo(int current, int total);
    void focusSearchField();

signals:
    void searchRequested(const QString &text);
    void nextRequested();
    void previousRequested();
    void closeRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLineEdit *m_searchField;
    QLabel *m_matchLabel;
};

} // namespace c11
