#pragma once

#include <QDialog>

#include "telemetry/TelemetryTypes.h"

class QLabel;
class QTextEdit;

namespace sentinelforge {

// Modeless rule detail — analyst can keep the alert list visible.
class RuleDetailDialog final : public QDialog {
    Q_OBJECT
public:
    explicit RuleDetailDialog(QWidget* parent = nullptr);
    void showRule(const RuleInfo& info);

private:
    QLabel* title_ = nullptr;
    QLabel* meta_ = nullptr;
    QTextEdit* falsePositives_ = nullptr;
    QTextEdit* investigationNotes_ = nullptr;
    QTextEdit* logic_ = nullptr;
    QTextEdit* description_ = nullptr;
};

}  // namespace sentinelforge
