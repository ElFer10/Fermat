#include "mainwindow.h"
#include "equationwidget.h"
#include "ui_mainwindow.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    auto *layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(12, 12, 12, 12);

    auto *equationWidget = new EquationWidget(ui->centralwidget);
    auto *toolBar = addToolBar(tr("Herramientas matemáticas"));
    toolBar->setMovable(false);

    auto *fractionAction = toolBar->addAction(tr("Fracción a/b"));
    fractionAction->setToolTip(tr("Convierte la expresión actual en numerador"));

    auto *rootAction = toolBar->addAction(tr("Raíz √"));
    rootAction->setToolTip(tr("Inserta una raíz cuadrada"));

    auto *clearAction = toolBar->addAction(tr("Limpiar"));
    clearAction->setToolTip(tr("Borra toda la expresión"));

    toolBar->addSeparator();

    auto *copyLatexAction = toolBar->addAction(tr("Copiar LaTeX"));

    copyLatexAction->setToolTip(tr("Copia la ecuación en formato LaTeX"));

    connect(copyLatexAction, &QAction::triggered, this, [this, equationWidget]() {
        const QString latex = equationWidget->toLatex();

        QApplication::clipboard()->setText(latex);

        statusBar()->showMessage(tr("LaTeX copiado: %1").arg(latex), 3000);
    });

    connect(rootAction, &QAction::triggered, equationWidget, &EquationWidget::insertRoot);
    connect(fractionAction, &QAction::triggered, equationWidget, &EquationWidget::insertFraction);
    connect(clearAction, &QAction::triggered, equationWidget, &EquationWidget::clearEquation);

    setWindowTitle(tr("Fermat"));
    resize(800, 400);
    layout->addWidget(equationWidget);

    equationWidget->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}
