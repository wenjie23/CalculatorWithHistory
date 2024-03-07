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

} // namespace


ElementPath::ElementPath(ElementDisplay* start, ElementDisplay* end, const QColor &color, QObject* parent):
    QObject(parent), start(start), end(end), color(color)
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
    vLayout->addStretch(1);
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
    if (_equations->size() > (layout()->count() - 1)) {
        auto* lastLineOfLayout = layout()->itemAt(layout()->count() - 1)->layout();
        if (lastLineOfLayout){
            for (int i = 0; i < lastLineOfLayout->count(); ++i){
                auto* display = dynamic_cast<ElementDisplay*>(lastLineOfLayout->itemAt(i)->widget());
                if (!display)
                    continue;
                display->show();
                QFont font = display->font();
                font.setPointSize(g_smallPointSize);
                display->setFont(font);
                QPalette palette = this->palette();
                palette.setColor(QPalette::WindowText, g_historyTextColor);
                display->setPalette(palette);
            }
        }

        // _elementsDisplay.emplace_back();
        auto* lineLayout = new QHBoxLayout();
        lineLayout->addStretch(1);
        layout()->addItem(lineLayout);
        newLineAdded = true;
    }
    // assert(_equations->size() <= _elementsDisplay.size());
    while (_equations->size() < (layout()->count() - 1)) {
        // auto& lastLineOfDisplay = _elementsDisplay.back();
        auto* lastLineLayout = layout()->itemAt(layout()->count() - 1)->layout();
        while (lastLineLayout->count() > 1) {
            // delete lastLineOfDisplay.back();
            // lastLineOfDisplay.pop_back();

            auto* display = lastLineLayout->takeAt(lastLineLayout->count() - 1);
            delete display;
        }
        // _elementsDisplay.pop_back();
    }

    if (_equations->empty() && layout()->count() == 1){
        adjustElementsDisplayGeo(newLineAdded);
        repaint();
        return;
    }

    const auto& equation = _equations->back();
    // auto& lineOfDisplay = _elementsDisplay.back();
    auto* lastLineLayout = static_cast<QHBoxLayout*>(layout()->itemAt(layout()->count() - 1)->layout());
    while (equation.size() < lastLineLayout->count() - 1) {
        auto* display = lastLineLayout->takeAt(lastLineLayout->count() - 1);
        delete display;
        // delete lineOfDisplay.back();
        // lineOfDisplay.pop_back();
    }
    if (equation.empty()) {
        lastLineLayout->insertWidget(-1, new ElementDisplay(this));
        // lineOfDisplay.push_back(new ElementDisplay(this));
        adjustElementsDisplayGeo(newLineAdded);
        repaint();
        return;
    }
    for (int i = std::max(int(equation.size()) - 2, 0); i < equation.size(); ++i) {
        // if (i >= lineOfDisplay.size()) {
        //     lineOfDisplay.push_back(new ElementDisplay(this, equation[i].get()));

        // } else {
        //     lineOfDisplay[i]->setElement(equation[i].get());
        // }

        if (i >= lastLineLayout->count() - 1)
        {
            auto* display = new ElementDisplay(this, equation[i].get());
            // display->show();
            lastLineLayout->addWidget(display);
            // qDebug() << lastLineLayout->itemAt(1)->widget()
        } else {
            static_cast<ElementDisplay*>(
                lastLineLayout->itemAt(lastLineLayout->count() - 1)->widget())->setElement(equation[i].get());
        }
    }
    adjustElementsDisplayGeo(newLineAdded);
    update();
}

