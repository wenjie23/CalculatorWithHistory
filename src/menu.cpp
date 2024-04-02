#include "menu.h"
#include "ui_menu.h"
#include <QGraphicsBlurEffect>
#include <QPainter>

namespace {
constexpr QSize g_menuSize(43, 142);
const QString g_copiedText = "Copied!";
}

Menu::Menu(QWidget* parent) : QFrame(parent), ui(new Ui::Menu)
{
    ui->setupUi(this);
    setFixedSize(g_menuSize);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/Button/correct.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->copyButton->setClickedIcon(icon);
    ui->copyButton->bubbleWidget()->setBoundary(parent);
    ui->copyButton->bubbleWidget()->setText(g_copiedText);
}

Menu::~Menu() { delete ui; }

void Menu::on_clearButton_clicked() { emit clearButtonClicked(); }

void Menu::on_connectionSwitch_toggled(bool checked) { emit connectionButtonToggled(checked); }

void Menu::on_copyButton_clicked() { emit copyButtonClicked(); }
