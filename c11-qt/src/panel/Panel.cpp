#include "Panel.h"

namespace c11 {

Panel::Panel(PanelType type, QObject *parent)
    : QObject(parent), m_id(QUuid::createUuid()), m_type(type) {}

Panel::~Panel() = default;

} // namespace c11
