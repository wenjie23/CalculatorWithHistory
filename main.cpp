#include "mainwindow.h"

#include <QApplication>
#include "calculator.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/App/resource/app_icon.png"), QSize(), QIcon::Normal,
                 QIcon::Off);
    a.setWindowIcon(icon);
    Calculator calculator;
    calculator.show();
    return a.exec();
}
