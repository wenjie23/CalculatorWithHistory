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
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

#include "display.h"
#include "menu.h"

namespace {
constexpr int g_bigPointSize = 48;
constexpr int g_smallPointSize = 20;

const QString g_fontFamily("Arial");
constexpr int g_bigFontWidgetHeight = 76;
constexpr int g_smallFontWidgetHeight = 36;

constexpr int g_animationDuration = 300;

constexpr int g_hChannelUpperBound = 361;
constexpr int g_sChannel = 92;
constexpr int g_lChannel = 158;
constexpr double g_connectionLineWidth = 1.6;
constexpr QColor g_displayTextColor(38, 39, 42);
constexpr QColor g_historyTextColor(90, 90, 90);

constexpr QSize g_menuButtonSize(34, 30);
constexpr QPoint g_menuButtonPos(4, 3);
const QString g_menuButtonFileName(":/Button/menu_hamburger.png");

constexpr QChar g_minusSign = '-';
constexpr QChar g_leftParenthesis = '(';
constexpr QChar g_rightParenthesis = ')';

constexpr int g_elementRectDX = 1;
constexpr int g_elementRectDY = -2;
constexpr int g_elementRectRadius = 2;

QLayoutItem* lastItemInLayout(QLayout* layout)
{
    if (!layout)
        return nullptr;
    return layout->itemAt(layout->count() - 1);
}

QLayoutItem* takeLastItemInLayout(QLayout* layout)
{
    if (!layout)
        return nullptr;
    return layout->takeAt(layout->count() - 1);
}

int widgetsWidth(QLayout* layout)
{
    int totalWidth = 0;
    for (int c = 0; c < layout->count(); ++c) {
        totalWidth += layout->itemAt(c)->widget()->width();
    }
    return totalWidth;
}

bool changeWidgetsWidthBy(QLayout* layout, int change)
{
    QFont font = static_cast<ElementDisplay*>(layout->itemAt(0)->widget())->font();
    font.setPointSize(font.pointSize() + change);
    if (font.pointSize() > g_bigPointSize || font.pointSize() < g_smallPointSize) {
        return false;
    }

    for (int c = 0; c < layout->count(); ++c) {
        auto* display = static_cast<ElementDisplay*>(layout->itemAt(c)->widget());
        display->setFont(font);
        display->adjustSize();
        display->updateGeometry();
    }
    return true;
}
} // namespace

ElementPath::ElementPath(ElementDisplay* start, ElementDisplay* end, QObject* parent)
    : QObject(parent), _start(start), _end(end)
{
    update();
}

QPointF ElementPath::startPoint() const
{
    if (!_start)
        return {};
    return QPointF(_start->pos()) + QPointF(_start->width() / 2, _start->height());
}

QPointF ElementPath::endPoint() const
{
    if (!_end)
        return {};
    return _end->pos() + QPointF(_end->width() / 2, 0);
}

void ElementPath::update()
{
    clear();
    const QPointF startP = startPoint();
    const QPointF endP = endPoint();
    moveTo(startP);
    const double dx = endP.x() - startP.x();
    const double dy = endP.y() - startP.y();
    const int directionModifier = dx > 0 ? 1 : -1;
    constexpr double angle = qDegreesToRadians(30.f);
    const double offsetX = dy * qSin(angle);
    const double offsetY = dy * qCos(angle);

    const QPointF c1(startP.x() + directionModifier * offsetX, startP.y() + offsetY);
    const QPointF c2(endP.x() - directionModifier * offsetX, endP.y() - offsetY);
    cubicTo(c1, c2, endP);
}

QColor ElementPath::color() const
{
    if (!_start)
        return {};
    return *(_start->connectColor());
}

