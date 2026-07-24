#include "widgets/SparklineWidget.h"

#include <QPainter>
#include <QPainterPath>

#include "Theme.h"

namespace sentinelforge {

SparklineWidget::SparklineWidget(QWidget* parent) : QWidget(parent) {
    setFixedHeight(28);
    setMinimumWidth(80);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);
    samples_.fill(0.f);
}

void SparklineWidget::pushSample(float value) {
    samples_[static_cast<size_t>(write_)] = value;
    write_ = (write_ + 1) % kSamples;
    count_ = qMin(count_ + 1, kSamples);
    update();
}

void SparklineWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (count_ < 2) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(QLatin1String(theme::Accent)), 1.0));

    float minV = samples_[0];
    float maxV = samples_[0];
    for (int i = 0; i < count_; ++i) {
        const float v = samples_[static_cast<size_t>(i)];
        minV = qMin(minV, v);
        maxV = qMax(maxV, v);
    }
    const float range = qMax(0.001f, maxV - minV);

    QPainterPath path;
    for (int i = 0; i < count_; ++i) {
        const int idx = (write_ - count_ + i + kSamples) % kSamples;
        const float v = samples_[static_cast<size_t>(idx)];
        const float x = width() * static_cast<float>(i) / static_cast<float>(count_ - 1);
        const float y = height() - ((v - minV) / range) * static_cast<float>(height() - 2) - 1.f;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    p.drawPath(path);
}

}  // namespace sentinelforge
