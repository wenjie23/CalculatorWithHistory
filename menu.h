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
    QAbstractButton* pasteButton() const;
    QAbstractButton* connectionButton() const;
    QAbstractButton* clearButton() const;

private:
    Ui::Menu* ui;
};

#endif // MENU_H
