#pragma once

#include <QWidget>
#include <array>

namespace sentinelforge {

class SparklineWidget final : public QWidget {
    Q_OBJECT
public:
    static constexpr int kSamples = 48;

    explicit SparklineWidget(QWidget* parent = nullptr);
    void pushSample(float value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::array<float, kSamples> samples_{};
    int write_ = 0;
    int count_ = 0;
};

}  // namespace sentinelforge
