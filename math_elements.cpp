
#include <deque>
#include <stdexcept>

#include <QString>
#include <QDebug>
#include <iostream>

#include "math_elements.h"

extern const QString g_plus("+");
extern const QString g_minus("-");
extern const QString g_multiply("*");
extern const QString g_divide("÷");

namespace {
double calculateImpl(double left, double right, const QString& op)
{
    if (op == g_plus) {
        return left + right;
    } else if (op == g_minus) {
        return left - right;
    } else if (op == g_multiply) {
        return left * right;
    } else if (op == g_divide) {
        return left / right;
    } else {
        throw std::invalid_argument("Invalid arguments");
        return -1;
    }
}
} // namespace
double Equation::calculate() const
{
    if (size() < 4)
        throw std::invalid_argument("Invalid");
    std::deque<std::shared_ptr<Element>> calcuationStack;
    calcuationStack.push_back((*this)[0]);
    for (int i = 1; i < size() - (_completed ? 3 : 1); ++i) {
        if (calcuationStack.empty() || dynamic_cast<Operator*>((*this)[i].get())) {
            calcuationStack.push_back((*this)[i]);
            continue;
        }
        if (!calcuationStack.empty() && dynamic_cast<Operator*>(calcuationStack.back().get())) {
            if (calcuationStack.back()->text() == "*" || calcuationStack.back()->text() == "/") {
                const QString op = calcuationStack.back()->text();
                calcuationStack.pop_back();
                double value = calcuationStack.back()->text().toDouble();
                calcuationStack.pop_back();
                calcuationStack.push_back(std::make_shared<Number>(
                    calculateImpl(value, static_cast<Number*>((*this)[i].get())->value(), op)));
                continue;
            }
        }
        calcuationStack.push_back((*this)[i]);
    }

    double resultSoFar = calcuationStack.front()->text().toDouble();
    calcuationStack.pop_front();
    while (!calcuationStack.empty()) {
        QString op = calcuationStack.front()->text();
        calcuationStack.pop_front();
        double right = calcuationStack.front()->text().toDouble();
        calcuationStack.pop_front();
        resultSoFar = calculateImpl(resultSoFar, right, op);
        std::cout << resultSoFar << std::endl;
    }
    return resultSoFar;
}

void Equation::append(uint8_t digit)
{
    if (completed())
        return;
    if (empty() || !dynamic_cast<Number*>(back().get())) {
        push_back(std::make_shared<Number>(digit));
    } else if (auto* casted = dynamic_cast<Number*>(back().get())) {
        casted->appendDigit(digit);
    }
}

void Equation::append(const QString& op)
{
    if (empty() || completed() || dynamic_cast<Operator*>(back().get()))
        return;
    if (size() < 3 && op == QString("="))
        return;
    push_back(std::make_shared<Operator>(op));
    if (op == QString("=")) {
        const double result = calculate();
        push_back(std::make_shared<Number>(result));
        _completed = true;
    }
}

void Equation::appendDecimal()
{
    if (completed())
        return;
    if (empty() || !dynamic_cast<Number*>(back().get())) {
        auto number = std::make_shared<Number>(0);
        number->appendDecimal();
        push_back(number);
    } else if (dynamic_cast<Number*>(back().get())) {
        dynamic_cast<Number*>(back().get())->appendDecimal();
    }
}

QString Equation::text() const
{
    QString result;
    for (const auto& element : *this) {
        result.append(element->text());
    }
    return result;
}

bool Equation::tryPopCharacter()
{
    if (empty())
        return true;
    if (dynamic_cast<Operator*>(back().get())) {
        pop_back();
        return true;
    }
    auto* const number = static_cast<Number*>(back().get());
    QString numberText = number->text();
    if (numberText.isEmpty()) {
        pop_back();
        return tryPopCharacter();
    }
    numberText.resize(numberText.size() - 1);
    const bool successful = number->trySetValue(numberText);
    if (!successful) {
        pop_back();
        return true;
    }
}

bool Number::trySetValue(const QString& s)
{
    bool ok;
    s.toDouble(&ok);
    if (!ok)
        return false;
    _text = s;
    emit changed();
    return true;
}

void Number::appendDecimal()
{
    bool ok;
    const QString newText = _text + QChar('.');
    auto parsed = newText.toDouble(&ok);
    if (ok) {
        _text = newText;
        emit changed();
    }
}

Operator::Operator(const QString& op) : Element(op) {}

void EquationQueue::append(uint8_t digit)
{
    assert(digit >=0 && digit <= 9);
    if (empty() || back().completed()) {
        Equation equation;
        equation.append(digit);
        push_back(equation);
    } else {
        back().append(digit);
    }
    popFrontIfExceedLimit();
    emit changed();
}

void EquationQueue::appendDicimal()
{
    if (empty() || back().completed()) {
        emplace_back();
    }
    back().appendDecimal();
    popFrontIfExceedLimit();
    emit changed();
}

void EquationQueue::append(const QString& op)
{
    if (empty())
        return;
    if (back().completed()) {
        Equation equation;
        equation.push_back(std::make_shared<Number>(static_cast<Number*>(back().back().get())->value()));
        equation.append(op);
        push_back((equation));
        popFrontIfExceedLimit();
        emit changed();
        return;
    }
    back().append(op);
    popFrontIfExceedLimit();
    emit changed();
}

void EquationQueue::tryPopLastCharacter()
{
    if (empty() || back().completed() || back().empty())
        return;
    back().tryPopCharacter();
    emit changed();
}

QString EquationQueue::text() const
{
    QString result;
    for (const auto& e : *this) {
        result.append(e.text());
        if (e.completed()) {
            result.append('\n');
        }
    }
    return result;
}
