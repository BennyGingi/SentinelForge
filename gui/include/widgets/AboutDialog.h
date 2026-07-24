#pragma once

#include <QDialog>

namespace sentinelforge {

// Modeless — reachable from Help > About and the Settings page; stays open
// alongside the rest of the console like RuleDetailDialog.
class AboutDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);
    void showAbout();
};

}  // namespace sentinelforge
