#include <memory>
#include <algorithm>

#include <QDebug>
#include <QPainterPath>
#include <QtMath>
#include <QRandomGenerator>
#include <QPointer>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QApplication>
#include <QClipboard>
#include <QToolButton>
#include <QStringBuilder>
#include <QPalette>

#include "display.h"
#include "math_elements.h"
#include "menu.h"

namespace {
static const int g_bigPointSize = 48;
static const int g_smallPointSize = 20;
static const int g_normalHistSizePointSize = 30;
static const QString g_fontFamily("Arial");

static const int g_animationDuration = 300;

static const int g_hChannelUpperBount = 361;
static const int g_sChannel = 92;
static const int g_lChannel = 158;
static const double g_connectionLineWidth = 1.6;

static const QSize g_menuButtonSize(34, 30);
static const QPoint g_menuButtonPos(4, 3);
static const QString g_menuButtonFileName(":/Button/resource/menu_hamburger.png");

} // namespace
class ElementDisplay : public QLabel
{
    friend Display;
public:
    ElementDisplay(QWidget* parent) : ElementDisplay(parent, nullptr) {}
    ElementDisplay(QWidget* parent, Element* element, bool showConnection = true) : QLabel(parent)
    {
        setElement(element);
        setMargin(2);

        setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        QFont font(g_fontFamily, g_bigPointSize);
        font.setStyleStrategy(QFont::PreferAntialias);
        setFont(font);

        QPalette palette = this->palette();
        palette.setColor(QPalette::WindowText, QColor(38, 39, 42));
        setPalette(palette);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QLabel::paintEvent(event);
        if (showConnections && (!_nexts.empty() || _previous )) {
            QPainter painter(this);
            QPen pen(_connectColor.spec() == QColor::Invalid ? Qt::gray : _connectColor, g_connectionLineWidth);
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
        disconnect(e, &Element::changed, this, &ElementDisplay::updateElementText);
        connect(e, &Element::changed, this, &ElementDisplay::updateElementText);
        _element = e;
        updateElementText();
    }
    QColor connectColor() const { return _connectColor; }
    void setConnectColor(const QColor color) { _connectColor = color; }

    const std::vector<QPointer<ElementDisplay>>& nexts() { return _nexts; };
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
            if (_nexts.back())
                _nexts.back()->_previous = nullptr;
            _nexts.pop_back();
        }
    }

private slots:
    void updateElementText()
    {
        if (!_element) {
            setText("");
        }
        auto textToShow = _element->text();
        if (text() == textToShow)
            return;
        if (textToShow.size() > 1 && textToShow[0] == QChar('-'))
            textToShow = QStringLiteral("(") % textToShow % QStringLiteral(")");
        setText(textToShow);
    }

private:
    QPointer<Element> _element;
    std::vector<QPointer<ElementDisplay>> _nexts;
    QPointer<ElementDisplay> _previous;
    QColor _connectColor;
    static bool showConnections;
};
bool ElementDisplay::showConnections = true;

ElementPath::ElementPath(const QPointF& start, const QPointF& end, const QColor& color)
    : color(color)
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

Display::Display(QWidget* parent) : QWidget(parent)
{
    _menu = new Menu(this);
    connect(_menu, &Menu::pasteButtonClicked, this, &Display::pasteAllResults);
    connect(_menu, &Menu::connectionButtonToggled, this, &Display::toggleConnection);
    connect(_menu, &Menu::clearButtonClicked, this, &Display::clearAllHistory);

    _animation = new QPropertyAnimation(_menu, "pos", this);
    _animation->setDuration(g_animationDuration);
    _animation->setEasingCurve(QEasingCurve::InOutQuad);

    _menuButton = new QToolButton(this);
    _menuButton->setIcon(QIcon(g_menuButtonFileName));
    _menuButton->setStyleSheet(_menu->styleSheet());
    _menuButton->setFixedSize(g_menuButtonSize);
    _menuButton->move(g_menuButtonPos);
    _menuButton->setCheckable(true);
    connect(_menuButton, &QPushButton::toggled, this, &Display::toggleMenu);

    toggleMenu(false);
    _menu->raise();
    _menuButton->raise();
}

void Display::setEquations(const std::shared_ptr<EquationQueue>& equations)
{
    if (_equations == equations)
        return;
    if (_equations.get())
        disconnect(_equations.get(), &EquationQueue::changed, this,
                   &Display::alignElementDisplayContent);
    _equations = equations;
    connect(_equations.get(), &EquationQueue::changed, this, &Display::alignElementDisplayContent);
}

