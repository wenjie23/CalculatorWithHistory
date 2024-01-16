#ifndef DISPLAY_H
#define DISPLAY_H

#include <QLabel>
#include <QPainter>
#include <QPainterPath>

class EquationQueue;
class ElementDisplay;
class QPropertyAnimation;
class Menu;
class QToolButton;

struct ElementPath : public QPainterPath
{
    ElementPath() : QPainterPath() {}
    ElementPath(const QPointF& start, const QPointF& end, const QColor& color);
    QColor _color;
};

class Display : public QWidget
{
public:
    explicit Display(QWidget* parent = nullptr);
    ~Display(){};

    void paintEvent(QPaintEvent* event) override;
    void setEquations(const std::shared_ptr<EquationQueue>& equations);

private slots:
    void setUpdateToTrue();
    void pasteAllResults();
    void toggleConnection(bool show);
    void clearAllHistory();

private:
    std::shared_ptr<EquationQueue> _equations;
    bool _needUpdateElements = false;
    std::vector<std::vector<ElementDisplay*>> _elementsDisplay;
    void drawPaths();
    static QPainterPath calcPath(const QPointF& start, const QPointF& end);
    std::vector<ElementPath> _paths;
    QToolButton* _menuButton;
    Menu* _menu;
    QPropertyAnimation* _animation;

    ElementPath generatePath(ElementDisplay* one, ElementDisplay* other);

    void showMenu();
    void hideMenu();

    bool _showConnections = true;
};

#endif // DISPLAY_H
