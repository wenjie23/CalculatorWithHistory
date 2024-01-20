#ifndef MATH_ELEMENTS_H
#define MATH_ELEMENTS_H

#include <QObject>
#include <QString>
#include <deque>
#include <memory>

class Element : public QObject
{
    Q_OBJECT
public:
    virtual ~Element() = default;
    virtual QString text() const { return _text; }

signals:
    void changed();

protected:
    Element() = default;
    explicit Element(const QString& text): _text(text) {}
    QString _text;
};

// this implementation will loss precision!
class Number : public Element
{
public:
    explicit Number(double v) : Element(QString::number(v, 'g', 15)) {}
    ~Number() = default;

    double value() const { return _text.toDouble(); }
    void setValue(double v)
    {
        auto text = QString::number(v, 'g', 15);
        if (_text == text)
            return;
        _text = text;
        emit changed();
    }
    bool trySetValue(const QString& s);
    void appendDigit(uint8_t digit)
    {
        assert(digit >= 0 && digit <= 9);
        _text += QString::number(digit, 'g', 15);
        emit changed();
    }
    void appendDecimal();
    friend bool operator==(const Number& a, const Number& b)
    {
        return a._text.toDouble() == b._text.toDouble();
    }
};

class Operator : public Element
{
public:
    explicit Operator(const QString& op);
    ~Operator() = default;
};

class Equation : public std::vector<std::shared_ptr<Element>>
{
public:
    Equation() = default;
    double calculate() const;
    QString text() const;
    bool completed() const { return _completed; }

    void append(uint8_t digit);
    void append(const QString& op);
    void appendDecimal();
    bool tryPopCharacter();

private:
    bool _completed = false;
};

class EquationQueue : public QObject, public std::deque<Equation>
{
    Q_OBJECT
public:
    explicit EquationQueue(size_t sizeLimit = 32) : std::deque<Equation>(), _sizeLimit(sizeLimit){};

    QString text() const;

    void append(uint8_t digit);
    void appendDicimal();
    void append(const QString& op);
    void tryPopLastCharacter();

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
#endif // MATH_ELEMENTS_H
