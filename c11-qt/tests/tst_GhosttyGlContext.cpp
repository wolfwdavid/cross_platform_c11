#include <QtTest/QtTest>
#include <QApplication>

#include "ghostty/GhosttyGlContext.h"
#include "ghostty/GhosttyQtPlatform.h"

using namespace c11;

// Verifies the host (Qt) side of the GHOSTTY_PLATFORM_QT integration seam:
// a desktop-GL context can be created and yields a native handle, and that
// handle is correctly marshalled into ghostty_surface_config_s. Does NOT
// require libghostty (the config struct is filled directly).
class TestGhosttyGlContext : public QObject
{
    Q_OBJECT
private slots:
    void offscreenContextProvidesNativeHandle();
    void configureSurfaceFillsQtPlatform();
};

void TestGhosttyGlContext::offscreenContextProvidesNativeHandle()
{
    GhosttyGlContext ctx;
    if (!ctx.createOffscreen())
        QSKIP("No usable OpenGL context here (headless / no GL driver)");

    QVERIFY(ctx.isValid());
    const QString version = ctx.glVersionString();
    QVERIFY2(!version.isEmpty(), "GL_VERSION should be queryable when current");
    qInfo() << "GL_VERSION:" << version;
    QVERIFY2(ctx.nativeContext() != nullptr,
             "expected a native GL context handle (HGLRC/GLX/EGL)");
}

void TestGhosttyGlContext::configureSurfaceFillsQtPlatform()
{
    GhosttyGlContext ctx;
    if (!ctx.createOffscreen())
        QSKIP("No usable OpenGL context here (headless / no GL driver)");

    QVERIFY(GhosttyQtPlatform::isSupported());

    ghostty_surface_config_s cfg{};
    void *fakeWindow = reinterpret_cast<void *>(static_cast<quintptr>(0xABCDu));
    const bool ok = GhosttyQtPlatform::configureSurface(
        cfg, ctx, fakeWindow, 800, 600, 1.0);

    QVERIFY2(ok, "configureSurface should succeed with a valid GL context");
    QCOMPARE(cfg.platform_tag, GHOSTTY_PLATFORM_QT);
    QCOMPARE(cfg.platform.qt.native_window, fakeWindow);
    QCOMPARE(cfg.platform.qt.gl_context, ctx.nativeContext());
    QCOMPARE(cfg.platform.qt.width, 800u);
    QCOMPARE(cfg.platform.qt.height, 600u);
}

QTEST_MAIN(TestGhosttyGlContext)
#include "tst_GhosttyGlContext.moc"
