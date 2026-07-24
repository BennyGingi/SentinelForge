#pragma once

#include <QMainWindow>
#include <memory>

#include "telemetry/ITelemetrySource.h"

class QSplitter;
class QStackedWidget;
class QFrame;
class QThread;
class QKeyEvent;

namespace sentinelforge {

class NavigationController;
class DashboardPage;
class InspectorPane;
class AboutDialog;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::unique_ptr<ITelemetrySource> source, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi();
    void buildMenus();
    void wireTelemetry();
    void restoreState();
    void saveState() const;

    std::unique_ptr<ITelemetrySource> source_;
    QThread* sourceThread_ = nullptr;

    QFrame* navRail_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QSplitter* contentSplitter_ = nullptr;
    NavigationController* navigation_ = nullptr;
    DashboardPage* dashboard_ = nullptr;
    InspectorPane* inspector_ = nullptr;
    AboutDialog* aboutDialog_ = nullptr;
};

}  // namespace sentinelforge