void Display::alignElementDisplayContent()
{
    if (_equations->size() > _elementsDisplay.size()) {
        _elementsDisplay.emplace_back();
    }
    assert(_equations->size() <= _elementsDisplay.size());
    while (_equations->size() < _elementsDisplay.size()) {
        while (!_elementsDisplay.back().empty()) {
            delete _elementsDisplay.back().back();
            _elementsDisplay.back().pop_back();
        }
        _elementsDisplay.pop_back();
    }

    if (_equations->empty() && _elementsDisplay.empty()){
        adjustElementsDisplayGeo();
        repaint();
        return;
    }

    const auto& equation = _equations->back();
    auto& lineOfDisplay = _elementsDisplay.back();
    while (equation.size() < lineOfDisplay.size()) {
        delete lineOfDisplay.back();
        lineOfDisplay.pop_back();
    }
    if (equation.empty()) {
        lineOfDisplay.push_back(new ElementDisplay(this));
        adjustElementsDisplayGeo();
        repaint();
        return;
    }
    for (int i = std::max(int(equation.size()) - 2, 0); i < equation.size(); ++i) {
        if (i >= lineOfDisplay.size()) {
            lineOfDisplay.push_back(new ElementDisplay(this, equation[i].get()));
        } else {
            lineOfDisplay[i]->setElement(equation[i].get());
        }
    }
    adjustElementsDisplayGeo();
    update();
}

void Display::adjustElementsDisplayGeo()
{
    if (_elementsDisplay.empty() || _elementsDisplay.back().empty()){
        _paths.clear();
        return;
    }
    const int xMargin = 1;
    const int yMargin = 3;

    int lastX = width();
    int lastY = height();

    for (int line = _elementsDisplay.size() - 1; line >= 0; --line) {
        for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
            auto* display = _elementsDisplay[line][column];
            if (line < _elementsDisplay.size() - 1 &&
                display->font().pointSize() > g_normalHistSizePointSize) {
                QFont font = display->font();
                font.setPointSize(g_normalHistSizePointSize);
                display->setFont(font);
            }
            display->adjustSize();
            display->show();
            lastX -= display->width() + xMargin;
            display->move(lastX, lastY - display->height());
        }

        while (lastX < 0 && line == _elementsDisplay.size() - 1 && !_elementsDisplay[line].empty()) {
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
        lastX = width();
        lastY -= _elementsDisplay[line].back()->height() + yMargin;
    }

    updateConnectionForSecondLastLine();
    regeneratePaths();
}

void Display::updateConnectionForSecondLastLine()
{
    const int displayLineCount = _elementsDisplay.size();
    if (displayLineCount < 2)
        return;
    auto& secondLastLine = _elementsDisplay[displayLineCount - 2];
    for (int column = secondLastLine.size() - 1; column >= 0; --column) {
        secondLastLine[column]->clearAllNext();
    }
    auto& lastLine = _elementsDisplay.back();
    for (int column = secondLastLine.size() - 1; column >= 0; --column) {
        auto* const displayFromPreviousLine = secondLastLine[column];
        auto* const previousLineElement = dynamic_cast<Number*>(displayFromPreviousLine->element());
        if (!previousLineElement)
            continue;
        for (int latestDisplayIndex = lastLine.size() - 1; latestDisplayIndex >= 0;
             --latestDisplayIndex) {
            auto* lastLineElement = dynamic_cast<Number*>(lastLine[latestDisplayIndex]->element());
            if (lastLineElement && (*previousLineElement)==(*lastLineElement)) {
                displayFromPreviousLine->addNext(lastLine[latestDisplayIndex]);
            }
        }
    }
}

void Display::regeneratePaths()
{
    _paths.clear();
    for (int r = 0; r < _elementsDisplay.size(); ++r) {
        for (int c = 0; c < _elementsDisplay[r].size(); ++c) {
            auto* const display = _elementsDisplay[r][c];
            for (const auto& next : display->nexts()) {
                if (!next)
                    continue;
                addPath(display, next);
            }
        }
    }
}

void Display::addPath(ElementDisplay* one, ElementDisplay* other)
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
    _paths.emplace_back(QPointF(one->pos()) + QPointF(one->width() / 2, one->height()),
                        other->pos() + QPointF(one->width() / 2, 0), color);
}

void Display::paintEvent(QPaintEvent* event)
{
    if (ElementDisplay::showConnections)
        drawPaths();
    _menu->raise();
    _menuButton->raise();
    QWidget::paintEvent(event);
}

void Display::drawPaths()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const auto& path : _paths) {
        QPen pen(path.color, g_connectionLineWidth);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawPath(path);
    }
}

void Display::toggleMenu(bool show)
{
    _animation->setStartValue(_menu->pos());
    _animation->setEndValue(QPoint(show ? 0 : -_menu->width(), _menu->y()));
    _animation->start();
    repaint(_menu->geometry());
}

void Display::pasteAllResults() const
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
    if (ElementDisplay::showConnections == show)
        return;
    ElementDisplay::showConnections = show;
    update();
}

void Display::clearAllHistory()
{
    _equations->clear();
    emit _equations->changed();
}
