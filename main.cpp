#include "mainwindow.h"

#include <QApplication>
#include "calculator.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Calculator calculator;
    calculator.show();
    return a.exec();
}
