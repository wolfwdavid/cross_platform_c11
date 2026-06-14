#pragma once

#include "ghostty/GhosttyRuntime.h"
#include "ghostty/GhosttyConfig.h"

#include <QObject>
#include <memory>

namespace c11 {

class C11Application : public QObject {
    Q_OBJECT

public:
    explicit C11Application(QObject *parent = nullptr);
    ~C11Application() override;

    bool initialize();

    GhosttyRuntime &ghosttyRuntime() { return *m_ghosttyRuntime; }
    const GhosttyConfig &ghosttyConfig() const { return m_config; }

    void reloadConfig();

signals:
    void configReloaded();

private:
    void mirrorEnvVars();

    std::unique_ptr<GhosttyRuntime> m_ghosttyRuntime;
    GhosttyConfig m_config;
};

} // namespace c11