void Display::adjustElementsDisplayGeo(bool newLineAdded)
{
    for (int i = 0; i < layout()->count(); ++i)
    {
        auto* line = layout()->itemAt(i)->layout();
        if (!line)
            continue;
        for (int c = 0; c < line->count(); ++c){
            auto* display = dynamic_cast<ElementDisplay*>(line->itemAt(c)->widget());
            if (!display)
                continue;
            display->show();
        }
    }
    updateConnectionForSecondLastLine();
    regeneratePaths();
    return;
    if (_elementsDisplay.empty() || _elementsDisplay.back().empty()){
        for (int i = 1; i < layout()->count(); ++i)
        {
            // layout()->remo
        }
        _paths.clear();
        return;
    }
    {
        const int line = _elementsDisplay.size() - 1;
        auto* lineLayout = new QHBoxLayout();
        layout()->addItem(lineLayout);
        lineLayout->addStretch(1);
        for (int column = 0; column < _elementsDisplay[line].size(); ++column) {
            qDebug() << "here" << size();

            auto* display = _elementsDisplay[line][column];
            if (line == _elementsDisplay.size() - 2) {
                QFont font = display->font();
                font.setPointSize(g_smallPointSize);
                display->setFont(font);
                QPalette palette = this->palette();
                palette.setColor(QPalette::WindowText, g_historyTextColor);
                display->setPalette(palette);
            }
            display->adjustSize();
            display->show();
            qDebug() << display->text();
            lineLayout->insertWidget(-1, display);
        }
        return;
    }

    const int xMargin = 1;
    const int yMargin = 3;

    int minX = 0;
    int minY = 0;
    {
        const int line = _elementsDisplay.size() - 1;
        int lastX = width();
        for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
            auto* display = _elementsDisplay[line][column];
            if (line == _elementsDisplay.size() - 2) {
                QFont font = display->font();
                font.setPointSize(g_smallPointSize);
                display->setFont(font);
                QPalette palette = this->palette();
                palette.setColor(QPalette::WindowText, g_historyTextColor);
                display->setPalette(palette);
            }
            display->adjustSize();
            display->show();
            lastX -= display->width() + xMargin;
            display->move(lastX, height() - display->height());
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
                display->move(lastX, height() - display->height());
            }
        }
        minX = std::min(minX, lastX);
    }
    if (!newLineAdded){

        if (minX < 0){
            // qDebug() << minX;
            // resize(width() - minX, height());
            // qDebug() << size();
            qDebug() << "~~" << size() << childrenRect() << sizeHint();
            adjustSize();
            // resize(childrenRect().size());
            qDebug() << size();
            updateConnectionForSecondLastLine();
            regeneratePaths();
}

        return;
    }


    for (int line = _elementsDisplay.size() - 2; line >= 0; --line) {
        int lastX = width();
        for (int column = _elementsDisplay[line].size() - 1; column >= 0; --column) {
            auto* display = _elementsDisplay[line][column];
            if (line == _elementsDisplay.size() - 2) {
                QFont font = display->font();
                font.setPointSize(g_smallPointSize);
                display->setFont(font);
                QPalette palette = this->palette();
                palette.setColor(QPalette::WindowText, g_historyTextColor);
                display->setPalette(palette);
                display->adjustSize();
                display->show();
                lastX -= display->width() + xMargin;
                display->move(lastX, height() - g_bigFontWidgetHeight - g_smallFontWidgetHeight - yMargin);
            } else {
                display->move(display->x(), display->y() - g_smallFontWidgetHeight - yMargin);
            }
        }
        minX = std::min(minX, lastX);
    }
    if (minX < 0){
        qDebug() << minX;
        // resize(width() - minX, height());
        qDebug() << "~~" << size();
        adjustSize();
        qDebug() << size();

    }
    updateConnectionForSecondLastLine();
    regeneratePaths();

}

void Display::updateConnectionForSecondLastLine()
{

    const int displayLineCount = layout()->count() - 1;
    if (displayLineCount < 2)
        return;
    auto* secondLastLine = layout()->itemAt(displayLineCount - 1)->layout();
    for (int column = 0; column < secondLastLine->count(); ++column){
        auto* display = dynamic_cast<ElementDisplay*>(secondLastLine->itemAt(column)->widget());
        if (!display)
            continue;
        display->clearAllNext();
    }

    auto* lastLine = layout()->itemAt(displayLineCount)->layout();
    for (int column = secondLastLine->count() - 1; column >= 0; --column){
        auto* const displayFromPreviousLine = dynamic_cast<ElementDisplay*>(
            secondLastLine->itemAt(column)->widget());
        if (!displayFromPreviousLine)
            continue;
        auto* const previousLineElement = dynamic_cast<Number*>(displayFromPreviousLine->element());
        if (!previousLineElement)
            continue;
        for (int latestDisplayIndex = lastLine->count() - 1; latestDisplayIndex >= 0;
             --latestDisplayIndex) {
            auto* lastLineDisplay = dynamic_cast<ElementDisplay*>(lastLine->itemAt(latestDisplayIndex)->widget());
            if (!lastLineDisplay)
                continue;
            auto* lastLineElement = dynamic_cast<Number*>(lastLineDisplay->element());

            if (lastLineElement && !lastLineDisplay->_previous &&
                (*previousLineElement)==(*lastLineElement)) {
                displayFromPreviousLine->addNext(lastLineDisplay);
            }
        }
    }

    // for (int column = secondLastLine.size() - 1; column >= 0; --column) {
    //      secondLastLine[column]->clearAllNext();
    // }
    // auto& lastLine = _elementsDisplay.back();
    // for (int column = secondLastLine.size() - 1; column >= 0; --column) {
    //     auto* const displayFromPreviousLine = secondLastLine[column];
    //     auto* const previousLineElement = dynamic_cast<Number*>(displayFromPreviousLine->element());
    //     if (!previousLineElement)
    //         continue;
    //     for (int latestDisplayIndex = lastLine.size() - 1; latestDisplayIndex >= 0;
    //          --latestDisplayIndex) {
    //         auto* lastLineElement = dynamic_cast<Number*>(lastLine[latestDisplayIndex]->element());
    //         if (lastLineElement && !lastLine[latestDisplayIndex]->_previous &&
    //             (*previousLineElement)==(*lastLineElement)) {
    //             displayFromPreviousLine->addNext(lastLine[latestDisplayIndex]);
    //         }
    //     }
    // }
}

