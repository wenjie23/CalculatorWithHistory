#include "calculation.h"
#include <cassert>
#include <QString>
#include <QDebug>
#include <deque>
#include <stdexcept>
namespace {
double calculateImpl(double left, double right, const QString& op)
{
    if (op == QString("+")) {
        return left + right;
    } else if (op == QString("-")) {
        return left - right;
    } else if (op == QString("*")) {
        return left * right;
    } else if (op == QString("/")) {
        return left / right;
    } else if (op == QString("%")) {
    } else {
        throw std::invalid_argument("Invalid arguments");
    }
}
} // namespace
double Equation::calculate() const
{
    if (_elements.size() < 4 || _elements.back()->text() != "=")
        throw std::invalid_argument("Invalid arguments");
    std::deque<std::shared_ptr<Element>> calcuationStack;
    calcuationStack.push_back(_elements[0]);
    for (int i = 1; i < _elements.size() - 1; ++i) {
        if (calcuationStack.empty() || dynamic_cast<Operator*>(_elements[i].get())) {
            calcuationStack.push_back(_elements[i]);
            continue;
        }
        if (!calcuationStack.empty() && dynamic_cast<Operator*>(calcuationStack.back().get())) {
            if (calcuationStack.back()->text() == "*" || calcuationStack.back()->text() == "/") {
                const QString op = calcuationStack.back()->text();
                calcuationStack.pop_back();
                double value = calcuationStack.back()->text().toDouble();
                calcuationStack.pop_back();
                calcuationStack.push_back(std::make_shared<Number>(
                    calculateImpl(value, static_cast<Number*>(_elements[i].get())->value(), op)));
                continue;
            }
        }
        calcuationStack.push_back(_elements[i]);
    }

    double resultSoFar = calcuationStack.front()->text().toDouble();
    calcuationStack.pop_front();
    while (!calcuationStack.empty()) {
        QString op = calcuationStack.front()->text();
        calcuationStack.pop_front();
        double right = calcuationStack.front()->text().toDouble();
        calcuationStack.pop_front();
        resultSoFar = calculateImpl(resultSoFar, right, op);
    }
    return resultSoFar;
}

void Equation::append(int digit)
{
    if (_elements.empty() || !dynamic_cast<Number*>(_elements.back().get())) {
        _elements.push_back(std::make_shared<Number>(digit));

    } else if (auto* casted = dynamic_cast<Number*>(_elements.back().get())) {
        casted->appendDigit(digit);
    }
}

void Equation::append(const QString& op)
{
    if (_elements.empty() || completed() || dynamic_cast<Operator*>(_elements.back().get()))
        return;
    if (_elements.size() < 3 && op == QString("="))
        return;
    _elements.push_back(std::make_shared<Operator>(op));
    if (op == QString("=")) {
        const double result = calculate();
        _elements.push_back(std::make_shared<Number>(result));
        _completed = true;
    }
}

void Equation::appendDecimal()
{
    if (completed())
        return;
    if (_elements.empty() || !dynamic_cast<Number*>(_elements.back().get())) {
        auto number = std::make_shared<Number>(0);
        number->appendDecimal();
        _elements.push_back(number);
    } else if (dynamic_cast<Number*>(_elements.back().get())) {
        dynamic_cast<Number*>(_elements.back().get())->appendDecimal();
    }
}

QString Equation::text() const
{
    QString result;
    for (const auto& element : _elements) {
        result.append(element->text());
    }
    return result;
}

bool Equation::empty() const
{
    return elements().empty() || _elements[0]->text() == QStringLiteral(" ") ||
           _elements[0]->text() == QStringLiteral("");
}

void Equation::pop()
{
    if (empty())
        return;
    if (dynamic_cast<Operator*>(_elements.back().get())) {
        _elements.pop_back();
        return;
    }
    auto* number = static_cast<Number*>(_elements.back().get());
    QString numberText = number->text();
    if (numberText.isEmpty()) {
        _elements.pop_back();
        return pop();
    }
    numberText.resize(numberText.size() - 1);
    const bool successful = number->trySetValue(numberText);
    if (!successful)
        _elements.pop_back();
}

void Equation::push_back(const std::shared_ptr<Element>& element) { _elements.push_back(element); }

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
    QString newText = _text + QString(".");
    auto parsed = newText.toDouble(&ok);
    if (ok) {
        _text = newText;
        emit changed();
    }
}

QString Number::text() const { return _text; }

Operator::Operator(const QString& op) : Element(), _op(op) {}

QString Operator::text() const { return _op; }

void EquationQueue::append(int digit)
{
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
    emit changed();
}

void EquationQueue::append(const QString& op)
{
    if (empty())
        return;
    if (back().completed()) {
        Equation equation;
        auto newElement = std::make_shared<Number>();
        newElement->setValue(static_cast<Number*>(back().elements().back().get())->value());
        equation.push_back(newElement);
        equation.append(op);
        push_back((equation));
        emit changed();
        return;
    }
    back().append(op);
    popFrontIfExceedLimit();
    emit changed();
}

void EquationQueue::popLastCharacter()
{
    if (empty() || back().completed() || back().empty())
        return;
    back().pop();
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
