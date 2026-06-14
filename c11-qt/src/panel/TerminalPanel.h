#pragma once

#include "Panel.h"
#include "ghostty/GhosttyWidget.h"
#include "ghostty/GhosttyRuntime.h"

#include <QUuid>

namespace c11 {

class TerminalPanel : public Panel {
    Q_OBJECT

public:
    explicit TerminalPanel(GhosttyRuntime &runtime,
                           const QUuid &workspaceId,
                           const QString &workingDirectory = {},
                           const QString &command = {},
                           QObject *parent = nullptr);
    ~TerminalPanel() override;

    QString displayTitle() const override { return m_title; }
    QWidget *contentWidget() override { return m_widget; }

    void focus() override;
    void unfocus() override;
    void close() override;

    GhosttyWidget *ghosttyWidget() const { return m_widget; }
    QUuid workspaceId() const { return m_workspaceId; }

    void setTitle(const QString &title);
    void setWorkspaceId(const QUuid &id) { m_workspaceId = id; }

    bool processExited() const;

private:
    GhosttyWidget *m_widget;
    QUuid m_workspaceId;
    QString m_title = "Terminal";
};

} // namespace c11
