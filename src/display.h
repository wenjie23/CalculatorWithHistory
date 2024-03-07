#ifndef DISPLAY_H
#define DISPLAY_H

#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>

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

class ScrollDisplay : public QScrollArea
{
    Q_OBJECT
public:
    explicit ScrollDisplay(QWidget* parent = nullptr);
    ~ScrollDisplay(){};

protected:
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void toggleMenu(bool show);
    void onHorizontalRangeChanged(int min, int max);
    void onVerticalRangeChanged(int min, int max);

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

private:
    void adjustElementsDisplayGeo(bool newLineAdded);
    void updateConnectionForSecondLastLine();
    void regeneratePaths();
    void drawPaths();
    void addPath(ElementDisplay* one, ElementDisplay* other);

    std::shared_ptr<EquationQueue> _equations;
    std::vector<std::vector<ElementDisplay*>> _elementsDisplay;
    std::vector<ElementPath> _paths;
};

#endif // DISPLAY_H
