#pragma once
#include <QDialog>

class GuideDialog : public QDialog {
    Q_OBJECT
public:
    explicit GuideDialog(QWidget* parent = nullptr);

    // Open directly to a specific tab (0-based).
    void showTab(int index);
};
