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
static const int g_bigPointSize = 48;
static const int g_smallPointSize = 20;

static const QString g_fontFamily("Arial");
static const int g_bigFontWidgetHeight = 76;
static const int g_smallFontWidgetHeight = 36;

static const int g_animationDuration = 300;

static const int g_hChannelUpperBount = 361;
static const int g_sChannel = 92;
static const int g_lChannel = 158;
static const double g_connectionLineWidth = 1.6;
static const QColor g_displayTextColor(38, 39, 42);
static const QColor g_historyTextColor(90, 90, 90);

static const QSize g_menuButtonSize(34, 30);
static const QPoint g_menuButtonPos(4, 3);
static const QString g_menuButtonFileName(":/Button/menu_hamburger.png");

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
    : QObject(parent), start(start), end(end)
{
    update();
}

QPointF ElementPath::startPoint() const
{
    if (!start)
        return {};
    return QPointF(start->pos()) + QPointF(start->width() / 2, start->height());
}

QPointF ElementPath::endPoint() const
{
    if (!end)
        return {};
    return end->pos() + QPointF(end->width() / 2, 0);
}

void ElementPath::update()
{
    clear();
    const QPointF startP = startPoint();
    const QPointF endP = endPoint();
    moveTo(startP);
    const double dx = endP.x() - startP.x();
    const double dy = endP.y() - startP.y();
    const double dist = qSqrt(dx * dx + dy * dy);
    const double angle = qAtan2(dy, dx);
    const double offsetX = dist * 0.15 * qCos(angle + (dx > 0 ? M_PI / 8 : -M_PI / 8));
    const double offsetY = dist * 0.15 * qSin(angle + (dx > 0 ? M_PI / 8 : -M_PI / 8));

    const QPointF c1(startP.x() + offsetX, startP.y() + offsetY);
    const QPointF c2(endP.x() - offsetX, endP.y() - offsetY);

    cubicTo(c1, c2, endP);
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
        repaint();
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
        repaint();
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
    if (!lastLineLayout || lastLineLayout->isEmpty())
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
        auto* const previousLineElement = dynamic_cast<Number*>(displayFromPreviousLine->element());
        if (!previousLineElement)
            continue;
        for (int latestDisplayIndex = lastLine->count() - 1; latestDisplayIndex >= 0;
             --latestDisplayIndex) {
            auto* lastLineDisplay =
                dynamic_cast<ElementDisplay*>(lastLine->itemAt(latestDisplayIndex)->widget());
            if (!lastLineDisplay)
                continue;
            auto* lastLineElement = dynamic_cast<Number*>(lastLineDisplay->element());

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
            (std::hash<double>{}(static_cast<Number*>(one->element())->value()) % 361), 92, 158);
    }
    _paths.emplace_back(std::make_unique<ElementPath>(one, other, this));
}

void Display::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (ElementDisplay::_showConnections) {
        regeneratePaths();
        drawPaths();
    }
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
        QPen pen(*(path->start->connectColor()), g_connectionLineWidth);
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
    _equations->clear();
    emit _equations->changed();
}

ScrollDisplay::ScrollDisplay(QWidget* parent) : QScrollArea(parent)
{
    auto* display = new Display(this);
    setWidget(display);
    setWidgetResizable(true);
    setAlignment(Qt::AlignLeft | Qt::AlignBottom);

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
    _menu->raise();
    _menuButton->raise();
    QScrollArea::paintEvent(event);
}
bool ElementDisplay::_showConnections = true;

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
        painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 2, 2);
    }
}

Element* ElementDisplay::element() const { return _element; }

void ElementDisplay::setElement(Element* e)
{
    if (_element == e)
        return;
    disconnect(e, &Element::changed, this, &ElementDisplay::updateElementText);
    connect(e, &Element::changed, this, &ElementDisplay::updateElementText);
    _element = e;
    updateElementText();
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
    if (textToShow.size() > 1 && textToShow[0] == QChar('-'))
        textToShow = QStringLiteral("(") % textToShow % QStringLiteral(")");
    setText(textToShow);
}

#include "moc_display.cpp"
