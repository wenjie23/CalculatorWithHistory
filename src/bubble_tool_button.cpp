#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QEvent>
#include <QMoveEvent>
#include "bubble_tool_button.h"

namespace {
int g_arrowWidth = 7;
int g_arrowHeight = 10;
int g_bubbleRadius = 7;
}

BubbleWidget::BubbleWidget(QWidget* const anchor): QDialog(anchor), _anchor(anchor), _label(new QLabel(this))
{
    assert(anchor->parentWidget());
    setModal(false);
    setWindowFlag(Qt::FramelessWindowHint);
    auto* layout = new QHBoxLayout(this);
    layout->addSpacing(g_arrowWidth);
    layout->addWidget(_label);
    layout->setContentsMargins(QMargins(7, 7, 7, 7));
    _label->setText("Text");
    setLayout(layout);
    setAttribute(Qt::WA_TranslucentBackground);
    move(QPoint(_anchor->width(), _anchor->height()));

    auto* current = _anchor;
    while (current){
        current->installEventFilter(this);
        current = current->parentWidget();
    }
}

void BubbleWidget::setText(const QString& text)
{
    _label->setText(text);
}

void BubbleWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // Enable smooth edges
    painter.setPen(Qt::NoPen); // No border
    painter.setBrush(QColor(128,128,128)); // Fill with the given color

    // Create a path that defines the shape of the bubble
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
    path.closeSubpath(); // Close the path

    // Draw the path on the widget
    painter.drawPath(path);

    QDialog::paintEvent(event);
}


bool BubbleWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Move){
        auto* moveEvent = static_cast<QMoveEvent*>(event);
        move(_anchor->mapToGlobal(QPoint(_anchor->width(), _anchor->height() / 2 - height() / 2)));
        return false;
    }
    else if (event->type() == QEvent::Hide){
        hide();
        return false;
    }
    return QObject::eventFilter(watched, event);
}

void BubbleWidget::moveEvent(QMoveEvent *event)
{
    if (!_boundary){
        QDialog::moveEvent(event);
    }
    QPoint point = mapToGlobal(event->pos() + QPoint(0, height() / 2));
    qDebug() << point << _boundary->rect();

}

BubbleToolButton::BubbleToolButton(QWidget *parent): QToolButton(parent), _bubbleWidget(new BubbleWidget(this))
{
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/Button/paste.png"), QSize(), QIcon::Normal, QIcon::Off);
    setIcon(icon);
    connect(this, &QToolButton::clicked, this, &BubbleToolButton::onClicked);

    _timer.setSingleShot(true);
    _timer.setInterval(1500);
    connect(&_timer, &QTimer::timeout, this, &BubbleToolButton::onTimeOut, Qt::QueuedConnection);
}

void BubbleToolButton::setClickedIcon(QIcon icon)
{
    icon.addPixmap(icon.pixmap(200), QIcon::Disabled);
    _clickedIcon = icon;
}


void BubbleToolButton::onClicked()
{
    qDebug() << __func__;
    if (_clickedIcon.isNull())
        return;
    _previousIcon = icon();
    setIcon(_clickedIcon);
    setEnabled(false);
    qDebug() << "start";

    _timer.start();
    _bubbleWidget->show();
    _bubbleWidget->setEnabled(true);
    qDebug() << "started";
}

void BubbleToolButton::onTimeOut()
{
    qDebug() << __func__;
    setIcon(_previousIcon);
    setEnabled(true);

    _bubbleWidget->hide();
}

#include "moc_bubble_tool_button.cpp"
