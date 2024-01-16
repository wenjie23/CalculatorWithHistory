#ifndef DISPLAY_H
#define DISPLAY_H

#include <QLabel>
#include <QPainter>

class EquationQueue;
class ElementDisplay;

class Display : public QWidget
{
public:
    explicit Display(QWidget* parent = nullptr);
    ~Display(){};

    void paintEvent(QPaintEvent* event) override;
    void setEquations(const std::shared_ptr<EquationQueue>& equations);

private slots:
    void setUpdateToTrue();

private:
    std::shared_ptr<EquationQueue> _equations;
    bool _needUpdateElements = false;
    std::vector<std::vector<ElementDisplay*>> _elementsDisplay;
    void drowConnections(ElementDisplay* one, ElementDisplay* other);
    static QPainterPath calcPath(const QPointF& start, const QPointF& end);
};

#endif // DISPLAY_H
