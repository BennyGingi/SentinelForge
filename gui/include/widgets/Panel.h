#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace sentinelforge {

// Shared panel chrome. QSS background only paints on QFrame (or widgets with
// WA_StyledBackground). Every dashboard card is a Panel.
class Panel : public QFrame {
    Q_OBJECT
public:
    explicit Panel(const QString& title, QWidget* parent = nullptr);

    void setContent(QWidget* body);
    void setHeaderAction(QWidget* action);
    QVBoxLayout* contentLayout() const { return contentLayout_; }

private:
    QLabel* titleLabel_ = nullptr;
    QHBoxLayout* headerLayout_ = nullptr;
    QVBoxLayout* rootLayout_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
};

}  // namespace sentinelforge
