
#include <QKeyEvent>
#include <QDebug>

#include "main_window.h"
#include "ui_main_window.h"

namespace {
static const QString g_signOperatorText("+/-");
static const QString g_percentOperatorText("%");
static const QSize g_windowSize(640, 300);
static const QFont g_buttonFont(QStringLiteral("Arial"), 25);
}

extern const QString g_plus;
extern const QString g_minus;
extern const QString g_multiply;
extern const QString g_divide;

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->digit0, &QPushButton::clicked, this, [this]{ digitClicked(0); });
    connect(ui->digit1, &QPushButton::clicked, this, [this]{ digitClicked(1); });
    connect(ui->digit2, &QPushButton::clicked, this, [this]{ digitClicked(2); });
    connect(ui->digit3, &QPushButton::clicked, this, [this]{ digitClicked(3); });
    connect(ui->digit4, &QPushButton::clicked, this, [this]{ digitClicked(4); });
    connect(ui->digit5, &QPushButton::clicked, this, [this]{ digitClicked(5); });
    connect(ui->digit6, &QPushButton::clicked, this, [this]{ digitClicked(6); });
    connect(ui->digit7, &QPushButton::clicked, this, [this]{ digitClicked(7); });
    connect(ui->digit8, &QPushButton::clicked, this, [this]{ digitClicked(8); });
    connect(ui->digit9, &QPushButton::clicked, this, [this]{ digitClicked(9); });

    connect(ui->plus, &QPushButton::clicked, this,[this]{ binaryOperatorClicked(g_plus); });
    connect(ui->minus, &QPushButton::clicked, this,[this]{ binaryOperatorClicked(g_minus); });
    connect(ui->multiply, &QPushButton::clicked, this,[this]{ binaryOperatorClicked(g_multiply); });
    connect(ui->divide, &QPushButton::clicked, this,[this]{ binaryOperatorClicked(g_divide); });

    connect(ui->sign, &QPushButton::clicked, this,[this]{ unaryOperatorClicked(g_signOperatorText); });
    connect(ui->percent, &QPushButton::clicked, this,[this]{ unaryOperatorClicked(g_percentOperatorText); });

    connect(ui->equal, &QPushButton::clicked, this, &MainWindow::equalClicked);
    connect(ui->clear, &QPushButton::clicked, this, &MainWindow::clear);
    connect(ui->period, &QPushButton::clicked, this, &MainWindow::periodClicked);

    _equationQueue = std::make_shared<EquationQueue>();
    static_cast<Display*>(ui->display->widget())->setEquations(_equationQueue);
    // ui->display->scrollArea = ui->displayArea;

    setWindowTitle("CalculatorWithHistory");
    setFixedSize(g_windowSize);
    const auto allPButtons = findChildren<QPushButton*>();
    for (auto* button : allPButtons) {
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFont(g_buttonFont);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter) {
        enterClicked();
    } else if (event->key() == Qt::Key_Backspace) {
        _equationQueue->tryPopLastCharacter();
    }
    QWidget::keyPressEvent(event);
}

void MainWindow::digitClicked(uint8_t digit)
{
    assert(digit >=0 && digit <= 9);
    _equationQueue->append(digit);
}

void MainWindow::unaryOperatorClicked(const QString& op)
{
    if (_equationQueue->empty() || _equationQueue->back().empty() ||
        !dynamic_cast<Number*>(_equationQueue->back().back().get()))
        return;
    auto* const number = static_cast<Number*>(_equationQueue->back().back().get());
    const auto numberText = number->text();
    if (op == g_signOperatorText) {
        if (numberText[0] == '-') {
            number->trySetValue(numberText.right(numberText.size() - 1));
            emit _equationQueue->changed();
        } else {
            number->trySetValue(QStringLiteral("-").append(numberText));
            emit _equationQueue->changed();
        }
    } else if (op == g_percentOperatorText) {
        number->trySetValue(QString::number(numberText.toDouble() / 100));
        emit _equationQueue->changed();
    }
}

void MainWindow::binaryOperatorClicked(const QString& op)
{
    _equationQueue->append(op);
}

void MainWindow::periodClicked() { _equationQueue->appendDicimal(); }

void MainWindow::equalClicked()
{
    _equationQueue->append(QChar('='));
}

void MainWindow::enterClicked()
{
    if (_equationQueue->empty())
        return;
    if (_equationQueue->back().completed())
        ui->clear->animateClick();
    else
        ui->equal->animateClick();
}

void MainWindow::clear()
{
    if (_equationQueue->empty())
        return;
    if (_equationQueue->back().empty())
        _equationQueue->clear();
    else if (_equationQueue->back().completed()) {
        _equationQueue->emplace_back();
    } else {
        _equationQueue->back().clear();
    }
    emit _equationQueue->changed();
}
