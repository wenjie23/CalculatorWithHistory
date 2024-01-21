#include "menu.h"
#include "ui_menu.h"
#include <QGraphicsBlurEffect>
#include <QPainter>

namespace {
static const QSize g_menuSize(43, 142);
}

Menu::Menu(QWidget* parent) : QFrame(parent), ui(new Ui::Menu)
{
    ui->setupUi(this);
    setFixedSize(g_menuSize);
}

Menu::~Menu() { delete ui; }

void Menu::on_clearButton_clicked()
{
    emit clearButtonClicked();
}

void Menu::on_connectionSwitch_toggled(bool checked)
{
    emit connectionButtonToggled(checked);
}

void Menu::on_pasteButton_clicked()
{
    emit pasteButtonClicked();
}

