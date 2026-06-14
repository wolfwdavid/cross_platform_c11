#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>
#include <functional>

namespace c11 {

// Resolves git branch and worktree info from a working directory.
struct GitContext {
    QString branch;
    QString worktree;
    QString repoRoot;
    bool isDetachedHead = false;
};

class GitContextResolver : public QObject {
    Q_OBJECT

public:
    static GitContextResolver &instance();

    // Synchronous resolve (may run git commands)
    GitContext resolve(const QString &cwd);

    // Async resolve — calls callback on the main thread
    void resolveAsync(const QString &cwd, std::function<void(const GitContext &)> callback);

    // Clear cache
    void invalidateCache();

private:
    GitContextResolver();

    static GitContext resolveImpl(const QString &cwd);
    static QString runGit(const QStringList &args, const QString &cwd);

    mutable QMutex m_cacheMutex;
    QMap<QString, GitContext> m_cache;
};

} // namespace c11
