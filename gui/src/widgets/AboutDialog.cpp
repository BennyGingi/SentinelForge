#include "widgets/AboutDialog.h"

#include <QApplication>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtGlobal>

#include "AppInfo.h"

namespace sentinelforge {

namespace {

QLabel* makeCenteredMicroLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("MicroLabel"));
    label->setAlignment(Qt::AlignHCenter);
    return label;
}

QLabel* makeCenteredValue(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("ChipValue"));
    label->setAlignment(Qt::AlignHCenter);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

}  // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("About SentinelForge"));
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFixedWidth(340);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(10);

    auto* logo = new QLabel(this);
    logo->setPixmap(
        QIcon(QStringLiteral(":/branding/sentinelforge-logo.svg")).pixmap(64, 64));
    logo->setAlignment(Qt::AlignHCenter);
    root->addWidget(logo);

    auto* name = new QLabel(QApplication::applicationName(), this);
    name->setObjectName(QStringLiteral("PanelTitle"));
    name->setAlignment(Qt::AlignHCenter);
    root->addWidget(name);

    root->addSpacing(4);
    root->addWidget(makeCenteredMicroLabel(QStringLiteral("VERSION"), this));
    root->addWidget(makeCenteredValue(QString::fromLatin1(appinfo::Version), this));

    root->addWidget(makeCenteredMicroLabel(QStringLiteral("BUILD"), this));
    root->addWidget(makeCenteredValue(QString::fromLatin1(appinfo::GitCommitHash), this));

    root->addWidget(makeCenteredMicroLabel(QStringLiteral("QT VERSION"), this));
    root->addWidget(makeCenteredValue(QString::fromLatin1(QT_VERSION_STR), this));

    // License intentionally omitted: LICENSE at the repo root is currently
    // empty, so no license is actually established yet. Add a row here once
    // it is.

    auto* link = new QLabel(this);
    link->setObjectName(QStringLiteral("EmptyState"));
    link->setText(QStringLiteral("<a href=\"%1\">%1</a>")
                      .arg(QString::fromLatin1(appinfo::ProjectUrl)));
    link->setOpenExternalLinks(true);
    link->setAlignment(Qt::AlignHCenter);
    root->addSpacing(4);
    root->addWidget(link);

    auto* close = new QPushButton(QStringLiteral("Close"), this);
    close->setObjectName(QStringLiteral("PrimaryButton"));
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    root->addSpacing(8);
    root->addWidget(close, 0, Qt::AlignHCenter);
}

void AboutDialog::showAbout() {
    show();
    raise();
    activateWindow();
}

}  // namespace sentinelforge
