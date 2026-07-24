#include "MainWindow.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

#include "mitre/MitreCatalog.h"
#include "nav/NavigationController.h"
#include "pages/DashboardPage.h"
#include "pages/PlaceholderPage.h"
#include "widgets/DetectionTableView.h"
#include "widgets/InspectorPane.h"
#include "widgets/LogViewer.h"

#include <QLineEdit>
#include <QTableView>

namespace sentinelforge {

MainWindow::MainWindow(std::unique_ptr<ITelemetrySource> source, QWidget* parent)
    : QMainWindow(parent), source_(std::move(source)) {
    setWindowTitle(QStringLiteral("SentinelForge"));
    resize(1440, 900);
    buildUi();
    buildMenus();
    wireTelemetry();
    restoreState();
}

MainWindow::~MainWindow() {
    if (source_) {
        QMetaObject::invokeMethod(source_.get(), "stop", Qt::BlockingQueuedConnection);
    }
    if (sourceThread_) {
        sourceThread_->quit();
        sourceThread_->wait(3000);
    }
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    navRail_ = new QFrame(central);
    navRail_->setObjectName(QStringLiteral("NavRail"));
    navRail_->setAttribute(Qt::WA_StyledBackground, true);
    navRail_->setFixedWidth(56);
    auto* railLayout = new QVBoxLayout(navRail_);
    railLayout->setContentsMargins(0, 8, 0, 8);
    railLayout->setSpacing(4);

    stack_ = new QStackedWidget(central);
    contentSplitter_ = new QSplitter(Qt::Horizontal, central);
    contentSplitter_->setChildrenCollapsible(false);
    contentSplitter_->addWidget(stack_);

    inspector_ = new InspectorPane(contentSplitter_);
    inspector_->setAttribute(Qt::WA_StyledBackground, true);
    contentSplitter_->addWidget(inspector_);
    contentSplitter_->setStretchFactor(0, 1);
    contentSplitter_->setStretchFactor(1, 0);
    contentSplitter_->setSizes({1100, 0});

    root->addWidget(navRail_);
    root->addWidget(contentSplitter_, 1);

    navigation_ = new NavigationController(navRail_, stack_, this);
    connect(navigation_, &NavigationController::collapsedChanged, this, [this](bool collapsed) {
        navRail_->setFixedWidth(collapsed ? 56 : 200);
    });
    navRail_->setFixedWidth(navigation_->isCollapsed() ? 56 : 200);

    navigation_->addPage(QStringLiteral(":/icons/dashboard.svg"), QStringLiteral("Dashboard"),
                         [this]() {
        dashboard_ = new DashboardPage(stack_);
        connect(dashboard_, &DashboardPage::detectionActivated, inspector_,
                &InspectorPane::showDetection);
        connect(dashboard_, &DashboardPage::alertActivated, inspector_,
                &InspectorPane::showAlert);
        connect(dashboard_, &DashboardPage::openRuleRequested, inspector_,
                &InspectorPane::showRule);
        connect(dashboard_, &DashboardPage::openMitreRequested, this,
                [](const QString& techniqueId) {
                    const QUrl url = MitreCatalog::attackUrl(techniqueId);
                    if (url.isValid()) {
                        QDesktopServices::openUrl(url);
                    }
                });
        inspector_->setDetectionModel(dashboard_->detectionModel());
        if (source_) {
            inspector_->setRuleLookup([this](const QString& ruleId) {
                RuleInfo info;
                QMetaObject::invokeMethod(
                    source_.get(), "ruleInfo", Qt::BlockingQueuedConnection,
                    Q_RETURN_ARG(sentinelforge::RuleInfo, info), Q_ARG(QString, ruleId));
                return info;
            });
            connect(source_.get(), &ITelemetrySource::detectionsReceived, dashboard_,
                    &DashboardPage::onDetections, Qt::QueuedConnection);
            connect(source_.get(), &ITelemetrySource::correlationAlertsReceived, dashboard_,
                    &DashboardPage::onAlerts, Qt::QueuedConnection);
            connect(source_.get(), &ITelemetrySource::logLinesReceived, dashboard_,
                    &DashboardPage::onLogs, Qt::QueuedConnection);
            connect(source_.get(), &ITelemetrySource::statsUpdated, dashboard_,
                    &DashboardPage::onStats, Qt::QueuedConnection);
            connect(source_.get(), &ITelemetrySource::connectionStateChanged, dashboard_,
                    &DashboardPage::onConnectionState, Qt::QueuedConnection);
        }
        return dashboard_;
    });

    navigation_->addPage(QStringLiteral(":/icons/detections.svg"), QStringLiteral("Detections"),
                         [this]() {
        return new PlaceholderPage(
            QStringLiteral("Detections"),
            QStringLiteral("Investigative detection history with search, column filters, and "
                           "export will land here. The live feed remains on Dashboard."),
            stack_);
    });
    navigation_->addPage(QStringLiteral(":/icons/correlation.svg"), QStringLiteral("Correlation"),
                         [this]() {
        return new PlaceholderPage(
            QStringLiteral("Correlation"),
            QStringLiteral("Full correlation alert history and chain inspection will land here."),
            stack_);
    });
    navigation_->addPage(QStringLiteral(":/icons/mitre.svg"), QStringLiteral("MITRE ATT&CK"),
                         [this]() {
        return new PlaceholderPage(
            QStringLiteral("MITRE ATT&CK"),
            QStringLiteral("Technique coverage heatmaps and control mapping are planned next."),
            stack_);
    });
    navigation_->addPage(QStringLiteral(":/icons/timeline.svg"), QStringLiteral("Threat Timeline"),
                         [this]() {
        return new PlaceholderPage(
            QStringLiteral("Threat Timeline"),
            QStringLiteral("Chronological attack-path visualization is deferred."), stack_);
    });
    navigation_->addPage(QStringLiteral(":/icons/infrastructure.svg"),
                         QStringLiteral("Infrastructure"), [this]() {
        return new PlaceholderPage(
            QStringLiteral("Infrastructure"),
            QStringLiteral("Docker and Kubernetes health views share this page (tabbed later)."),
            stack_);
    });

    railLayout->addStretch();

    navigation_->addPage(QStringLiteral(":/icons/settings.svg"), QStringLiteral("Settings"),
                         [this]() {
        return new PlaceholderPage(
            QStringLiteral("Settings"),
            QStringLiteral(
                "Console preferences and collector connection settings will appear here."),
            stack_);
    });

    // Shortcuts (also listed under Help → Keyboard Shortcuts).
    auto addShortcut = [this](const QKeySequence& seq, auto fn) {
        auto* sc = new QShortcut(seq, this);
        connect(sc, &QShortcut::activated, this, fn);
    };
    addShortcut(QKeySequence::Find, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->focusSearch();
        }
    });
    addShortcut(QKeySequence(Qt::Key_Slash), [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->focusSearch();
        }
    });
    addShortcut(QKeySequence(Qt::Key_Space), [this]() {
        if (dashboard_) {
            dashboard_->togglePaused();
        }
    });
    addShortcut(QKeySequence(Qt::Key_F5), [this]() {
        if (dashboard_) {
            dashboard_->setPaused(false);
        }
    });
    addShortcut(QKeySequence(Qt::Key_Escape), [this]() {
        if (dashboard_ && dashboard_->detectionTable()->searchField()->hasFocus()) {
            dashboard_->detectionTable()->clearSearch();
            return;
        }
        inspector_->closeInspector();
    });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), [this]() {
        if (dashboard_) {
            dashboard_->logViewer()->focusEditor();
        }
    });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->table()->setFocus(Qt::ShortcutFocusReason);
        }
    });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_Home), [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectOldest();
        }
    });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_End), [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectNewest();
        }
    });
    for (int i = 0; i < 7; ++i) {
        addShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_1 + i)),
                    [this, i]() { navigation_->setCurrentIndex(i); });
    }
}

