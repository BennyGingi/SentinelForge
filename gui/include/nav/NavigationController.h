#pragma once

#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>
#include <utility>
#include <vector>

class QFrame;

namespace sentinelforge {

class NavigationController final : public QObject {
    Q_OBJECT
public:
    using PageFactory = std::function<QWidget*()>;

    NavigationController(QFrame* rail, QStackedWidget* stack, QObject* parent = nullptr);

    // iconPath is a resource path like ":/icons/dashboard.svg".
    int addPage(const QString& iconPath, const QString& title, PageFactory factory);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed_; }

    void setCurrentIndex(int index);
    int currentIndex() const;

signals:
    void pageChanged(int index);
    void collapsedChanged(bool collapsed);

private:
    void ensurePageBuilt(int index);
    void refreshButtonPresentation();
    QIcon tintedIcon(const QString& path, const QColor& color) const;

    struct PageEntry {
        QString title;
        QString iconPath;
        PageFactory factory;
        QWidget* widget = nullptr;
        QToolButton* button = nullptr;
    };

    QFrame* rail_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QVBoxLayout* railLayout_ = nullptr;
    QToolButton* toggleButton_ = nullptr;
    std::vector<PageEntry> pages_;
    bool collapsed_ = true;
};

}  // namespace sentinelforge