Display::Display(QWidget* parent) : QWidget(parent)
{
    auto* vLayout = new QVBoxLayout(this);
    vLayout->setAlignment(Qt::AlignBottom);
    setLayout(vLayout);
    adjustSize();
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
    bool newLineAdded = false;
    if (_equations->size() > layout()->count()) {
        if (!layout()->isEmpty()) {
            auto* const lastLineOfLayout = lastItemInLayout(layout())->layout();
            for (int i = 0; i < lastLineOfLayout->count(); ++i) {
                auto* display =
                    dynamic_cast<ElementDisplay*>(lastLineOfLayout->itemAt(i)->widget());
                if (!display)
                    continue;
                display->show();
                QFont font = display->font();
                font.setPointSize(g_smallPointSize);
                display->setFont(font);
                QPalette palette = this->palette();
                palette.setColor(QPalette::WindowText, g_historyTextColor);
                display->setPalette(palette);
                display->setFixedHeight(g_smallFontWidgetHeight);
            }
        }
        auto* lineLayout = new QHBoxLayout(this);
        lineLayout->setAlignment(Qt::AlignRight);
        layout()->addItem(lineLayout);
        newLineAdded = true;
    }
    while (_equations->size() < layout()->count()) {
        auto* const lastLineItem = takeLastItemInLayout(layout());
        while (auto* lastItem = takeLastItemInLayout(lastLineItem->layout())) {
            delete lastItem->widget();
            delete lastItem;
        }
        delete lastLineItem->layout();
    }

    if (_equations->empty() && layout()->count() == 0) {
        adjustElementsDisplayGeo(newLineAdded);
        update();
        return;
    }

    const auto& equation = _equations->back();
    auto* lastLineLayout = static_cast<QHBoxLayout*>(lastItemInLayout(layout())->layout());
    while (equation.size() < lastLineLayout->count()) {
        auto* display = takeLastItemInLayout(lastLineLayout);
        delete display->widget();
        delete display;
    }
    if (equation.empty()) {
        lastLineLayout->insertWidget(-1, new ElementDisplay(this));
        adjustElementsDisplayGeo(newLineAdded);
        update();
        return;
    }
    for (int i = std::max(int(equation.size()) - 2, 0); i < equation.size(); ++i) {
        if (i >= lastLineLayout->count()) {
            auto* display = new ElementDisplay(this, equation[i].get());
            if (lastLineLayout->count() > 0) {
                display->setFont(
                    static_cast<ElementDisplay*>(lastItemInLayout(lastLineLayout)->widget())
                        ->font());
            }
            display->show();
            lastLineLayout->addWidget(display);
        } else {
            static_cast<ElementDisplay*>(lastItemInLayout(lastLineLayout)->widget())
                ->setElement(equation[i].get());
        }
    }

    adjustLastLineFontSize();
    adjustElementsDisplayGeo(newLineAdded);
    update();
}

void Display::adjustLastLineFontSize()
{
    auto* lastLineLayout = static_cast<QHBoxLayout*>(lastItemInLayout(layout())->layout());
    if (!lastLineLayout || lastLineLayout->count() == 0)
        return;
    int totalWidth = widgetsWidth(lastLineLayout);
    while (totalWidth < 360) {
        if (!changeWidgetsWidthBy(lastLineLayout, 1))
            break;
        totalWidth = widgetsWidth(lastLineLayout);
    }

    while (totalWidth > 360) {
        if (!changeWidgetsWidthBy(lastLineLayout, -1))
            break;
        totalWidth = widgetsWidth(lastLineLayout);
    }
}

void Display::adjustElementsDisplayGeo(bool newLineAdded)
{
    for (int i = 0; i < layout()->count(); ++i) {
        auto* line = layout()->itemAt(i)->layout();
        if (!line)
            continue;
        for (int c = 0; c < line->count(); ++c) {
            auto* display = dynamic_cast<ElementDisplay*>(line->itemAt(c)->widget());
            if (display) {
                display->updateGeometry();
                display->show();
            }
        }
    }
    updateConnectionForSecondLastLine();
}

