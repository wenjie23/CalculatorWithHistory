#ifndef DISPLAY_H
#define DISPLAY_H

#include <QPointer>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>

#include "math_elements.h"

class EquationQueue;
class ElementDisplay;
class QPropertyAnimation;
class Menu;
class QToolButton;

class ElementPath : public QObject, public QPainterPath
{
public:
    ElementPath(ElementDisplay* start, ElementDisplay* end, QObject* parent);
    QPointF startPoint() const;
    QPointF endPoint() const;
    void update();
    ElementDisplay* start;
    ElementDisplay* end;
    bool dirty = true;
};

class ScrollDisplay : public QScrollArea
{
    Q_OBJECT
public:
    explicit ScrollDisplay(QWidget* parent = nullptr);
    ~ScrollDisplay(){};

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void toggleMenu(bool show);
    void setBarToMax();

private:

    QPropertyAnimation* _animation;
    Menu* _menu;
    QToolButton* _menuButton;
};

class Display : public QWidget
{
    Q_OBJECT
public:
    explicit Display(QWidget* parent = nullptr);
    ~Display(){};

    void setEquations(const std::shared_ptr<EquationQueue>& equations);
    QScrollArea* scrollArea;

public slots:
    void alignElementDisplayContent();
    void pasteAllResults() const;
    void toggleConnection(bool show);
    void clearAllHistory();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override {
        return childrenRect().size();
    }
    void resizeEvent(QResizeEvent* event) override;

private:
    void adjustElementsDisplayGeo(bool newLineAdded);
    void adjustLastLineFontSize();
    void updateConnectionForSecondLastLine();
    void regeneratePaths();
    void drawPaths();
    void addPath(ElementDisplay* one, ElementDisplay* other);

    std::shared_ptr<EquationQueue> _equations;
    std::vector<std::unique_ptr<ElementPath>> _paths;
};

class ElementDisplay : public QLabel
{
    Q_OBJECT
    friend Display;
public:
    ElementDisplay(QWidget* parent) : ElementDisplay(parent, nullptr) {}
    ElementDisplay(QWidget* parent, Element* element, bool showConnection = true);

    void paintEvent(QPaintEvent* event) override;

    Element* element() const;
    void setElement(Element* e);
    std::shared_ptr<QColor> connectColor() const { return _connectColor; }
    void setConnectColor(const std::shared_ptr<QColor>& color) { _connectColor = color; }

    const std::vector<QPointer<ElementDisplay>>& nexts() { return _nexts; };
    void addNext(ElementDisplay* display);
    void clearAllNext();

private slots:
    void updateElementText();

private:
    QPointer<Element> _element;
    std::vector<QPointer<ElementDisplay>> _nexts;
    QPointer<ElementDisplay> _previous;
    std::shared_ptr<QColor> _connectColor;
    static bool _showConnections;
};
#endif // DISPLAY_H
