#include "GhosttyConfig.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QStyleHints>
#include <QTextStream>
#include <QDebug>

namespace c11 {

GhosttyConfig::ColorScheme GhosttyConfig::currentSystemScheme()
{
    auto *hints = QGuiApplication::styleHints();
    if (hints && hints->colorScheme() == Qt::ColorScheme::Light) {
        return ColorScheme::Light;
    }
    return ColorScheme::Dark;
}

GhosttyConfig GhosttyConfig::load(ColorScheme scheme)
{
    GhosttyConfig config;

    // Load from standard Ghostty config paths
    for (const auto &path : configPaths()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config.parse(QString::fromUtf8(file.readAll()));
        }
    }

    config.applyContrastFallback();
    return config;
}

QStringList GhosttyConfig::configPaths()
{
    QStringList paths;
    QString home = QDir::homePath();

    paths << home + "/.config/ghostty/config"
          << home + "/.config/ghostty/config.ghostty";

#ifdef Q_OS_MACOS
    paths << home + "/Library/Application Support/com.mitchellh.ghostty/config"
          << home + "/Library/Application Support/com.mitchellh.ghostty/config.ghostty"
          << home + "/Library/Application Support/com.stage11.c11/config"
          << home + "/Library/Application Support/com.stage11.c11/config.ghostty";
#else
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!configDir.isEmpty()) {
        paths << configDir + "/c11/config"
              << configDir + "/c11/config.ghostty";
    }
#endif

    return paths;
}

void GhosttyConfig::parse(const QString &contents)
{
    QTextStream stream(const_cast<QString *>(&contents), QIODevice::ReadOnly);
    QString line;
    while (stream.readLineInto(&line)) {
        auto trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;

        auto eqIdx = trimmed.indexOf('=');
        if (eqIdx < 0) continue;

        auto key = trimmed.left(eqIdx).trimmed();
        auto value = trimmed.mid(eqIdx + 1).trimmed();
        // Strip surrounding quotes
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }

        if (key == "font-family") {
            fontFamily = value;
        } else if (key == "font-size") {
            bool ok;
            double sz = value.toDouble(&ok);
            if (ok) fontSize = sz;
        } else if (key == "theme") {
            theme = value;
        } else if (key == "working-directory") {
            workingDirectory = value;
        } else if (key == "scrollback-limit") {
            bool ok;
            int limit = value.toInt(&ok);
            if (ok) scrollbackLimit = limit;
        } else if (key == "background") {
            auto c = parseHexColor(value);
            if (c.isValid()) backgroundColor = c;
        } else if (key == "background-opacity") {
            bool ok;
            double op = value.toDouble(&ok);
            if (ok) backgroundOpacity = op;
        } else if (key == "foreground") {
            auto c = parseHexColor(value);
            if (c.isValid()) foregroundColor = c;
        } else if (key == "cursor-color") {
            auto c = parseHexColor(value);
            if (c.isValid()) cursorColor = c;
        } else if (key == "cursor-text") {
            auto c = parseHexColor(value);
            if (c.isValid()) cursorTextColor = c;
        } else if (key == "selection-background") {
            auto c = parseHexColor(value);
            if (c.isValid()) selectionBackground = c;
        } else if (key == "selection-foreground") {
            auto c = parseHexColor(value);
            if (c.isValid()) selectionForeground = c;
        } else if (key == "unfocused-split-opacity") {
            bool ok;
            double op = value.toDouble(&ok);
            if (ok) unfocusedSplitOpacity = op;
        } else if (key == "unfocused-split-fill") {
            auto c = parseHexColor(value);
            if (c.isValid()) unfocusedSplitFill = c;
        } else if (key == "split-divider-color") {
            auto c = parseHexColor(value);
            if (c.isValid()) splitDividerColor = c;
        } else if (key == "c11-sidebar-background" || key == "cmux-sidebar-background") {
            rawSidebarBackground = value;
        } else if (key == "c11-sidebar-tint-opacity" || key == "cmux-sidebar-tint-opacity") {
            bool ok;
            double op = value.toDouble(&ok);
            if (ok) sidebarTintOpacity = op;
        } else if (key.startsWith("palette")) {
            // palette = N=#RRGGBB
            auto parts = value.split('=');
            if (parts.size() == 2) {
                bool ok;
                int idx = parts[0].trimmed().toInt(&ok);
                if (ok && idx >= 0 && idx < 256) {
                    auto c = parseHexColor(parts[1].trimmed());
                    if (c.isValid()) palette[idx] = c;
                }
            }
        }
    }
}

QColor GhosttyConfig::parseHexColor(const QString &hex)
{
    QString h = hex.trimmed();
    if (!h.startsWith('#')) h.prepend('#');
    return QColor::fromString(h);
}

bool GhosttyConfig::isLightColor(const QColor &color)
{
    // Perceived luminance
    double lum = 0.299 * color.redF() + 0.587 * color.greenF() + 0.114 * color.blueF();
    return lum > 0.5;
}

double GhosttyConfig::unfocusedSplitOverlayOpacity() const
{
    double clamped = qBound(0.15, unfocusedSplitOpacity, 1.0);
    return qBound(0.0, 1.0 - clamped, 1.0);
}

QColor GhosttyConfig::unfocusedSplitOverlayFill() const
{
    return unfocusedSplitFill.isValid() ? unfocusedSplitFill : backgroundColor;
}

QColor GhosttyConfig::resolvedSplitDividerColor() const
{
    if (splitDividerColor.isValid()) return splitDividerColor;

    bool light = isLightColor(backgroundColor);
    double factor = light ? 0.08 : 0.4;
    return backgroundColor.darker(100 + static_cast<int>(factor * 100));
}

void GhosttyConfig::applyContrastFallback()
{
    if (isLightColor(backgroundColor) && isLightColor(foregroundColor)) {
        foregroundColor = QColor(0x1A, 0x1A, 0x1A);
    }
}

} // namespace c11
