#ifndef MENU_H
#define MENU_H

#include <QWidget>
#include <QFrame>

namespace Ui {
class Menu;
}

class QAbstractButton;

class Menu : public QFrame
{
    Q_OBJECT

public:
    explicit Menu(QWidget* parent = nullptr);
    ~Menu();

signals:
    void copyButtonClicked();
    void connectionButtonToggled(bool checked);
    void clearButtonClicked();

private slots:
    void on_clearButton_clicked();
    void on_connectionSwitch_toggled(bool checked);
    void on_copyButton_clicked();

private:
    Ui::Menu* ui;
};

#endif // MENU_H
