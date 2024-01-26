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
    ElementPath() = default;
    ElementPath(const QPointF& start, const QPointF& end, const QColor& color);
    QColor color;
};

class Display : public QWidget
{
    Q_OBJECT
public:
    explicit Display(QWidget* parent = nullptr);
    ~Display(){};

    void setEquations(const std::shared_ptr<EquationQueue>& equations);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void alignElementDisplayContent();
    void pasteAllResults() const;
    void toggleConnection(bool show);
    void toggleMenu(bool show);
    void clearAllHistory();

private:
    void adjustElementsDisplayGeo(bool newLineAdded);
    void updateConnectionForSecondLastLine();
    void regeneratePaths();
    void drawPaths();
    void addPath(ElementDisplay* one, ElementDisplay* other);

    std::shared_ptr<EquationQueue> _equations;
    std::vector<std::vector<ElementDisplay*>> _elementsDisplay;
    std::vector<ElementPath> _paths;
    QToolButton* _menuButton;
    Menu* _menu;
    QPropertyAnimation* _animation;
};

#endif // DISPLAY_H
