#pragma once

#include <QFrame>
#include <QToolButton>
#include <QVBoxLayout>

class QLabel;

namespace sentinelforge {

class CollapsibleSection final : public QFrame {
    Q_OBJECT
public:
    explicit CollapsibleSection(const QString& title, bool expanded = true,
                                QWidget* parent = nullptr);

    void setContent(QWidget* body);
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded_; }

private:
    void sync();

    QToolButton* toggle_ = nullptr;
    QLabel* title_ = nullptr;
    QWidget* bodyHost_ = nullptr;
    QVBoxLayout* bodyLayout_ = nullptr;
    bool expanded_ = true;
};

}  // namespace sentinelforge
