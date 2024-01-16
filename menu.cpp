#include "menu.h"
#include "ui_menu.h"
#include <QGraphicsBlurEffect>
#include <QPainter>

Menu::Menu(QWidget* parent) : QFrame(parent), ui(new Ui::Menu)
{
    ui->setupUi(this);
    //    ui->pasteButton->setFixedSize(30, 34);
    //    ui->connectionSwitch->setFixedSize(30, 34);
    //    ui->clearButton->setFixedSize(30, 34);
    setFixedSize(43, 142);
}

Menu::~Menu() { delete ui; }

QAbstractButton* Menu::pasteButton() const { return ui->pasteButton; }

QAbstractButton* Menu::connectionButton() const { return ui->connectionSwitch; }

QAbstractButton* Menu::clearButton() const { return ui->clearButton; }
