#include <QApplication>
#include <QIcon>
#include "main_window.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QIcon icon;
    icon.addFile(QStringLiteral(":/App/app_icon.png"));
    a.setWindowIcon(icon);
    MainWindow mainWindow;
    mainWindow.show();
    return a.exec();
}