void Display::regeneratePaths()
{
    _paths.clear();
    for (int r = 0; r < layout()->count(); ++r){
        auto* line = layout()->itemAt(r)->layout();
        if (!line)
            continue;
        for (int c = 0; c < line->count(); ++c){
            auto* display = dynamic_cast<ElementDisplay*>(line->itemAt(c)->widget());
            if (!display)
                continue;
            for (const auto& next : display->nexts()) {
                if (!next)
                    continue;
                addPath(display, next);
            }
        }
    }


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
        color.setHsl(QRandomGenerator::global()->bounded(361), 92, 158);
        one->setConnectColor(color);
    }
    other->setConnectColor(color);
    _paths.emplace_back(new ElementPath(one, other, color, this));
}

void Display::paintEvent(QPaintEvent* event)
{
    if (ElementDisplay::showConnections)
        drawPaths();
    QWidget::paintEvent(event);
}

void Display::resizeEvent(QResizeEvent *event)
{
    for (auto* path : _paths) {
        path->update();
    }
    QWidget::resizeEvent(event);

}

void Display::drawPaths()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (auto* path : _paths) {
        QPen pen(path->color, g_connectionLineWidth);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawPath(*path);
    }
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

ScrollDisplay::ScrollDisplay(QWidget *parent): QScrollArea(parent)
{
    auto* display = new Display(this);
    setWidget(display);
    setWidgetResizable(true);
    setAlignment(Qt::AlignLeft | Qt::AlignBottom);


    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &ScrollDisplay::onHorizontalRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ScrollDisplay::onVerticalRangeChanged);

    _menu = new Menu(this);
    connect(_menu, &Menu::pasteButtonClicked, display, &Display::pasteAllResults);
    connect(_menu, &Menu::connectionButtonToggled, display, &Display::toggleConnection);
    connect(_menu, &Menu::clearButtonClicked, display, &Display::clearAllHistory);

    _animation = new QPropertyAnimation(_menu, "pos", this);
    _animation->setDuration(g_animationDuration);
    _animation->setEasingCurve(QEasingCurve::InOutQuad);

    _menuButton = new QToolButton(this);
    _menuButton->setIcon(QIcon(g_menuButtonFileName));
    _menuButton->setStyleSheet(_menu->styleSheet());
    _menuButton->setFixedSize(g_menuButtonSize);
    _menuButton->move(g_menuButtonPos);
    _menuButton->setCheckable(true);
    connect(_menuButton, &QPushButton::toggled, this, &ScrollDisplay::toggleMenu);

    toggleMenu(false);
    _menu->raise();
    _menuButton->raise();
}
bool ElementDisplay::showConnections = true;

void ScrollDisplay::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ShiftModifier) // If shift key is pressed
    {
        QWheelEvent horizontalEvent(event->position(), event->globalPosition(), event->pixelDelta(), event->angleDelta().transposed(),
                                    event->buttons(), event->modifiers(), event->phase(), true);
        QScrollArea::wheelEvent(&horizontalEvent);
    }
    else
    {
        QScrollArea::wheelEvent(event);
    }
}

void ScrollDisplay::onHorizontalRangeChanged(int min, int max)
{
    horizontalScrollBar()->setValue(max);
}

void ScrollDisplay::onVerticalRangeChanged(int min, int max)
{
    verticalScrollBar()->setValue(max);
}

void ScrollDisplay::toggleMenu(bool show)
{
    _animation->setStartValue(_menu->pos());
    _animation->setEndValue(QPoint(show ? 0 : -_menu->width(), _menu->y()));
    _animation->start();
    repaint(_menu->geometry());
}

#include "moc_display.cpp"

ElementDisplay::ElementDisplay(QWidget *parent, Element *element, bool showConnection) : QLabel(parent)
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
}

void ElementDisplay::paintEvent(QPaintEvent *event)
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

Element *ElementDisplay::element() const { return _element; }

void ElementDisplay::setElement(Element *e)
{
    if (_element == e)
        return;
    disconnect(e, &Element::changed, this, &ElementDisplay::updateElementText);
    connect(e, &Element::changed, this, &ElementDisplay::updateElementText);
    _element = e;
    updateElementText();
}

void ElementDisplay::addNext(ElementDisplay *display)
{
    if (!display)
        return;
    _nexts.push_back(display);
    display->_previous = this;
}

void ElementDisplay::clearAllNext()
{
    while (!_nexts.empty()) {
        if (_nexts.back())
            _nexts.back()->_previous = nullptr;
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
