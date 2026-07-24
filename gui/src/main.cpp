#include "MainWindow.h"
#include "telemetry/MockTelemetrySource.h"
#include "telemetry/TelemetryTypes.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleFactory>

#include "Theme.h"

#include <memory>

using namespace sentinelforge;

namespace {

void applyPalette(QApplication& app) {
    QPalette palette;
    const QColor base{QString::fromLatin1(theme::BgBase)};
    const QColor surface{QString::fromLatin1(theme::BgSurface)};
    const QColor text{QString::fromLatin1(theme::TextPrimary)};
    const QColor disabled{QString::fromLatin1(theme::TextMuted)};
    const QColor accent{QString::fromLatin1(theme::Accent)};
    const QColor raised{QString::fromLatin1(theme::BgRaised)};

    palette.setColor(QPalette::Window, base);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, surface);
    palette.setColor(QPalette::AlternateBase, surface);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, raised);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::Highlight, raised);
    palette.setColor(QPalette::HighlightedText, text);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::PlaceholderText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    app.setPalette(palette);
}

void applyStylesheet(QApplication& app) {
    QFile file(QStringLiteral(":/styles/dark.qss"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SentinelForge"));
    QApplication::setOrganizationName(QStringLiteral("SentinelForge"));
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    qRegisterMetaType<RuleInfo>("sentinelforge::RuleInfo");
    qRegisterMetaType<Detection>("sentinelforge::Detection");
    qRegisterMetaType<CorrelationAlert>("sentinelforge::CorrelationAlert");
    qRegisterMetaType<LogLine>("sentinelforge::LogLine");
    qRegisterMetaType<CollectorStats>("sentinelforge::CollectorStats");
    qRegisterMetaType<ConnectionState>("sentinelforge::ConnectionState");
    qRegisterMetaType<QVector<Detection>>("QVector<sentinelforge::Detection>");
    qRegisterMetaType<QVector<CorrelationAlert>>("QVector<sentinelforge::CorrelationAlert>");
    qRegisterMetaType<QVector<LogLine>>("QVector<sentinelforge::LogLine>");

    applyPalette(app);
    applyStylesheet(app);

    // Swap this one line to connect a real collector source later.
    auto source = std::make_unique<MockTelemetrySource>();

    MainWindow window(std::move(source));
    window.show();
    return app.exec();
}
