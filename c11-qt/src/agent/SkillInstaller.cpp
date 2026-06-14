#include "SkillInstaller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

namespace c11 {

SkillInstaller &SkillInstaller::instance()
{
    static SkillInstaller installer;
    return installer;
}

bool SkillInstaller::installSkill(const QString &name)
{
    QString src = skillSourceDir() + "/" + name;
    QString dst = skillInstallDir(name);

    if (!QDir(src).exists()) {
        qWarning() << "SkillInstaller: source not found:" << src;
        return false;
    }

    if (!copyDirectory(src, dst)) {
        qWarning() << "SkillInstaller: failed to copy" << src << "to" << dst;
        return false;
    }

    // Write marker file
    QFile marker(dst + "/.c11-skill.json");
    if (marker.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["installed_by"] = "c11";
        obj["name"] = name;
        obj["version"] = QCoreApplication::applicationVersion();
        marker.write(QJsonDocument(obj).toJson());
    }

    emit skillInstalled(name);
    return true;
}

int SkillInstaller::installAllSkills()
{
    int count = 0;

    // Read manifest
    QString manifestPath = skillSourceDir() + "/MANIFEST.json";
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        qWarning() << "SkillInstaller: no MANIFEST.json at" << manifestPath;
        return 0;
    }

    QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    QJsonArray skills = doc.object().value("installable").toArray();

    for (const auto &val : skills) {
        QString name = val.toString();
        if (!name.isEmpty() && installSkill(name)) {
            count++;
        }
    }

    return count;
}

bool SkillInstaller::syncSkill(const QString &name)
{
    if (!isInstalled(name)) return false;

    QString src = skillSourceDir() + "/" + name;
    QString dst = skillInstallDir(name);

    if (!copyDirectory(src, dst)) return false;

    emit skillSynced(name);
    return true;
}

QStringList SkillInstaller::installedSkills() const
{
    QStringList result;
    QDir installDir(skillInstallDir());
    if (!installDir.exists()) return result;

    for (const auto &entry : installDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (QFile::exists(entry.absoluteFilePath() + "/.c11-skill.json")) {
            result.append(entry.fileName());
        }
    }
    return result;
}

bool SkillInstaller::isInstalled(const QString &name) const
{
    return QFile::exists(skillInstallDir(name) + "/.c11-skill.json");
}

QString SkillInstaller::skillSourceDir()
{
    // Look in app bundle or alongside the binary
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/../Resources/skills",
        appDir + "/skills",
        appDir + "/../../skills",
    };
    for (const auto &path : candidates) {
        if (QDir(path).exists()) return QDir(path).absolutePath();
    }
    return appDir + "/skills";
}

QString SkillInstaller::skillInstallDir()
{
    return QDir::homePath() + "/.claude/skills";
}

QString SkillInstaller::skillInstallDir(const QString &name)
{
    return skillInstallDir() + "/" + name;
}

bool SkillInstaller::copyDirectory(const QString &src, const QString &dst) const
{
    QDir srcDir(src);
    if (!srcDir.exists()) return false;

    QDir().mkpath(dst);

    for (const auto &entry : srcDir.entryInfoList(QDir::Files)) {
        QString srcFile = entry.absoluteFilePath();
        QString dstFile = dst + "/" + entry.fileName();
        QFile::remove(dstFile); // overwrite
        if (!QFile::copy(srcFile, dstFile)) return false;
    }

    for (const auto &entry : srcDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!copyDirectory(entry.absoluteFilePath(), dst + "/" + entry.fileName())) {
            return false;
        }
    }

    return true;
}

} // namespace c11
