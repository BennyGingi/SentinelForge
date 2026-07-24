#pragma once

#include <QFrame>

#include "telemetry/TelemetryTypes.h"

class QLabel;
class QTimer;

namespace sentinelforge {

// Non-modal toast for new critical detections. Never steals focus.
class CriticalToast final : public QFrame {
    Q_OBJECT
public:
    explicit CriticalToast(QWidget* parent = nullptr);
    void showDetection(const Detection& detection);

private:
    QLabel* title_ = nullptr;
    QLabel* body_ = nullptr;
    QTimer* hideTimer_ = nullptr;
};

}  // namespace sentinelforge
