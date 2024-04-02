#ifndef BUBBLE_TOOL_BUTTON_H
#define BUBBLE_TOOL_BUTTON_H

#include <QToolButton>
#include <QTimer>
#include <QDialog>
#include <QPointer>

class QLabel;

class BubbleWidget : public QDialog
{
    Q_OBJECT
public:
    explicit BubbleWidget(QWidget* const anchor);
    void setText(const QString& text);
    void setBoundary(QWidget* widget) { _boundary = widget; }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

private:
    QWidget* const _anchor;
    QPointer<QWidget> _boundary;
    QLabel* const _label;
};

class BubbleToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit BubbleToolButton(QWidget* parent);
    void setClickedIcon(const QIcon& icon);
    void setBubbleText(const QString& text);
    BubbleWidget* const bubbleWidget() const { return _bubbleWidget; }

protected slots:
    void onClicked();
    void onTimeOut();

private:
    QIcon _clickedIcon;
    QIcon _previousIcon;
    QTimer _timer;
    BubbleWidget* const _bubbleWidget;
};

#endif // BUBBLE_TOOL_BUTTON_H
