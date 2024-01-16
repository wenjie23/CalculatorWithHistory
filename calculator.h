#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QWidget>
#include "calculation.h"

class QLCDNumber;
class QPushButton;
class QGridLayout;
class Equation;
class Display;

class Calculator : public QWidget
{
    Q_OBJECT
public:
    Calculator(QWidget* parent = nullptr);

    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void digitClicked(int digit);
    void unaryOperatorClicked(const QString& op);
    void binaryOperatorClicked(const QString& op);
    void dotClicked();
    void equalClicked();
    void enterClicked();

    void clear();

private:
    void updateDisplay();

    Display* _display;
    QPushButton* _digitButtons[10];
    QPushButton* _unaryOperatorButtons[2];
    QPushButton* _binaryOperatorButtons[4];
    QPushButton* _dotButton;
    QPushButton* _equalButton;
    QPushButton* _clearButton;
    QGridLayout* _layout;

    double _value;
    double _operand;
    QString _operator;

    std::shared_ptr<EquationQueue> _equationQueue;
};

#endif // CALCULATOR_H
