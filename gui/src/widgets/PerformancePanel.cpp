#include "widgets/PerformancePanel.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "Theme.h"
#include "widgets/Panel.h"
#include "widgets/SparklineWidget.h"

namespace sentinelforge {

namespace {

QWidget* makeStatBlock(const QString& label, QLabel** valueOut, QLabel** unitOut,
                       SparklineWidget** sparkOut, QProgressBar** barOut, QWidget* parent) {
    auto* box = new QWidget(parent);
    box->setAttribute(Qt::WA_StyledBackground, false);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* micro = new QLabel(label.toUpper(), box);
    micro->setObjectName(QStringLiteral("MicroLabel"));
    layout->addWidget(micro);

    auto* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0, 0, 0, 0);
    valueRow->setSpacing(6);
    auto* value = new QLabel(QStringLiteral("0"), box);
    value->setObjectName(QStringLiteral("HeroMetric"));
    valueRow->addWidget(value);
    auto* unit = new QLabel(box);
    unit->setObjectName(QStringLiteral("MetricUnit"));
    valueRow->addWidget(unit);
    valueRow->addStretch();
    layout->addLayout(valueRow);
    *valueOut = value;
    *unitOut = unit;

    if (sparkOut) {
        auto* spark = new SparklineWidget(box);
        layout->addWidget(spark);
        *sparkOut = spark;
    }
    if (barOut) {
        auto* bar = new QProgressBar(box);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(4);
        layout->addWidget(bar);
        *barOut = bar;
    }
    return box;
}

}  // namespace

PerformancePanel::PerformancePanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    panel_ = new Panel(QStringLiteral("Performance"), this);

    auto* body = new QWidget(panel_);
    auto* grid = new QGridLayout(body);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(16);

    grid->addWidget(makeStatBlock(QStringLiteral("Events / sec"), &eventsValue_, &eventsUnit_,
                                  &eventsSpark_, nullptr, body),
                    0, 0);
    grid->addWidget(makeStatBlock(QStringLiteral("Pipeline latency"), &latencyValue_, &latencyUnit_,
                                  &latencySpark_, nullptr, body),
                    0, 1);
    grid->addWidget(
        makeStatBlock(QStringLiteral("CPU"), &cpuValue_, &cpuUnit_, nullptr, &cpuBar_, body), 1, 0);
    grid->addWidget(
        makeStatBlock(QStringLiteral("Memory"), &memValue_, &memUnit_, nullptr, &memBar_, body), 1,
        1);

    eventsUnit_->setText(QStringLiteral("/s"));
    latencyUnit_->setText(QStringLiteral("ms"));
    cpuUnit_->setText(QStringLiteral("%"));
    memUnit_->setText(QStringLiteral("MB"));

    panel_->setContent(body);
    root->addWidget(panel_, 1);
}

void PerformancePanel::onStatsUpdated(CollectorStats stats) {
    eventsValue_->setText(QString::number(stats.eventsPerSecond, 'f', 1));
    latencyValue_->setText(QString::number(stats.pipelineLatencyMs, 'f', 1));
    cpuValue_->setText(QString::number(stats.cpuPercent, 'f', 1));
    memValue_->setText(QString::number(stats.memoryMb, 'f', 0));
    cpuBar_->setValue(qBound(0, 100, static_cast<int>(stats.cpuPercent)));
    memBar_->setValue(qBound(0, 100, static_cast<int>(stats.memoryMb / 4.0)));
    eventsSpark_->pushSample(static_cast<float>(stats.eventsPerSecond));
    latencySpark_->pushSample(static_cast<float>(stats.pipelineLatencyMs));
}

}  // namespace sentinelforge
