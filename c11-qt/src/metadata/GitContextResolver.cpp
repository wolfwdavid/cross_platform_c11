#include "GitContextResolver.h"

#include <QProcess>
#include <QDir>
#include <QApplication>
#include <QThread>
#include <QTimer>

namespace c11 {

GitContextResolver &GitContextResolver::instance()
{
    static GitContextResolver resolver;
    return resolver;
}

GitContextResolver::GitContextResolver() = default;

GitContext GitContextResolver::resolve(const QString &cwd)
{
    if (cwd.isEmpty()) return {};

    {
        QMutexLocker lock(&m_cacheMutex);
        auto it = m_cache.find(cwd);
        if (it != m_cache.end()) return *it;
    }

    GitContext ctx = resolveImpl(cwd);

    {
        QMutexLocker lock(&m_cacheMutex);
        m_cache[cwd] = ctx;
    }

    return ctx;
}

void GitContextResolver::resolveAsync(const QString &cwd,
                                       std::function<void(const GitContext &)> callback)
{
    // Run on a background thread, call back on main
    auto *thread = QThread::create([this, cwd, callback]() {
        GitContext ctx = resolve(cwd);
        QTimer::singleShot(0, qApp, [callback, ctx]() {
            callback(ctx);
        });
    });
    thread->start();
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void GitContextResolver::invalidateCache()
{
    QMutexLocker lock(&m_cacheMutex);
    m_cache.clear();
}

GitContext GitContextResolver::resolveImpl(const QString &cwd)
{
    GitContext ctx;

    // Check if inside a git repo
    QString topLevel = runGit({"rev-parse", "--show-toplevel"}, cwd);
    if (topLevel.isEmpty()) return ctx;

    ctx.repoRoot = topLevel;

    // Get branch
    QString branch = runGit({"rev-parse", "--abbrev-ref", "HEAD"}, cwd);
    if (branch == "HEAD") {
        ctx.isDetachedHead = true;
        // Get short SHA instead
        ctx.branch = runGit({"rev-parse", "--short", "HEAD"}, cwd);
    } else {
        ctx.branch = branch;
    }

    // Check for worktree
    QString commonDir = runGit({"rev-parse", "--git-common-dir"}, cwd);
    QString gitDir = runGit({"rev-parse", "--git-dir"}, cwd);
    if (!commonDir.isEmpty() && !gitDir.isEmpty() && commonDir != gitDir) {
        // This is a worktree
        QDir dir(cwd);
        ctx.worktree = dir.dirName();
    }

    return ctx;
}

QString GitContextResolver::runGit(const QStringList &args, const QString &cwd)
{
    QProcess git;
    git.setWorkingDirectory(cwd);
    git.start("git", args);
    if (!git.waitForFinished(3000) || git.exitCode() != 0) {
        return {};
    }
    return QString::fromUtf8(git.readAllStandardOutput()).trimmed();
}

} // namespace c11
