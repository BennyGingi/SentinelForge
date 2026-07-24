#include "widgets/InspectorPane.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "Theme.h"
#include "mitre/MitreCatalog.h"
#include "models/DetectionModel.h"
#include "util/TimestampFormat.h"
#include "widgets/CollapsibleSection.h"
#include "widgets/RuleDetailDialog.h"

namespace sentinelforge {

namespace {

QFrame* makeDivider(QWidget* parent) {
    auto* line = new QFrame(parent);
    line->setObjectName(QStringLiteral("SectionDivider"));
    line->setFrameShape(QFrame::HLine);
    line->setFixedHeight(1);
    return line;
}

}  // namespace

InspectorPane::InspectorPane(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("InspectorPane"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(0);
    setMaximumWidth(420);
    MitreCatalog::instance().load();
    buildUi();
    hide();
}

void InspectorPane::setDetectionModel(DetectionModel* model) { detectionModel_ = model; }

void InspectorPane::setRuleLookup(RuleLookup lookup) { ruleLookup_ = std::move(lookup); }

void InspectorPane::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    backButton_ = new QToolButton(header);
    backButton_->setText(QStringLiteral("← Back"));
    backButton_->setObjectName(QStringLiteral("SectionToggle"));
    backButton_->setVisible(false);
    connect(backButton_, &QToolButton::clicked, this, &InspectorPane::goBack);
    headerLayout->addWidget(backButton_);

    title_ = new QLabel(QStringLiteral("Inspector"), header);
    title_->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout->addWidget(title_, 1);
    root->addWidget(header);

    placeholder_ = new QLabel(
        QStringLiteral("Select a detection or correlation alert to inspect details."), this);
    placeholder_->setObjectName(QStringLiteral("EmptyState"));
    placeholder_->setWordWrap(true);
    root->addWidget(placeholder_);

    alertBody_ = new QPlainTextEdit(this);
    alertBody_->setReadOnly(true);
    alertBody_->setObjectName(QStringLiteral("LogViewer"));
    alertBody_->setVisible(false);
    root->addWidget(alertBody_, 1);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setVisible(false);

    detectionRoot_ = new QWidget(scroll_);
    auto* bodyLayout = new QVBoxLayout(detectionRoot_);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // --- Triage ---
    triageSection_ = new CollapsibleSection(QStringLiteral("Triage"), true, detectionRoot_);
    auto* triage = new QWidget(triageSection_);
    auto* triageLayout = new QVBoxLayout(triage);
    triageLayout->setContentsMargins(0, 0, 0, 0);
    triageLayout->setSpacing(6);
    triageLayout->addWidget(makeKeyValue(QStringLiteral("Severity"), &severityValue_, triage));
    {
        auto* row = new QWidget(triage);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        auto* key = new QLabel(QStringLiteral("Rule"), row);
        key->setObjectName(QStringLiteral("EmptyState"));
        rowLayout->addWidget(key);
        ruleValue_ = new QLabel(row);
        ruleValue_->setWordWrap(true);
        rowLayout->addWidget(ruleValue_, 1);
        ruleLinkButton_ = new QPushButton(QStringLiteral("Details"), row);
        ruleLinkButton_->setFixedWidth(72);
        connect(ruleLinkButton_, &QPushButton::clicked, this, &InspectorPane::openRuleDetail);
        rowLayout->addWidget(ruleLinkButton_);
        triageLayout->addWidget(row);
    }
    triageLayout->addWidget(makeKeyValue(QStringLiteral("Action"), &actionValue_, triage));
    {
        auto* conf = new QWidget(triage);
        auto* confLayout = new QHBoxLayout(conf);
        confLayout->setContentsMargins(0, 0, 0, 0);
        auto* key = new QLabel(QStringLiteral("Confidence"), conf);
        key->setObjectName(QStringLiteral("EmptyState"));
        confLayout->addWidget(key);
        confidenceBar_ = new QProgressBar(conf);
        confidenceBar_->setRange(0, 100);
        confidenceBar_->setTextVisible(false);
        confidenceBar_->setFixedHeight(4);
        confLayout->addWidget(confidenceBar_, 1);
        confidencePct_ = new QLabel(conf);
        confidencePct_->setObjectName(QStringLiteral("ChipValue"));
        confLayout->addWidget(confidencePct_);
        triageLayout->addWidget(conf);
    }
    triageLayout->addWidget(makeKeyValue(QStringLiteral("Occurrences"), &occurrencesValue_, triage));
    triageLayout->addWidget(makeKeyValue(QStringLiteral("Host"), &hostValue_, triage));
    triageLayout->addWidget(makeKeyValue(QStringLiteral("User"), &userValue_, triage));
    triageLayout->addWidget(makeKeyValue(QStringLiteral("Time"), &timeValue_, triage));
    triageSection_->setContent(triage);
    bodyLayout->addWidget(triageSection_);
    bodyLayout->addWidget(makeDivider(detectionRoot_));

    // --- Process lineage ---
    lineageSection_ = new CollapsibleSection(QStringLiteral("Process lineage"), true, detectionRoot_);
    auto* lineage = new QWidget(lineageSection_);
    auto* lineageLayout = new QVBoxLayout(lineage);
    lineageLayout->setContentsMargins(0, 0, 0, 0);
    lineageLayout->setSpacing(6);
    parentLine_ = new QLabel(lineage);
    parentLine_->setWordWrap(true);
    lineageLayout->addWidget(parentLine_);
    auto* arrow = new QLabel(QStringLiteral("↓"), lineage);
    arrow->setAlignment(Qt::AlignHCenter);
    arrow->setObjectName(QStringLiteral("EmptyState"));
    lineageLayout->addWidget(arrow);
    processLine_ = new QLabel(lineage);
    processLine_->setWordWrap(true);
    lineageLayout->addWidget(processLine_);
    {
        auto* cmdHeader = new QWidget(lineage);
        auto* cmdHeaderLayout = new QHBoxLayout(cmdHeader);
        cmdHeaderLayout->setContentsMargins(0, 0, 0, 0);
        auto* cmdLabel = new QLabel(QStringLiteral("COMMAND LINE"), cmdHeader);
        cmdLabel->setObjectName(QStringLiteral("MicroLabel"));
        cmdHeaderLayout->addWidget(cmdLabel);
        cmdHeaderLayout->addStretch();
        copyCommandButton_ = new QToolButton(cmdHeader);
        copyCommandButton_->setText(QStringLiteral("Copy"));
        connect(copyCommandButton_, &QToolButton::clicked, this,
                [this]() { copyText(commandLine_->toPlainText()); });
        cmdHeaderLayout->addWidget(copyCommandButton_);
        lineageLayout->addWidget(cmdHeader);
    }
    commandLine_ = new QPlainTextEdit(lineage);
    commandLine_->setObjectName(QStringLiteral("CommandLineView"));
    commandLine_->setReadOnly(true);
    commandLine_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    commandLine_->setMaximumHeight(120);
    lineageLayout->addWidget(commandLine_);
    lineageSection_->setContent(lineage);
    bodyLayout->addWidget(lineageSection_);
    bodyLayout->addWidget(makeDivider(detectionRoot_));

    // --- Related ---
    relatedSection_ = new CollapsibleSection(QStringLiteral("Related detections"), false, detectionRoot_);
    auto* related = new QWidget(relatedSection_);
    relatedList_ = new QVBoxLayout(related);
    relatedList_->setContentsMargins(0, 0, 0, 0);
    relatedList_->setSpacing(4);
    relatedEmpty_ = new QLabel(QStringLiteral("No related detections."), related);
    relatedEmpty_->setObjectName(QStringLiteral("EmptyState"));
    relatedList_->addWidget(relatedEmpty_);
    relatedSection_->setContent(related);
    bodyLayout->addWidget(relatedSection_);
    bodyLayout->addWidget(makeDivider(detectionRoot_));

    // --- Attribution ---
    attributionSection_ =
        new CollapsibleSection(QStringLiteral("Attribution"), true, detectionRoot_);
    auto* attrib = new QWidget(attributionSection_);
    auto* attribLayout = new QVBoxLayout(attrib);
    attribLayout->setContentsMargins(0, 0, 0, 0);
    attribLayout->setSpacing(4);
    mitreName_ = new QLabel(attrib);
    mitreName_->setWordWrap(true);
    attribLayout->addWidget(mitreName_);
    mitreSub_ = new QLabel(attrib);
    mitreSub_->setWordWrap(true);
    attribLayout->addWidget(mitreSub_);
    mitreId_ = new QLabel(attrib);
    mitreId_->setObjectName(QStringLiteral("EmptyState"));
    attribLayout->addWidget(mitreId_);
    mitreDesc_ = new QLabel(attrib);
    mitreDesc_->setWordWrap(true);
    mitreDesc_->setObjectName(QStringLiteral("EmptyState"));
    attribLayout->addWidget(mitreDesc_);
    mitreLinkButton_ = new QToolButton(attrib);
    mitreLinkButton_->setText(QStringLiteral("Open ATT&&CK page"));
    connect(mitreLinkButton_, &QToolButton::clicked, this, &InspectorPane::openMitreLink);
    attribLayout->addWidget(mitreLinkButton_, 0, Qt::AlignLeft);
    attributionSection_->setContent(attrib);
    bodyLayout->addWidget(attributionSection_);
    bodyLayout->addWidget(makeDivider(detectionRoot_));

    // --- Indicators ---
    indicatorsSection_ = new CollapsibleSection(QStringLiteral("Indicators"), true, detectionRoot_);
    auto* iocs = new QWidget(indicatorsSection_);
    iocList_ = new QVBoxLayout(iocs);
    iocList_->setContentsMargins(0, 0, 0, 0);
    iocList_->setSpacing(4);
    iocEmpty_ = new QLabel(QStringLiteral("No indicators."), iocs);
    iocEmpty_->setObjectName(QStringLiteral("EmptyState"));
    iocList_->addWidget(iocEmpty_);
    indicatorsSection_->setContent(iocs);
    bodyLayout->addWidget(indicatorsSection_);
    bodyLayout->addWidget(makeDivider(detectionRoot_));

    // --- Raw ---
    rawSection_ = new CollapsibleSection(QStringLiteral("Raw event"), false, detectionRoot_);
    auto* raw = new QWidget(rawSection_);
    auto* rawLayout = new QVBoxLayout(raw);
    rawLayout->setContentsMargins(0, 0, 0, 0);
    auto* rawHeader = new QHBoxLayout();
    rawHeader->addStretch();
    copyRawButton_ = new QToolButton(raw);
    copyRawButton_->setText(QStringLiteral("Copy all"));
    connect(copyRawButton_, &QToolButton::clicked, this,
            [this]() { copyText(rawJson_->toPlainText()); });
    rawHeader->addWidget(copyRawButton_);
    rawLayout->addLayout(rawHeader);
    rawJson_ = new QPlainTextEdit(raw);
    rawJson_->setObjectName(QStringLiteral("LogViewer"));
    rawJson_->setReadOnly(true);
    rawJson_->setMinimumHeight(180);
    rawLayout->addWidget(rawJson_);
    rawSection_->setContent(raw);
    bodyLayout->addWidget(rawSection_);
    bodyLayout->addStretch();

    scroll_->setWidget(detectionRoot_);
    root->addWidget(scroll_, 1);

    ruleDialog_ = new RuleDetailDialog(this);
}

QWidget* InspectorPane::makeKeyValue(const QString& key, QLabel** valueOut, QWidget* parent) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    auto* keyLabel = new QLabel(key, row);
    keyLabel->setObjectName(QStringLiteral("EmptyState"));
    keyLabel->setMinimumWidth(88);
    layout->addWidget(keyLabel);
    auto* value = new QLabel(row);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(value, 1);
    *valueOut = value;
    return row;
}

