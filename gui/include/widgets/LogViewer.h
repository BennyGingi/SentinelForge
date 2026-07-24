#pragma once

#include <QWidget>

#include "telemetry/TelemetryTypes.h"

class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QLineEdit;

namespace sentinelforge {

class Panel;

class LogViewer final : public QWidget {
    Q_OBJECT
public:
    explicit LogViewer(QWidget* parent = nullptr);

    void focusEditor();

public slots:
    void appendLines(const QVector<sentinelforge::LogLine>& lines);

private:
    void applyFilter();
    void onManualScroll(int value);
    void copyVisible();
    QString formatLineHtml(const LogLine& line) const;

    Panel* panel_ = nullptr;
    QPlainTextEdit* editor_ = nullptr;
    QComboBox* levelFilter_ = nullptr;
    QLineEdit* searchField_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QPushButton* resumeButton_ = nullptr;
    bool autoScroll_ = true;
    QVector<LogLine> buffer_;
};

}  // namespace sentinelforge
