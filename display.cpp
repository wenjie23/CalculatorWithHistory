#include "display.h"
#include "calculation.h"
#include <QDebug>
#include <QPainterPath>
#include <QtMath>
#include <QRandomGenerator>
#include <QPointer>
#include <algorithm>
#include <unordered_set>
#include <memory>

class ElementDisplay : public QLabel
{
public:
    ElementDisplay(QWidget* parent) : ElementDisplay(parent, nullptr) {}
    ElementDisplay(QWidget* parent, Element* element) : QLabel(parent)
    {
        setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        setFont(QFont("Arial", 24));
        setElement(element);
        setMargin(2);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);
        if (_previous || !_nexts.empty()) {
            QPainter painter(this);
            QPen pen(_connectColor.spec() == QColor::Invalid ? Qt::gray : _connectColor, 1.2);
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 2, 2);
        }
    }

    Element* element() const { return _element; }
    void setElement(Element* e)
    {
        if (_element == e)
            return;
        connect(e, &Element::changed, this, [this, e]() { updateElementText(); });
        _element = e;
        updateElementText();
    }
    QColor connectColor() const { return _connectColor; }
    void setConnectColor(const QColor color) { _connectColor = color; }
    void addNext(ElementDisplay* display)
    {
        if (!display)
            return;
        _nexts.push_back(display);
        display->_previous = this;
    }
    void clearAllNext()
    {
        while (!_nexts.empty()) {
            _nexts.back()->_previous = nullptr;
            _nexts.pop_back();
        }
    }

    void setPrev(ElementDisplay* prev) { _previous = prev; }
    ElementDisplay* prev() const { return _previous; }

    bool hasNext() const { return !_nexts.empty(); }

    std::vector<QPointer<ElementDisplay>>& nexts() { return _nexts; };

private:
    void updateElementText()
    {
        if (!element()) {
            setText("");
        }
        auto textToShow = _element->text();
        if (text() == textToShow)
            return;
        if (textToShow.size() > 1 && textToShow[0] == '-')
            textToShow = QStringLiteral("(").append(textToShow).append(QStringLiteral(")"));
        setText(textToShow);
    }

    QPointer<Element> _element = nullptr;
    std::vector<QPointer<ElementDisplay>> _nexts;
    QPointer<ElementDisplay> _previous = nullptr;
    QColor _connectColor;
};

Display::Display(QWidget* parent) {}

void Display::setEquations(const std::shared_ptr<EquationQueue>& equations)
{
    if (_equations.get())
        disconnect(_equations.get(), &EquationQueue::changed, this, &Display::setUpdateToTrue);
    _equations = equations;
    connect(_equations.get(), &EquationQueue::changed, this, &Display::setUpdateToTrue);
}

void Display::setUpdateToTrue()
{
    if (_equations->size() > _elementsDisplay.size()) {
        _elementsDisplay.emplace_back();
    } else if (_equations->size() < _elementsDisplay.size()) {
        while (_equations->size() < _elementsDisplay.size()) {
            while (!_elementsDisplay.back().empty()) {
                _elementsDisplay.back().back()->deleteLater();
                _elementsDisplay.back().pop_back();
            }
            _elementsDisplay.pop_back();
        }
    }
    if (_equations->empty() && _elementsDisplay.empty())
        return;

    const auto& equation = (*_equations).back();
    auto& lineOfDisplay = _elementsDisplay.back();
    while (equation.elements().size() < lineOfDisplay.size()) {
        lineOfDisplay.back()->deleteLater();
        lineOfDisplay.pop_back();
    }
    if (equation.elements().empty()) {
        lineOfDisplay.push_back(new ElementDisplay(this));
        return;
    }
    for (int i = std::max(int(equation.elements().size()) - 2, 0); i < equation.elements().size();
         ++i) {
        if (i >= lineOfDisplay.size()) {
            lineOfDisplay.push_back(new ElementDisplay(this, equation.elements()[i].get()));
        } else {
            lineOfDisplay[i]->setElement(equation.elements()[i].get());
        }
    }
    _needUpdateElements = true;
    return;
}

void Display::paintEvent(QPaintEvent* event)
{
    qDebug() << _elementsDisplay.size();
    if (!_needUpdateElements || !_equations) {
        QWidget::paintEvent(event);
        return;
    }

    const int xMargin = 1;
    const int yMargin = 3;

    int lastX = width();
    int lastY = height();

    qDebug() << "line:" << _elementsDisplay.size();
    if (!_elementsDisplay.empty()) {
        qDebug() << "equation size" << _elementsDisplay.back().size();
    }
    for (int line = _elementsDisplay.size() - 1; line >= 0; --line) {
        for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
            auto* display = _elementsDisplay[line][column];
            display->show();
            display->adjustSize();
            lastX -= display->width() + xMargin;
            display->move(lastX, lastY - display->height());
        }

        if (line == _elementsDisplay.size() - 2 && !_elementsDisplay.back().empty()) {
            for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
                auto& displayFromPreviousLine = _elementsDisplay[line][column];
                displayFromPreviousLine->clearAllNext();
                if (_elementsDisplay.back().empty())
                    continue;

                for (int latestDisplayC = _elementsDisplay.back().size() - 1; latestDisplayC >= 0;
                     --latestDisplayC) {
                    auto* dispaly = dynamic_cast<Number*>(displayFromPreviousLine->element());
                    if (!_elementsDisplay.back()[latestDisplayC]->element())
                        continue;
                    auto* latestDisplay =
                        dynamic_cast<Number*>(_elementsDisplay.back()[latestDisplayC]->element());

                    if (dispaly && latestDisplay && dispaly->text() == latestDisplay->text()) {
                        displayFromPreviousLine->addNext(_elementsDisplay.back()[latestDisplayC]);
                    }
                }
            }
        }

        lastX = width();
        lastY -= _elementsDisplay.back().back()->height() + yMargin;
    }

    for (int r = 0; r < _elementsDisplay.size(); ++r) {
        for (int c = 0; c < _elementsDisplay[r].size(); ++c) {
            auto* const display = _elementsDisplay[r][c];
            for (const auto& next : display->nexts()) {
                if (!next)
                    continue;
                drowConnections(display, next);
            }
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.end();
    update();
    QWidget::paintEvent(event);
}

void Display::drowConnections(ElementDisplay* one, ElementDisplay* other)
{
    QColor color;
    if (one->connectColor().spec() != QColor::Invalid) {
        color = one->connectColor();
    } else {
        auto* random = QRandomGenerator::global();
        color.setHsl(QRandomGenerator::global()->bounded(361), 92, 158);
        one->setConnectColor(color);
    }
    other->setConnectColor(color);

    const auto path = calcPath(QPointF(one->pos()) + QPointF(one->width() / 2, one->height()),
                               other->pos() + QPointF(one->width() / 2, 0));

    //    color.setHsl(144, 92, 158);
    QPainter painter(this);
    QPen pen(color, 1.6);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPath(path);
}

QPainterPath Display::calcPath(const QPointF& start, const QPointF& end)
{
    QPainterPath path;
    path.moveTo(start);
    const double dx = end.x() - start.x();
    const double dy = end.y() - start.y();
    const double dist = qSqrt(dx * dx + dy * dy);
    const double angle = qAtan2(dy, dx);
    const double offset = dist * 0.15;

    QPointF c1(start.x() + offset * qCos(angle + M_PI / 8),
               start.y() - offset * qSin(angle + M_PI / 8));

    QPointF c2(end.x() - offset * qCos(angle + M_PI / 8),
               end.y() + offset * qSin(angle + M_PI / 8));

    path.cubicTo(c1, c2, end);
    return path;
}