void InspectorPane::showDetection(const Detection& detection) {
    backStack_.clear();
    renderDetection(detection, false);
    ensureOpen();
}

void InspectorPane::ensureOpen() {
    show();
    if (auto* splitter = qobject_cast<QSplitter*>(parentWidget())) {
        QList<int> sizes = splitter->sizes();
        if (sizes.size() >= 2 && sizes.at(1) < 280) {
            const int total = sizes.at(0) + sizes.at(1);
            splitter->setSizes({qMax(400, total - 420), 420});
        }
    }
}

void InspectorPane::renderDetection(const Detection& detection, bool pushHistory) {
    if (pushHistory && showingDetection_ && !current_.id.isEmpty() && current_.id != detection.id) {
        backStack_.push(current_.id);
    }
    current_ = detection;
    showingDetection_ = true;

    ensureOpen();
    placeholder_->setVisible(false);
    alertBody_->setVisible(false);
    scroll_->setVisible(true);
    backButton_->setVisible(!backStack_.isEmpty());
    title_->setText(QStringLiteral("Detection"));

    severityValue_->setText(severityLabel(detection.severity));
    ruleValue_->setText(detection.ruleName);
    actionValue_->setText(detection.recommendedAction);
    confidenceBar_->setValue(qBound(0, 100, detection.confidence));
    confidencePct_->setText(QStringLiteral("%1%").arg(detection.confidence));
    occurrencesValue_->setText(QString::number(detection.occurrences));
    hostValue_->setText(detection.host);
    userValue_->setText(detection.user);
    timeValue_->setText(timefmt::tooltipTime(detection.timestampMs, QDateTime::currentMSecsSinceEpoch()));

    parentLine_->setText(QStringLiteral("%1  (PID %2)")
                             .arg(detection.parentProcessName.isEmpty() ? QStringLiteral("—")
                                                                        : detection.parentProcessName)
                             .arg(detection.parentPid));
    processLine_->setText(
        QStringLiteral("%1  (PID %2)").arg(detection.processName).arg(detection.pid));
    commandLine_->setPlainText(detection.commandLine);

    // Related detections — resolve ids against the model only (no inference).
    QWidget* relatedHost = relatedList_->parentWidget();
    while (QLayoutItem* item = relatedList_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    int shown = 0;
    if (detectionModel_) {
        for (const QString& id : detection.relatedIds) {
            if (shown >= 8) {
                break;
            }
            const Detection* related = detectionModel_->findById(id);
            if (!related) {
                continue;
            }
            auto* btn = new QPushButton(
                QStringLiteral("%1  %2  %3")
                    .arg(severityLabel(related->severity), timefmt::columnTime(related->timestampMs),
                         related->ruleName),
                relatedHost);
            const Detection copy = *related;
            connect(btn, &QPushButton::clicked, this, [this, copy]() { renderDetection(copy, true); });
            relatedList_->addWidget(btn);
            ++shown;
        }
    }
    if (shown == 0) {
        relatedEmpty_ = new QLabel(QStringLiteral("No related detections."), relatedHost);
        relatedEmpty_->setObjectName(QStringLiteral("EmptyState"));
        relatedList_->addWidget(relatedEmpty_);
    } else {
        relatedEmpty_ = nullptr;
    }

    const MitreTechnique tech = MitreCatalog::instance().lookup(detection.techniqueId);
    if (detection.techniqueId.isEmpty()) {
        mitreName_->setText(QStringLiteral("No MITRE technique mapped"));
        mitreSub_->clear();
        mitreId_->clear();
        mitreDesc_->clear();
        mitreLinkButton_->setEnabled(false);
    } else {
        mitreName_->setText(tech.name.isEmpty() ? detection.techniqueId : tech.name);
        mitreSub_->setText(tech.subtechnique);
        mitreSub_->setVisible(!tech.subtechnique.isEmpty());
        mitreId_->setText(detection.techniqueId);
        mitreDesc_->setText(tech.description);
        mitreLinkButton_->setEnabled(MitreCatalog::isValidTechniqueId(detection.techniqueId));
    }

    while (QLayoutItem* item = iocList_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    QWidget* iocHost = iocList_->parentWidget();
    if (detection.iocs.isEmpty()) {
        iocEmpty_ = new QLabel(QStringLiteral("No indicators."), iocHost);
        iocEmpty_->setObjectName(QStringLiteral("EmptyState"));
        iocList_->addWidget(iocEmpty_);
    } else {
        for (const Ioc& ioc : detection.iocs) {
            auto* row = new QWidget(iocHost);
            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            auto* type = new QLabel(iocTypeLabel(ioc.type), row);
            type->setObjectName(QStringLiteral("MicroLabel"));
            type->setFixedWidth(96);
            rowLayout->addWidget(type);
            auto* value = new QLabel(ioc.value, row);
            value->setTextInteractionFlags(Qt::TextSelectableByMouse);
            value->setWordWrap(true);
            rowLayout->addWidget(value, 1);
            auto* copy = new QToolButton(row);
            copy->setText(QStringLiteral("Copy"));
            const QString v = ioc.value;
            connect(copy, &QToolButton::clicked, this, [this, v]() { copyText(v); });
            rowLayout->addWidget(copy);
            iocList_->addWidget(row);
        }
    }

    // Raw viewer displays verbatim — no parsing.
    rawJson_->setPlainText(detection.rawEventJson);
}

void InspectorPane::showAlert(const CorrelationAlert& alert) {
    showingDetection_ = false;
    backStack_.clear();
    backButton_->setVisible(false);
    ensureOpen();
    placeholder_->setVisible(false);
    scroll_->setVisible(false);
    alertBody_->setVisible(true);
    title_->setText(QStringLiteral("Correlation alert"));
    alertBody_->setPlainText(
        QStringLiteral("Title: %1\nSeverity: %2\nConfidence: %3%\nEvents: %4\nTime: %5\n\n%6")
            .arg(alert.title, severityLabel(alert.severity))
            .arg(alert.confidence)
            .arg(alert.matchedEventCount)
            .arg(timefmt::tooltipTime(alert.timestampMs, QDateTime::currentMSecsSinceEpoch()),
                 alert.description));
}

void InspectorPane::clear() {
    showingDetection_ = false;
    backStack_.clear();
    backButton_->setVisible(false);
    title_->setText(QStringLiteral("Inspector"));
    placeholder_->setVisible(true);
    scroll_->setVisible(false);
    alertBody_->setVisible(false);
}

void InspectorPane::closeInspector() {
    clear();
    hide();
    if (auto* splitter = qobject_cast<QSplitter*>(parentWidget())) {
        QList<int> sizes = splitter->sizes();
        if (sizes.size() >= 2) {
            const int total = sizes.at(0) + sizes.at(1);
            splitter->setSizes({total, 0});
        }
    }
}

void InspectorPane::goBack() {
    if (backStack_.isEmpty() || !detectionModel_) {
        return;
    }
    const QString id = backStack_.pop();
    const Detection* d = detectionModel_->findById(id);
    if (!d) {
        backButton_->setVisible(!backStack_.isEmpty());
        return;
    }
    renderDetection(*d, false);
    backButton_->setVisible(!backStack_.isEmpty());
}

void InspectorPane::openRuleDetail() {
    showRule(current_.ruleId);
}

void InspectorPane::showRule(const QString& ruleId) {
    if (!ruleLookup_) {
        return;
    }
    const RuleInfo info = ruleLookup_(ruleId);
    if (info.ruleId.isEmpty() && info.name.isEmpty()) {
        RuleInfo fallback;
        fallback.ruleId = ruleId.isEmpty() ? current_.ruleId : ruleId;
        fallback.name = current_.ruleName;
        fallback.description = current_.ruleDescription;
        fallback.techniqueId = current_.techniqueId;
        fallback.severity = current_.severity;
        ruleDialog_->showRule(fallback);
    } else {
        ruleDialog_->showRule(info);
    }
    emit ruleActivated(ruleId);
}

void InspectorPane::openMitreLink() {
    const QUrl url = MitreCatalog::attackUrl(current_.techniqueId);
    if (url.isValid()) {
        QDesktopServices::openUrl(url);
    }
}

void InspectorPane::copyText(const QString& text) {
    if (QClipboard* clip = QApplication::clipboard()) {
        clip->setText(text);
    }
}

}  // namespace sentinelforge
