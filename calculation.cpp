#include "calculation.h"
#include <cassert>
#include <QString>
#include <QDebug>

double Equation::calculate() const
{
    assert(_elements.size() == 4);
    double left = static_cast<Number*>(_elements[0].get())->value();
    double right = static_cast<Number*>(_elements[2].get())->value();
    QString op = static_cast<Operator*>(_elements[1].get())->text();
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
        return 0;
    }
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
    if (_elements.empty() || completed())
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

void Equation::push_back(const std::shared_ptr<Element>& element) { _elements.push_back(element); }

void Number::appendDecimal()
{
    bool ok;
    QString newText = _text + QString(".");
    auto parsed = newText.toDouble(&ok);
    if (ok)
        _text = newText;
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
}

void EquationQueue::appendDicimal()
{
    if (empty() || back().completed()) {
        emplace_back();
    }
    back().appendDecimal();
}

void EquationQueue::append(const QString& op)
{
    if (empty())
        return;
    if (back().completed()) {
        Equation equation;
        equation.push_back(back().elements().back());
        equation.append(op);
        push_back((equation));
        return;
    }
    back().append(op);
    popFrontIfExceedLimit();
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