void MainWindow::buildMenus() {
    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(QStringLiteral("Pause / Resume\tSpace"), this, [this]() {
        if (dashboard_) {
            dashboard_->togglePaused();
        }
    });
    viewMenu->addAction(QStringLiteral("Resume Live\tF5"), this, [this]() {
        if (dashboard_) {
            dashboard_->setPaused(false);
        }
    });
    viewMenu->addSeparator();
    viewMenu->addAction(QStringLiteral("Focus Search\tCtrl+F"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->focusSearch();
        }
    });
    viewMenu->addAction(QStringLiteral("Focus Detections\tCtrl+D"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->table()->setFocus(Qt::ShortcutFocusReason);
        }
    });
    viewMenu->addAction(QStringLiteral("Focus Logs\tCtrl+L"), this, [this]() {
        if (dashboard_) {
            dashboard_->logViewer()->focusEditor();
        }
    });
    viewMenu->addAction(QStringLiteral("Close Inspector\tEsc"), inspector_,
                        &InspectorPane::closeInspector);

    auto* goMenu = menuBar()->addMenu(QStringLiteral("&Go"));
    goMenu->addAction(QStringLiteral("Newest Detection\tCtrl+End"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectNewest();
        }
    });
    goMenu->addAction(QStringLiteral("Oldest Detection\tCtrl+Home"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectOldest();
        }
    });
    goMenu->addAction(QStringLiteral("Next Detection\tJ"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectRelative(1);
        }
    });
    goMenu->addAction(QStringLiteral("Previous Detection\tK"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->selectRelative(-1);
        }
    });

    auto* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("Copy Row as Text\tC"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->copySelectionAsText();
        }
    });
    editMenu->addAction(QStringLiteral("Copy Row as JSON\tShift+C"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->copySelectionAsJson();
        }
    });
    editMenu->addAction(QStringLiteral("Open Selected\tEnter"), this, [this]() {
        if (dashboard_) {
            dashboard_->detectionTable()->activateSelection();
        }
    });

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("Keyboard Shortcuts"), this, [this]() {
        QMessageBox::information(
            this, QStringLiteral("Keyboard Shortcuts"),
            QStringLiteral(
                "Ctrl+F / /\tFocus search\n"
                "Space\tPause / Resume view\n"
                "F5\tResume live\n"
                "Esc\tClose inspector / clear search\n"
                "Ctrl+D\tFocus detection table\n"
                "Ctrl+L\tFocus logs\n"
                "J / K\tNext / previous detection\n"
                "Enter\tOpen inspector\n"
                "C / Shift+C\tCopy row text / JSON\n"
                "Ctrl+Home / End\tOldest / newest\n"
                "Ctrl+1…7\tSwitch nav page\n\n"
                "Pause freezes the view only. The collector keeps running;\n"
                "the view buffer is bounded and discloses any drops."));
    });
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (dashboard_ && dashboard_->detectionTable()->table()->hasFocus()) {
            dashboard_->detectionTable()->activateSelection();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::wireTelemetry() {
    if (!source_) {
        return;
    }

    sourceThread_ = new QThread(this);
    source_->moveToThread(sourceThread_);
    connect(sourceThread_, &QThread::started, source_.get(), &ITelemetrySource::start);
    sourceThread_->start();
}

void MainWindow::restoreState() {
    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    contentSplitter_->restoreState(
        settings.value(QStringLiteral("contentSplitter")).toByteArray());
}

void MainWindow::saveState() const {
    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("contentSplitter"), contentSplitter_->saveState());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveState();
    if (source_) {
        QMetaObject::invokeMethod(source_.get(), "stop", Qt::QueuedConnection);
    }
    QMainWindow::closeEvent(event);
}

}  // namespace sentinelforge