void Display::updateConnectionForSecondLastLine()
{
    const int displayLineCount = layout()->count();
    if (displayLineCount < 2)
        return;
    auto* secondLastLine = layout()->itemAt(displayLineCount - 2)->layout();
    for (int column = 0; column < secondLastLine->count(); ++column) {
        auto* display = dynamic_cast<ElementDisplay*>(secondLastLine->itemAt(column)->widget());
        if (!display)
            continue;
        display->clearAllNext();
    }
    auto* lastLine = layout()->itemAt(displayLineCount - 1)->layout();
    for (int column = secondLastLine->count() - 1; column >= 0; --column) {
        auto* const displayFromPreviousLine =
            dynamic_cast<ElementDisplay*>(secondLastLine->itemAt(column)->widget());
        if (!displayFromPreviousLine)
            continue;
        const auto* previousLineElement = dynamic_cast<const Number*>(displayFromPreviousLine->element());
        if (!previousLineElement)
            continue;
        for (int latestDisplayIndex = lastLine->count() - 1; latestDisplayIndex >= 0;
             --latestDisplayIndex) {
            auto* lastLineDisplay =
                dynamic_cast<ElementDisplay*>(lastLine->itemAt(latestDisplayIndex)->widget());
            if (!lastLineDisplay)
                continue;
            const auto* lastLineElement = dynamic_cast<const Number*>(lastLineDisplay->element());

            if (lastLineElement && !lastLineDisplay->_previous &&
                (*previousLineElement) == (*lastLineElement)) {
                displayFromPreviousLine->addNext(lastLineDisplay);
            }
        }
    }
}

void Display::regeneratePaths()
{
    _paths.clear();
    for (int r = 0; r < layout()->count(); ++r) {
        auto* line = layout()->itemAt(r)->layout();
        for (int c = 0; c < line->count(); ++c) {
            auto* display = dynamic_cast<ElementDisplay*>(line->itemAt(c)->widget());
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
    if (!one->connectColor()->isValid()) {
        one->connectColor()->setHsl(
            (std::hash<double>{}(static_cast<const Number*>(one->element())->value()) % g_hChannelUpperBound),
            g_sChannel, g_lChannel);
    }
    _paths.emplace_back(std::make_unique<ElementPath>(one, other, this));
}

void Display::paintEvent(QPaintEvent* event)
{
    if (ElementDisplay::_showConnections) {
        regeneratePaths();
        drawPaths();
    }
    QWidget::paintEvent(event);
}

QSize Display::sizeHint() const
{
    return childrenRect().size();
}

void Display::resizeEvent(QResizeEvent* event)
{
    for (auto& path : _paths) {
        path->update();
    }
    QWidget::resizeEvent(event);
}

void Display::drawPaths()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (auto& path : _paths) {
        QPen pen(path->color(), g_connectionLineWidth);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawPath(*path);
    }
}

void Display::pasteAllResults() const
{
    QString result;

    for (int r = 0; r < layout()->count(); ++r) {
        auto* line = layout()->itemAt(r)->layout();
        assert(line);
        for (int c = 0; c < line->count(); ++c) {
            auto* display = static_cast<ElementDisplay*>(line->itemAt(c)->widget());
            assert(display);

            result.append(display->text());
            if (display) {
                display->updateGeometry();
                display->show();
            }
        }
        result.append('\n');
    }
    result = result.trimmed();
    if (result.size() > 0)
        QApplication::clipboard()->setText(result);
}

void Display::toggleConnection(bool show)
{
    if (ElementDisplay::_showConnections == show)
        return;
    ElementDisplay::_showConnections = show;
    update();
}

void Display::clearAllHistory()
{
    if (!_equations->empty()) {
        _equations->clear();
        emit _equations->changed();
    }
}

ScrollDisplay::ScrollDisplay(QWidget* parent) : QScrollArea(parent)
{
    auto* display = new Display(this);
    setWidget(display);
    setWidgetResizable(true);
    setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    display->installEventFilter(this);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    horizontalScrollBar()->setSingleStep(5);
    verticalScrollBar()->setSingleStep(5);
    connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &ScrollDisplay::setBarToMax);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ScrollDisplay::setBarToMax);

    _menu = new Menu(this);
    connect(_menu, &Menu::copyButtonClicked, display, &Display::pasteAllResults);
    connect(_menu, &Menu::connectionButtonToggled, display, &Display::toggleConnection);
    connect(_menu, &Menu::clearButtonClicked, display, &Display::clearAllHistory);
    _menu->move(-_menu->width(), _menu->y());

    _animation = new QPropertyAnimation(_menu, "pos", this);
    _animation->setDuration(g_animationDuration);
    _animation->setEasingCurve(QEasingCurve::InOutQuad);
    toggleMenu(false);

    _menuButton = new QToolButton(this);
    _menuButton->setIcon(QIcon(g_menuButtonFileName));
    _menuButton->setStyleSheet(_menu->styleSheet());
    _menuButton->setFixedSize(g_menuButtonSize);
    _menuButton->move(g_menuButtonPos);
    _menuButton->setCheckable(true);
    connect(_menuButton, &QPushButton::toggled, this, &ScrollDisplay::toggleMenu);

    _menu->raise();
    _menuButton->raise();
    setStyleSheet(QStringLiteral("QScrollArea{border: none;}"));
}

