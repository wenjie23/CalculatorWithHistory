#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QEvent>
#include <QMoveEvent>
#include "bubble_tool_button.h"

namespace {
constexpr int g_arrowWidth = 7;
constexpr int g_arrowHeight = 10;
constexpr int g_bubbleRadius = 7;
constexpr QColor g_bubbleBackground(253, 253, 253);
const QString g_defaultBubbleText("Text");
constexpr int g_bubbleButtonTipTime = 1500;
constexpr int g_bubbleWidgetContentMargin = 7;
} // namespace

BubbleWidget::BubbleWidget(QWidget* const anchor)
    : QDialog(anchor), _anchor(anchor), _label(new QLabel(this))
{
    setModal(false);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    auto* layout = new QHBoxLayout(this);
    layout->addSpacing(g_arrowWidth);
    layout->addWidget(_label);
    layout->setContentsMargins(QMargins(g_bubbleWidgetContentMargin, g_bubbleWidgetContentMargin,
                                        g_bubbleWidgetContentMargin, g_bubbleWidgetContentMargin));
    _label->setText(g_defaultBubbleText);
    setLayout(layout);
    setAttribute(Qt::WA_TranslucentBackground);
    move(_anchor->mapToGlobal(QPoint(_anchor->width(), _anchor->height() / 2 - height() / 2)));

    auto* current = _anchor;
    while (current) {
        current->installEventFilter(this);
        current = current->parentWidget();
    }
}

void BubbleWidget::setText(const QString& text) { _label->setText(text); }

void BubbleWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_bubbleBackground); // Fill with the given color

    QPainterPath path;
    path.moveTo(0, height() / 2);
    path.lineTo(g_arrowWidth, height() / 2 - g_arrowHeight / 2);
    path.lineTo(g_arrowWidth, g_bubbleRadius);
    path.quadTo(g_arrowWidth, 0, g_arrowWidth + g_bubbleRadius, 0);
    path.lineTo(width() - g_bubbleRadius, 0);
    path.quadTo(width(), 0, width(), g_bubbleRadius);
    path.lineTo(width(), height() - g_bubbleRadius);
    path.quadTo(width(), height(), width() - g_bubbleRadius, height());
    path.lineTo(g_arrowWidth + g_bubbleRadius, height());
    path.quadTo(g_arrowWidth, height(), g_arrowWidth, height() - g_bubbleRadius);
    path.lineTo(g_arrowWidth, height() / 2 + g_arrowHeight / 2);
    path.closeSubpath();
    painter.drawPath(path);

    QDialog::paintEvent(event);
}

bool BubbleWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Move) {
        auto* moveEvent = static_cast<QMoveEvent*>(event);
        auto* movingWidget = static_cast<QWidget*>(watched);
        if (movingWidget == _anchor) {
            move(_anchor->mapToGlobal(
                QPoint(_anchor->width(), _anchor->height() / 2 - height() / 2)));
        } else {
            move(pos() + moveEvent->pos() - moveEvent->oldPos());
        }
        return false;
    } else if (event->type() == QEvent::Hide) {
        hide();
        return false;
    }
    return QObject::eventFilter(watched, event);
}

void BubbleWidget::moveEvent(QMoveEvent* event)
{
    if (!_boundary) {
        return QDialog::moveEvent(event);
    }
    if (!_boundary->rect()
             .translated(_boundary->mapToGlobal(QPoint(0, 0)))
             .contains(event->pos())) {
        hide();
    }
    return QDialog::moveEvent(event);
}

BubbleToolButton::BubbleToolButton(QWidget* parent)
    : QToolButton(parent), _bubbleWidget(new BubbleWidget(this))
{
    connect(this, &QToolButton::clicked, this, &BubbleToolButton::onClicked);
    _timer.setSingleShot(true);
    _timer.setInterval(g_bubbleButtonTipTime);
    connect(&_timer, &QTimer::timeout, this, &BubbleToolButton::onTimeOut, Qt::QueuedConnection);
}

void BubbleToolButton::setClickedIcon(const QIcon& icon)
{
    QIcon copy = icon;
    copy.addPixmap(icon.pixmap(200), QIcon::Disabled);
    _clickedIcon = copy;
}

void BubbleToolButton::onClicked()
{
    if (_clickedIcon.isNull())
        return;
    _previousIcon = icon();
    setIcon(_clickedIcon);
    setEnabled(false);

    _timer.start();
    _bubbleWidget->show();
    _bubbleWidget->setEnabled(true);
}

void BubbleToolButton::onTimeOut()
{
    setIcon(_previousIcon);
    setEnabled(true);
    _bubbleWidget->hide();
}

#include "moc_bubble_tool_button.cpp"
