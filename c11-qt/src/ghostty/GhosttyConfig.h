#pragma once

#include <QColor>
#include <QString>
#include <QMap>
#include <array>

namespace c11 {

// Mirrors the Swift GhosttyConfig: loads Ghostty config files and extracts
// display-relevant settings (font, colors, theme, sidebar appearance).
class GhosttyConfig {
public:
    enum class ColorScheme { Light, Dark };

    static GhosttyConfig load(ColorScheme scheme = currentSystemScheme());
    static ColorScheme currentSystemScheme();

    // Font
    QString fontFamily = "Menlo";
    double fontSize = 12.0;

    // Theme
    QString theme;
    QString workingDirectory;
    int scrollbackLimit = 10000;

    // Opacity / splits
    double unfocusedSplitOpacity = 0.7;
    QColor unfocusedSplitFill;
    QColor splitDividerColor;

    // Terminal colors
    QColor backgroundColor{0x27, 0x28, 0x22};
    double backgroundOpacity = 1.0;
    QColor foregroundColor{0xfd, 0xff, 0xf1};
    QColor cursorColor{0xc0, 0xc1, 0xb5};
    QColor cursorTextColor{0x8d, 0x8e, 0x82};
    QColor selectionBackground{0x57, 0x58, 0x4f};
    QColor selectionForeground{0xfd, 0xff, 0xf1};

    // Sidebar
    QString rawSidebarBackground;
    QColor sidebarBackground;
    QColor sidebarBackgroundLight;
    QColor sidebarBackgroundDark;
    double sidebarTintOpacity = -1.0; // negative = unset

    // 16-color palette
    QMap<int, QColor> palette;

    // Derived
    double unfocusedSplitOverlayOpacity() const;
    QColor unfocusedSplitOverlayFill() const;
    QColor resolvedSplitDividerColor() const;

    void parse(const QString &contents);
    void applyContrastFallback();

private:
    static QStringList configPaths();
    static QColor parseHexColor(const QString &hex);
    static bool isLightColor(const QColor &color);
};

} // namespace c11