void ScrollDisplay::paintEvent(QPaintEvent* event)
{
    QScrollArea::paintEvent(event);
    _menu->raise();
    _menuButton->raise();
}

void ScrollDisplay::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ShiftModifier) // If shift key is pressed
    {
        QWheelEvent horizontalEvent(event->position(), event->globalPosition(), event->pixelDelta(),
                                    event->angleDelta().transposed(), event->buttons(),
                                    event->modifiers() ^ Qt::ShiftModifier, event->phase(), true);
        QScrollArea::wheelEvent(&horizontalEvent);
    } else {
        QScrollArea::wheelEvent(event);
    }
}

bool ScrollDisplay::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::Paint)
        return false;
    const auto* paintEvent = static_cast<QPaintEvent*>(event);
    const auto* watchedWidget = static_cast<QWidget*>(watched);
    const QRect updateRectInCurrentCoordinate = paintEvent->rect().translated(watchedWidget->pos());
    const QRect menuRectInCurrentCoordinate = _menu->rect().translated(_menu->pos());
    if (updateRectInCurrentCoordinate.intersects(menuRectInCurrentCoordinate))
        _menu->update();
    return false;
}

void ScrollDisplay::setBarToMax()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
}

void ScrollDisplay::toggleMenu(bool show)
{
    _animation->setStartValue(_menu->pos());
    _animation->setEndValue(QPoint(show ? 0 : -_menu->width(), _menu->y()));
    _animation->start();
    repaint(_menu->geometry());
}

ElementDisplay::ElementDisplay(QWidget* parent, Element* element, bool showConnection)
    : QLabel(parent), _connectColor(std::make_shared<QColor>())
{
    setElement(element);
    setMargin(2);

    setAlignment(Qt::AlignLeft | Qt::AlignCenter);
    QFont font(g_fontFamily, g_bigPointSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    setFont(font);

    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, g_displayTextColor);
    setPalette(palette);
    setFixedHeight(g_bigFontWidgetHeight);
}

void ElementDisplay::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
    if (_showConnections && (!_nexts.empty() || _previous)) {
        QPainter painter(this);
        QPen pen(_connectColor == nullptr ? Qt::gray : *_connectColor, g_connectionLineWidth);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawRoundedRect(
            rect().adjusted(g_elementRectDX, g_elementRectDX, g_elementRectDY, g_elementRectDY),
            g_elementRectRadius, g_elementRectRadius);
    }
}

const Element* ElementDisplay::element() const { return _element; }

void ElementDisplay::setElement(const Element* e)
{
    if (_element == e)
        return;
    disconnect(e, &Element::changed, this, &ElementDisplay::updateElementText);
    _element = e;
    updateElementText();
    if (_element)
        connect(_element, &Element::changed, this, &ElementDisplay::updateElementText);
}

void ElementDisplay::addNext(ElementDisplay* display)
{
    if (!display)
        return;
    _nexts.push_back(display);
    display->_previous = this;
    if (!_connectColor)
        _connectColor = std::make_shared<QColor>();
    display->_connectColor = _connectColor;
}

void ElementDisplay::clearAllNext()
{
    while (!_nexts.empty()) {
        if (_nexts.back()) {
            _nexts.back()->_previous = nullptr;
            _nexts.back()->_connectColor.reset();
        }
        _nexts.pop_back();
    }
}

void ElementDisplay::updateElementText()
{
    if (!_element) {
        setText("");
    }
    auto textToShow = _element->text();
    if (text() == textToShow)
        return;
    if (textToShow.size() > 1 && textToShow[0] == g_minusSign)
        textToShow = g_leftParenthesis % textToShow % g_rightParenthesis;
    setText(textToShow);
}

bool ElementDisplay::_showConnections = true;

#include "moc_display.cpp"
