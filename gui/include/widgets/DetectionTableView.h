#pragma once

#include <QWidget>
#include <functional>

#include "models/DetectionModel.h"
#include "telemetry/TelemetryTypes.h"

class QLabel;
class QComboBox;
class QLineEdit;
class QTableView;
class QTimer;
class QMenu;

namespace sentinelforge {

class SeverityRailDelegate;
class Panel;
class DetectionFilterProxy;

class DetectionTableView final : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Live, Investigative };

    explicit DetectionTableView(Mode mode, QWidget* parent = nullptr);

    DetectionModel* model() const { return model_; }
    DetectionFilterProxy* proxy() const { return proxy_; }
    QTableView* table() const { return table_; }
    QLineEdit* searchField() const { return searchField_; }

    void setSeverityFilter(Severity severity, bool enabled);
    void setTimeRangeMinutes(int minutes);  // 0 = all
    void focusSearch();
    void clearSearch();
    void selectRelative(int delta);
    void selectNewest();
    void selectOldest();
    void activateSelection();
    void copySelectionAsText();
    void copySelectionAsJson();
    bool isFilteredEmpty() const;

signals:
    void detectionActivated(sentinelforge::Detection detection);
    void openRuleRequested(QString ruleId);
    void openMitreRequested(QString techniqueId);

private:
    void showEmptyState();
    void onDoubleClicked(const QModelIndex& index);
    void onFilterChanged();
    void onSearchEdited(const QString& text);
    void onSearchDebounce();
    void onRelativeTick();
    void onContextMenu(const QPoint& pos);
    void applyFilters();
    bool eventFilter(QObject* watched, QEvent* event) override;
    const Detection* selectedDetection() const;

    Mode mode_;
    Panel* panel_ = nullptr;
    DetectionModel* model_ = nullptr;
    DetectionFilterProxy* proxy_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* emptyLabel_ = nullptr;
    QLabel* resultCount_ = nullptr;
    QLineEdit* searchField_ = nullptr;
    QComboBox* severityFilter_ = nullptr;
    QComboBox* timeFilter_ = nullptr;
    SeverityRailDelegate* delegate_ = nullptr;
    QTimer* relativeTimer_ = nullptr;
    QTimer* searchDebounce_ = nullptr;
    int timeRangeMinutes_ = 0;
};

}  // namespace sentinelforge
