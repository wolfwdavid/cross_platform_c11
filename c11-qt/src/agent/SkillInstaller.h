#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace c11 {

// Installs c11 skills into ~/.claude/skills/<name>/ for agent discovery.
// Skills are one-time copies from the app's skills/ directory.
class SkillInstaller : public QObject {
    Q_OBJECT

public:
    static SkillInstaller &instance();

    // Install a skill by name (e.g., "c11", "c11-browser")
    bool installSkill(const QString &name);

    // Install all installable skills from the manifest
    int installAllSkills();

    // Sync an already-installed skill from source
    bool syncSkill(const QString &name);

    // List installed skills
    QStringList installedSkills() const;

    // Check if a skill is installed
    bool isInstalled(const QString &name) const;

    // Skill directories
    static QString skillSourceDir();
    static QString skillInstallDir();
    static QString skillInstallDir(const QString &name);

signals:
    void skillInstalled(const QString &name);
    void skillSynced(const QString &name);

private:
    SkillInstaller() = default;

    bool copyDirectory(const QString &src, const QString &dst) const;
};

} // namespace c11
