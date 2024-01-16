#include "display.h"
#include "calculation.h"
#include <QDebug>
#include <QPainterPath>
#include <QtMath>
#include <QRandomGenerator>

class ElementDisplay : public QLabel
{
public:
    ElementDisplay(QWidget* parent) : ElementDisplay(parent, nullptr) {}
    ElementDisplay(QWidget* parent, const std::shared_ptr<Element>& element)
        : QLabel(parent), _element(element)
    {
        setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        setFont(QFont("Arial", 24));
        QString textToShow;
        if (element) {
            textToShow = element->text();
            if (textToShow.size() > 1 && textToShow[0] == '-')
                textToShow = QStringLiteral("(").append(textToShow).append(QStringLiteral(")"));
        }
        setText(textToShow);
        setMargin(2);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);
        if (_connected) {
            QPainter painter(this);
            QPen pen(_connectColor.spec() == QColor::Invalid ? Qt::gray : _connectColor, 1.2);
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 2, 2);
        }
    }

    Element* element() const { return _element.get(); }
    void setConnected(bool connect) { _connected = connect; }
    bool connected() const { return _connected; }
    QColor connectColor() const { return _connectColor; }
    void setConnectColor(const QColor color) { _connectColor = color; }

private:
    std::shared_ptr<Element> _element;
    bool _connected = false;
    QColor _connectColor;
};

Display::Display(QWidget* parent) {}

void Display::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!_equations)
        return;
    //    qDebug() << __func__ << "paint";

    const int xMargin = 1;
    const int yMargin = 3;
    for (auto& line : _elementsDisplay) {
        for (auto* element : line) {
            delete element;
        }
        line.clear();
    }
    _elementsDisplay.clear();
    int lastX = width();
    int lastY = height();
    for (int equationIndex = _equations->size() - 1; equationIndex >= 0; --equationIndex) {
        _elementsDisplay.emplace_back();
        auto& elements = _equations->at(equationIndex).elements();
        if (elements.empty()) {
            auto* empty = new ElementDisplay(this);
            empty->show();
            empty->move(lastX, lastY - empty->height());
            empty->show();
            _elementsDisplay.back().push_back(empty);
        } else {
            for (int elementIndex = elements.size() - 1; elementIndex >= 0; --elementIndex) {
                auto* display = new ElementDisplay(this, elements[elementIndex]);
                display->show();
                lastX -= display->width() + xMargin;
                display->move(lastX, lastY - display->height());
                display->show();
                _elementsDisplay.back().push_back(display);
                if (dynamic_cast<Number*>(display->element()) && _elementsDisplay.size() > 1) {
                    auto& equationDisplay = _elementsDisplay[_elementsDisplay.size() - 2];
                    for (auto& previousDisplay : equationDisplay) {
                        if (dynamic_cast<Number*>(previousDisplay->element()) &&
                            previousDisplay->text() == display->text()) {
                            drowConnections(display, previousDisplay);
                        }
                    }
                }
            }
        }
        lastX = width();
        lastY -= _elementsDisplay.back().back()->height() + yMargin;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.end();
    update();
}

void Display::drowConnections(ElementDisplay* one, ElementDisplay* other)
{
    one->setConnected(true);
    other->setConnected(true);
    QColor color;
    if (one->connectColor().spec() != QColor::Invalid) {
        color = one->connectColor();
    } else {
        auto* random = QRandomGenerator::global();
        color.setHsl(144, 92, 158);
        one->setConnectColor(color);
    }
    other->setConnectColor(color);

    const auto path = calcPath(QPointF(one->pos()) + QPointF(one->width() / 2, one->height()),
                               other->pos() + QPointF(one->width() / 2, 0));

    color.setHsl(144, 92, 158);
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
