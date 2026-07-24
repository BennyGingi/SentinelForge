#pragma once

#include <QFrame>

class QLabel;
class QPushButton;

namespace sentinelforge {

class LiveControlsBar final : public QFrame {
    Q_OBJECT
public:
    explicit LiveControlsBar(QWidget* parent = nullptr);

    void setPaused(bool paused);
    void setBufferStats(int buffered, int dropped);

signals:
    void liveClicked();
    void pauseClicked();
    void resumeClicked();

private:
    void syncButtons();

    QPushButton* liveButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* resumeButton_ = nullptr;
    QLabel* bufferLabel_ = nullptr;
    bool paused_ = false;
};

}  // namespace sentinelforge
