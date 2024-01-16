#include "display.h"
#include "calculation.h"
#include <QDebug>
#include <QPainterPath>
#include <QtMath>
#include <QRandomGenerator>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>
#include <unordered_set>
#include <QPropertyAnimation>
#include <QApplication>
#include <QClipboard>
#include <QToolButton>
#include <memory>

#include "menu.h"

namespace {
static const int g_bigPointSize = 48;
static const int g_smallPointSize = 24;
static const int g_normalHistSizePointSize = 30;
} // namespace

ElementPath::ElementPath(const QPointF& start, const QPointF& end, const QColor& color)
    : _color(color)
{
    moveTo(start);
    const double dx = end.x() - start.x();
    const double dy = end.y() - start.y();
    const double dist = qSqrt(dx * dx + dy * dy);
    const double angle = qAtan2(dy, dx);
    const double offsetX = dist * 0.15 * qCos(angle + (dx > 0 ? M_PI / 8 : -M_PI / 8));
    const double offsetY = dist * 0.15 * qSin(angle + (dx > 0 ? M_PI / 8 : -M_PI / 8));

    const QPointF c1(start.x() + offsetX, start.y() + offsetY);
    const QPointF c2(end.x() - offsetX, end.y() - offsetY);

    cubicTo(c1, c2, end);
}

class ElementDisplay : public QLabel
{
public:
    ElementDisplay(QWidget* parent) : ElementDisplay(parent, nullptr) {}
    ElementDisplay(QWidget* parent, Element* element, bool showConnection = true) : QLabel(parent)
    {
        setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        setFont(QFont("Arial", g_bigPointSize));
        setElement(element);
        setMargin(2);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);
        if (showConnections && (_previous || !_nexts.empty())) {
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
        qDebug() << _nexts;
        while (!_nexts.empty()) {
            if (_nexts.back())
                _nexts.back()->_previous = nullptr;
            _nexts.pop_back();
        }
    }

    void setPrev(ElementDisplay* prev) { _previous = prev; }
    ElementDisplay* prev() const { return _previous; }

    bool hasNext() const { return !_nexts.empty(); }

    std::vector<QPointer<ElementDisplay>>& nexts() { return _nexts; };
    static void setShowConnections(bool show) { showConnections = show; }

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
    static bool showConnections;
};
bool ElementDisplay::showConnections = true;

Display::Display(QWidget* parent)
{
    _menu = new Menu(this);
    connect(_menu->pasteButton(), &QAbstractButton::clicked, this, &Display::pasteAllResults);
    connect(_menu->connectionButton(), &QAbstractButton::toggled, this, &Display::toggleConnection);
    connect(_menu->clearButton(), &QAbstractButton::clicked, this, &Display::clearAllHistory);

    _menu->show();
    _menu->move(20, 0);
    _animation = new QPropertyAnimation(_menu, "pos", this);
    _animation->setDuration(300);
    _animation->setEasingCurve(QEasingCurve::InOutQuad);

    _menuButton = new QToolButton(this);
    QIcon menuIcon;
    menuIcon.addFile(QString::fromUtf8(":/Button/resource/menu_hamburger.png"), QSize(),
                     QIcon::Normal, QIcon::Off);
    _menuButton->setIcon(menuIcon);
    _menuButton->setStyleSheet(_menu->styleSheet());
    _menuButton->setFixedSize({34, 30});
    _menuButton->setCheckable(true);
    _menuButton->move(4, 3);
    _menuButton->setWindowFlag(Qt::WindowStaysOnTopHint);
    connect(_menuButton, &QPushButton::toggled, this, [this]() {
        if (_menuButton->isChecked())
            showMenu();
        else
            hideMenu();
    });

    _menu->raise();
    _menuButton->raise();
}

void Display::setEquations(const std::shared_ptr<EquationQueue>& equations)
{
    if (_equations.get())
        disconnect(_equations.get(), &EquationQueue::changed, this, &Display::setUpdateToTrue);
    _equations = equations;
    connect(_equations.get(), &EquationQueue::changed, this, &Display::setUpdateToTrue);
}

