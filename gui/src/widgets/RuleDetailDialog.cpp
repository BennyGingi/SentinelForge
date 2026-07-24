#include "widgets/RuleDetailDialog.h"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

#include "Theme.h"

namespace sentinelforge {

namespace {

QTextEdit* makeBlock(const QString& objectName, QWidget* parent) {
    auto* edit = new QTextEdit(parent);
    edit->setObjectName(objectName);
    edit->setReadOnly(true);
    edit->setFrameShape(QFrame::NoFrame);
    edit->setMaximumHeight(120);
    return edit;
}

}  // namespace

RuleDetailDialog::RuleDetailDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Rule detail"));
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    resize(480, 640);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    title_ = new QLabel(this);
    title_->setObjectName(QStringLiteral("PanelTitle"));
    title_->setWordWrap(true);
    root->addWidget(title_);

    meta_ = new QLabel(this);
    meta_->setObjectName(QStringLiteral("EmptyState"));
    meta_->setWordWrap(true);
    root->addWidget(meta_);

    auto* fpLabel = new QLabel(QStringLiteral("KNOWN FALSE POSITIVES"), this);
    fpLabel->setObjectName(QStringLiteral("MicroLabel"));
    root->addWidget(fpLabel);
    falsePositives_ = makeBlock(QStringLiteral("RuleBlock"), this);
    root->addWidget(falsePositives_);

    auto* notesLabel = new QLabel(QStringLiteral("INVESTIGATION NOTES"), this);
    notesLabel->setObjectName(QStringLiteral("MicroLabel"));
    root->addWidget(notesLabel);
    investigationNotes_ = makeBlock(QStringLiteral("RuleBlock"), this);
    root->addWidget(investigationNotes_);

    auto* logicLabel = new QLabel(QStringLiteral("DETECTION LOGIC"), this);
    logicLabel->setObjectName(QStringLiteral("MicroLabel"));
    root->addWidget(logicLabel);
    logic_ = makeBlock(QStringLiteral("RuleBlock"), this);
    root->addWidget(logic_);

    auto* descLabel = new QLabel(QStringLiteral("DESCRIPTION"), this);
    descLabel->setObjectName(QStringLiteral("MicroLabel"));
    root->addWidget(descLabel);
    description_ = makeBlock(QStringLiteral("RuleBlock"), this);
    root->addWidget(description_, 1);
}

void RuleDetailDialog::showRule(const RuleInfo& info) {
    title_->setText(info.name.isEmpty() ? info.ruleId : info.name);
    meta_->setText(QStringLiteral("%1 · %2 · %3 · MITRE %4")
                       .arg(info.ruleId, info.author, info.version,
                            info.techniqueId.isEmpty() ? QStringLiteral("—") : info.techniqueId));
    falsePositives_->setPlainText(info.knownFalsePositives.isEmpty()
                                      ? QStringLiteral("None documented.")
                                      : info.knownFalsePositives);
    investigationNotes_->setPlainText(info.investigationNotes.isEmpty()
                                          ? QStringLiteral("None documented.")
                                          : info.investigationNotes);
    logic_->setPlainText(info.logicSummary);
    description_->setPlainText(info.description);
    if (info.sigmaSource.size() > 0) {
        description_->append(QStringLiteral("\n\nSigma: %1").arg(info.sigmaSource));
    }
    show();
    raise();
    activateWindow();
}

}  // namespace sentinelforge
