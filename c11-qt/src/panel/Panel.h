#pragma once

#include <QObject>
#include <QUuid>
#include <QString>
#include <QWidget>

namespace c11 {

enum class PanelType {
    Terminal,
    Browser,
    Markdown
};

// Abstract base for all panel types (terminal, browser, markdown).
// Each panel has a unique ID, display title, and a content widget.
class Panel : public QObject {
    Q_OBJECT

public:
    explicit Panel(PanelType type, QObject *parent = nullptr);
    ~Panel() override;

    QUuid id() const { return m_id; }
    PanelType panelType() const { return m_type; }

    virtual QString displayTitle() const = 0;
    virtual QWidget *contentWidget() = 0;

    virtual void focus() {}
    virtual void unfocus() {}
    virtual void close() {}

    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }

signals:
    void titleChanged(const QString &title);
    void closed();

protected:
    QUuid m_id;
    PanelType m_type;
    bool m_dirty = false;
};

} // namespace c11
