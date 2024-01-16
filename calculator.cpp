#include "calculator.h"
#include <QLCDNumber>
#include <QPushButton>
#include <QGridLayout>
#include <QKeyEvent>
#include <QtMath>
#include <array>
#include <QDebug>
#include "calculation.h"
#include "display.h"

Calculator::Calculator(QWidget* parent) : QWidget(parent)
{
    _display = new Display;
    _display->setFixedWidth(400);
    _equationQueue = std::make_shared<EquationQueue>();
    _display->setEquations(_equationQueue);

    _layout = new QGridLayout;
    _layout->addWidget(_display, 0, 0, 5, 1);
    _layout->setSpacing(0);
    _layout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < 10; ++i) {
        _digitButtons[i] = new QPushButton(QString::number(i));
        _digitButtons[i]->setShortcut(Qt::Key_0 + i);
        connect(_digitButtons[i], &QPushButton::clicked, [this, i] { digitClicked((i)); });
    }

    _unaryOperatorButtons[0] = new QPushButton("+/-");
    connect(_unaryOperatorButtons[0], &QPushButton::clicked,
            [this]() { unaryOperatorClicked(QStringLiteral("+/-")); });

    _unaryOperatorButtons[1] = new QPushButton("%");
    connect(_unaryOperatorButtons[1], &QPushButton::clicked,
            [this]() { unaryOperatorClicked(QStringLiteral("%")); });

    const std::array<QString, 4> binaryOps{
        {QString("+"), QString("-"), QString("*"), QString("/")}};
    const std::array<Qt::Key, 4> keys{
        {Qt::Key_Plus, Qt::Key_Minus, Qt::Key_Asterisk, Qt::Key_Slash}};
    for (int i = 0; i < 4; ++i) {
        _binaryOperatorButtons[i] = new QPushButton(binaryOps[i]);
        _binaryOperatorButtons[i]->setShortcut(keys[i]);
        connect(_binaryOperatorButtons[i], &QPushButton::clicked,
                [this, op = binaryOps[i]]() { binaryOperatorClicked(op); });
    }

    _dotButton = new QPushButton(".");
    _dotButton->setShortcut(Qt::Key_Period);
    connect(_dotButton, &QPushButton::clicked, this, &Calculator::dotClicked);

    _equalButton = new QPushButton("=");
    _equalButton->setShortcut(Qt::Key_Equal);
    connect(_equalButton, &QPushButton::clicked, this, &Calculator::equalClicked);

    _clearButton = new QPushButton("C");
    connect(_clearButton, &QPushButton::clicked, this, &Calculator::clear);

    _layout->addWidget(_clearButton, 0, 1);
    for (int i = 0; i < 2; ++i) {
        _layout->addWidget(_unaryOperatorButtons[i], 0, i + 2);
    }
    for (int i = 0; i < 4; ++i) {
        _layout->addWidget(_binaryOperatorButtons[i], i, 4);
    }

    for (int i = 1; i < 10; ++i) {
        const int row = 4 - qCeil(i / 3.0f);
        const int column = (i - 1) % 3 + 1;
        _layout->addWidget(_digitButtons[i], row, column);
    }
    _layout->addWidget(_digitButtons[0], 4, 1, 1, 2);
    _layout->addWidget(_dotButton, 4, 3);
    _layout->addWidget(_equalButton, 4, 4, 1, 1);

    setLayout(_layout);

    setWindowTitle("Calculator with History");
    setFixedSize(640, 300);
    const auto allPButtons = findChildren<QPushButton*>();
    for (auto* button : allPButtons) {
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFont(QFont("Arial", 25));
    }
}

void Calculator::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter) {
        enterClicked();
    } else if (event->key() == Qt::Key_Backspace) {
        _equationQueue->popLastCharacter();
    }
    qDebug() << event->key();
    QWidget::keyPressEvent(event);
}

void Calculator::digitClicked(int digit)
{
    _equationQueue->append(digit);
    updateDisplay();
}

void Calculator::unaryOperatorClicked(const QString& op)
{
    if (_equationQueue->empty() || _equationQueue->back().empty() ||
        !dynamic_cast<Number*>(_equationQueue->back().elements().back().get()))
        return;
    auto* const number = static_cast<Number*>(_equationQueue->back().elements().back().get());
    const auto numberText = number->text();
    if (op == QStringLiteral("+/-")) {
        if (numberText[0] == '-') {
            number->trySetValue(numberText.right(numberText.size() - 1));
        } else {
            number->trySetValue(QStringLiteral("-").append(numberText));
        }
    } else if (op == QStringLiteral("%")) {
        number->trySetValue(QString::number(numberText.toDouble() / 100));
    }
}

void Calculator::binaryOperatorClicked(const QString& op)
{
    if (!_operator.isEmpty()) {
        _value = calculate(_operand, _value, _operator);
    }
    updateDisplay();
    _operand = _value;
    _operator = op;
    _value = 0;

    _equationQueue->append(op);
    updateDisplay();
}

void Calculator::dotClicked()
{
    qDebug() << __func__;
    _equationQueue->appendDicimal();
}

void Calculator::equalClicked()
{
    if (!_operator.isEmpty()) {
        _value = calculate(_operand, _value, _operator);
        updateDisplay();
    }
    _operator.clear();
    _operand = 0;

    _equationQueue->append(QString('='));
    updateDisplay();
}

void Calculator::enterClicked()
{
    if (_equationQueue->empty())
        return;
    if (_equationQueue->back().completed())
        _clearButton->animateClick();
    else
        _equalButton->animateClick();
}

void Calculator::clear()
{
    if (_equationQueue->empty())
        return;
    if (_equationQueue->back().elements().empty())
        _equationQueue->clear();
    else if (_equationQueue->back().completed()) {
        _equationQueue->emplace_back();
    } else if (_equationQueue->back().elements().empty()) {
        _equationQueue->clear();
    } else {
        _equationQueue->back().elements().clear();
    }
    updateDisplay();
}

double Calculator::calculate(double left, double right, const QString& op)
{
    if (op == QString("+")) {
        return left + right;
    } else if (op == QString("-")) {
        return left - right;
    } else if (op == QString("*")) {
        return left * right;
    } else if (op == QString("/")) {
        return left / right;
    } else {
        return 0;
    }
}

void Calculator::updateDisplay() { _display->update(); }
