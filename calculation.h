#ifndef CALCULATION_H
#define CALCULATION_H

#include <QObject>
#include <QString>
#include <vector>
#include <deque>

class Element : public QObject
{
    Q_OBJECT
public:
    Element() = default;
    virtual ~Element() = default;
    virtual QString text() const = 0;

signals:
    void changed();
};

class Number : public Element
{
public:
    Number() : Element(){};
    Number(double v) : Element() { _text = QString::number(v); }
    ~Number() = default;
    double value() const { return _text.toDouble(); }
    void setValue(double v)
    {
        _text = QString::number(v);
        emit changed();
    }
    bool trySetValue(const QString& s);
    void appendDigit(const int digit)
    {
        _text += QString::number(digit);
        emit changed();
    }
    void appendDecimal();
    QString text() const override;

private:
    QString _text;
};

class Operator : public Element
{
public:
    Operator(const QString& op);
    ~Operator() = default;
    QString op() const { return _op; }
    QString text() const override;

private:
    QString _op;
};

class Equation
{
public:
    Equation() = default;
    double calculate() const;
    void append(int digit);
    void append(const QString& op);
    void appendDecimal();
    QString text() const;
    bool empty() const;
    void pop();
    bool completed() const { return _completed; }
    void push_back(const std::shared_ptr<Element>& element);
    std::vector<std::shared_ptr<Element>>& elements() { return _elements; }
    const std::vector<std::shared_ptr<Element>>& elements() const { return _elements; };

private:
    std::vector<std::shared_ptr<Element>> _elements;
    bool _completed = false;
};

class EquationQueue : public QObject, public std::deque<Equation>
{
    Q_OBJECT
public:
    EquationQueue(size_t sizeLimit = 999) : std::deque<Equation>(), _sizeLimit(sizeLimit){};

    void append(int digit);
    void appendDicimal();
    void append(const QString& op);
    void popLastCharacter();

    QString text() const;

signals:
    void changed();

private:
    void popFrontIfExceedLimit()
    {
        while (size() > _sizeLimit)
            pop_front();
    }

    size_t _sizeLimit;
};
#endif // CALCULATION_H
