#pragma once

#include "PaneLayout.h"
#include "panel/Panel.h"

#include <QWidget>
#include <QSplitter>
#include <QStackedWidget>
#include <QMap>
#include <QUuid>
#include <functional>

namespace c11 {

// Renders a PaneLayout tree as nested QSplitters.
// Each leaf maps to a panel's content widget.
class PaneLayoutWidget : public QWidget {
    Q_OBJECT

public:
    // PanelResolver: given a panel UUID, return its content QWidget.
    using PanelResolver = std::function<QWidget *(const QUuid &)>;

    explicit PaneLayoutWidget(PanelResolver resolver, QWidget *parent = nullptr);
    ~PaneLayoutWidget() override;

    void setLayout(const PaneLayout &layout);
    void clear();

signals:
    void panelFocused(const QUuid &panelId);

private:
    QWidget *buildWidget(const PaneLayout &node);

    PanelResolver m_resolver;
    QWidget *m_rootWidget = nullptr;
};

} // namespace c11