void Display::setUpdateToTrue()
{
    _needUpdateElements = true;
    if (_equations->size() > _elementsDisplay.size()) {
        _elementsDisplay.emplace_back();
    } else if (_equations->size() < _elementsDisplay.size()) {
        while (_equations->size() < _elementsDisplay.size()) {
            while (!_elementsDisplay.back().empty()) {
                delete _elementsDisplay.back().back();
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
    return;
}

void Display::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    if (!_needUpdateElements || !_equations) {
        if (_showConnections)
            drawPaths();
        update();
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
            if (line < _elementsDisplay.size() - 1 &&
                display->font().pointSize() > g_smallPointSize) {
                QFont font = display->font();
                font.setPointSize(g_normalHistSizePointSize);
                display->setFont(font);
            }
            display->adjustSize();
            display->show();
            lastX -= display->width() + xMargin;
            display->move(lastX, lastY - display->height());
        }
        while (lastX < 0 && line == _elementsDisplay.size() - 1 &&
               !_elementsDisplay[line].empty()) {
            QFont font = _elementsDisplay[line].back()->font();
            font.setPointSize(font.pointSize() - 2);
            if (font.pointSize() < g_smallPointSize)
                break;
            lastX = width();
            for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
                auto* display = _elementsDisplay[line][column];
                display->setFont(font);
                display->adjustSize();
                display->show();
                lastX -= display->width() + xMargin;
                display->move(lastX, lastY - display->height());
            }
        }

        if (line != _elementsDisplay.size() - 2 || _elementsDisplay.back().empty()) {
            lastX = width();
            lastY -= _elementsDisplay[line].back()->height() + yMargin;
            continue;
        }

        for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
            auto* displayFromPreviousLine = _elementsDisplay[line][column];
            displayFromPreviousLine->clearAllNext();
            auto* previousLinerElement = dynamic_cast<Number*>(displayFromPreviousLine->element());
            if (!previousLinerElement)
                continue;
            for (int latestDisplayC = _elementsDisplay.back().size() - 1; latestDisplayC >= 0;
                 --latestDisplayC) {
                auto* elementInLastLine = _elementsDisplay.back()[latestDisplayC]->element();
                if (!elementInLastLine)
                    continue;
                auto* latestDisplay = dynamic_cast<Number*>(elementInLastLine);

                if (latestDisplay && previousLinerElement->text() == latestDisplay->text()) {
                    displayFromPreviousLine->addNext(_elementsDisplay.back()[latestDisplayC]);
                }
            }
        }

        lastX = width();
        lastY -= _elementsDisplay[line].back()->height() + yMargin;
    }

    _paths.clear();
    for (int r = 0; r < _elementsDisplay.size(); ++r) {
        for (int c = 0; c < _elementsDisplay[r].size(); ++c) {
            auto* const display = _elementsDisplay[r][c];
            for (const auto& next : display->nexts()) {
                if (!next)
                    continue;
                _paths.push_back(generatePath(display, next));
            }
        }
    }

    {
        //        QPainterPath path;
        //        path.moveTo(30, 30);
        //        path.lineTo(200, 200);
        //        painter.drawPath(path);
    }
    if (_showConnections)
        drawPaths();

    QWidget::paintEvent(event);
    _needUpdateElements = false;
    _menu->raise();
    _menuButton->raise();
}

ElementPath Display::generatePath(ElementDisplay* one, ElementDisplay* other)
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
    return ElementPath(QPointF(one->pos()) + QPointF(one->width() / 2, one->height()),
                       other->pos() + QPointF(one->width() / 2, 0), color);
}

void Display::showMenu()
{
    qDebug() << __func__ << _menu->pos() << _menu->size();
    _animation->setStartValue(_menu->pos());
    _animation->setEndValue(QPoint(0, _menu->y()));
    _animation->start();
}
void Display::hideMenu()
{
    qDebug() << __func__ << _menu->pos() << _menu->size() << _menu->x();
    _animation->setStartValue(_menu->pos());
    _animation->setEndValue(QPoint(-_menu->width(), _menu->y()));
    _animation->start();
}

void Display::pasteAllResults()
{
    QString result;
    for (const auto& line : _elementsDisplay) {
        for (const auto* elementDisplay : line)
            result.append(elementDisplay->text());
        result.append('\n');
    }
    result = result.trimmed();
    if (result.size() > 0)
        QApplication::clipboard()->setText(result);
}

void Display::toggleConnection(bool show)
{
    if (_showConnections == show)
        return;
    _showConnections = show;
    ElementDisplay::setShowConnections(show);
    update();
}

void Display::clearAllHistory()
{
    _equations->clear();
    emit _equations->changed();
}

void Display::drawPaths()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const auto& path : _paths) {
        QPen pen(path._color, 1.6);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawPath(path);
    }
}
