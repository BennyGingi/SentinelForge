#pragma once

#include <QFrame>
#include <QStack>
#include <functional>

#include "telemetry/TelemetryTypes.h"

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

namespace sentinelforge {

class CollapsibleSection;
class DetectionModel;
class RuleDetailDialog;

class InspectorPane final : public QFrame {
    Q_OBJECT
public:
    using RuleLookup = std::function<RuleInfo(const QString& ruleId)>;

    explicit InspectorPane(QWidget* parent = nullptr);

    void setDetectionModel(DetectionModel* model);
    void setRuleLookup(RuleLookup lookup);

public slots:
    void showDetection(const sentinelforge::Detection& detection);
    void showAlert(const sentinelforge::CorrelationAlert& alert);
    void showRule(const QString& ruleId);
    void clear();
    void closeInspector();

signals:
    void ruleActivated(QString ruleId);

private:
    void buildUi();
    void renderDetection(const Detection& detection, bool pushHistory);
    void goBack();
    void openRuleDetail();
    void openMitreLink();
    void copyText(const QString& text);
    void ensureOpen();
    QWidget* makeKeyValue(const QString& key, QLabel** valueOut, QWidget* parent);

    DetectionModel* detectionModel_ = nullptr;
    RuleLookup ruleLookup_;
    Detection current_;
    QStack<QString> backStack_;
    bool showingDetection_ = false;

    QLabel* title_ = nullptr;
    QToolButton* backButton_ = nullptr;
    QScrollArea* scroll_ = nullptr;

    CollapsibleSection* triageSection_ = nullptr;
    CollapsibleSection* lineageSection_ = nullptr;
    CollapsibleSection* relatedSection_ = nullptr;
    CollapsibleSection* attributionSection_ = nullptr;
    CollapsibleSection* indicatorsSection_ = nullptr;
    CollapsibleSection* rawSection_ = nullptr;

    QLabel* severityValue_ = nullptr;
    QLabel* ruleValue_ = nullptr;
    QPushButton* ruleLinkButton_ = nullptr;
    QLabel* actionValue_ = nullptr;
    QProgressBar* confidenceBar_ = nullptr;
    QLabel* confidencePct_ = nullptr;
    QLabel* occurrencesValue_ = nullptr;
    QLabel* hostValue_ = nullptr;
    QLabel* userValue_ = nullptr;
    QLabel* timeValue_ = nullptr;

    QLabel* parentLine_ = nullptr;
    QLabel* processLine_ = nullptr;
    QPlainTextEdit* commandLine_ = nullptr;
    QToolButton* copyCommandButton_ = nullptr;

    QVBoxLayout* relatedList_ = nullptr;
    QLabel* relatedEmpty_ = nullptr;

    QLabel* mitreName_ = nullptr;
    QLabel* mitreSub_ = nullptr;
    QLabel* mitreId_ = nullptr;
    QLabel* mitreDesc_ = nullptr;
    QToolButton* mitreLinkButton_ = nullptr;

    QVBoxLayout* iocList_ = nullptr;
    QLabel* iocEmpty_ = nullptr;

    QPlainTextEdit* rawJson_ = nullptr;
    QToolButton* copyRawButton_ = nullptr;

    QLabel* placeholder_ = nullptr;
    QWidget* detectionRoot_ = nullptr;
    QPlainTextEdit* alertBody_ = nullptr;

    RuleDetailDialog* ruleDialog_ = nullptr;
};

}  // namespace sentinelforge
