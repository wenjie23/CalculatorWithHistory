#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QWidget>
#include "math_elements.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void digitClicked(uint8_t digit);
    void unaryOperatorClicked(const QString& op);
    void binaryOperatorClicked(const QString &op);
    void periodClicked();
    void equalClicked();
    void enterClicked();
    void clear();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<EquationQueue> _equationQueue;
};

#endif // MAIN_WINDOW_H
