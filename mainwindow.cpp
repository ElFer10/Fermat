#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "equationwidget.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFontComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto *layout = new QVBoxLayout(ui->centralwidget);

    layout->setContentsMargins(12, 12, 12, 12);

    auto *equationWidget = new EquationWidget(ui->centralwidget);

    layout->addWidget(equationWidget);

    auto *toolBar = addToolBar(tr("Herramientas matemáticas"));

    toolBar->setMovable(false);

    // ========================================================
    // Estructuras matemáticas
    // ========================================================

    auto *fractionAction = toolBar->addAction(tr("Fracción a/b"));

    fractionAction->setToolTip(tr("Inserta una fracción"));

    connect(fractionAction, &QAction::triggered, equationWidget, &EquationWidget::insertFraction);

    auto *rootAction = toolBar->addAction(tr("Raíz √"));

    rootAction->setToolTip(tr("Inserta una raíz cuadrada"));

    connect(rootAction, &QAction::triggered, equationWidget, &EquationWidget::insertRoot);

    auto *superscriptAction = toolBar->addAction(tr("Superíndice xⁿ"));

    superscriptAction->setToolTip(tr("Añade un superíndice"));

    connect(superscriptAction, &QAction::triggered, equationWidget, &EquationWidget::insertSuperscript);

    auto *subscriptAction = toolBar->addAction(tr("Subíndice xₙ"));

    subscriptAction->setToolTip(tr("Añade un subíndice"));

    connect(subscriptAction, &QAction::triggered, equationWidget, &EquationWidget::insertSubscript);

    auto *integralAction = toolBar->addAction(tr("Integral ∫"));

    integralAction->setToolTip(tr("Inserta una integral con límites"));

    connect(integralAction, &QAction::triggered, equationWidget, &EquationWidget::insertIntegral);

    auto *summationAction = toolBar->addAction(tr("Sumatoria ∑"));

    summationAction->setToolTip(tr("Inserta una sumatoria con límites"));

    connect(summationAction, &QAction::triggered, equationWidget, &EquationWidget::insertSummation);

    toolBar->addSeparator();

    // ========================================================
    // Selector de fuente
    // ========================================================

    auto *fontLabel = new QLabel(tr("Fuente:"), toolBar);

    toolBar->addWidget(fontLabel);

    auto *fontCombo = new QFontComboBox(toolBar);

    fontCombo->setFontFilters(QFontComboBox::ScalableFonts);

    fontCombo->setCurrentFont(equationWidget->mathFont());

    fontCombo->setMinimumWidth(170);

    toolBar->addWidget(fontCombo);

    connect(fontCombo, &QFontComboBox::currentFontChanged, equationWidget, &EquationWidget::setMathFont);

    // ========================================================
    // Selector de tamaño
    // ========================================================

    auto *sizeLabel = new QLabel(tr(" Tamaño:"), toolBar);

    toolBar->addWidget(sizeLabel);

    auto *sizeSpinBox = new QSpinBox(toolBar);

    sizeSpinBox->setRange(8, 96);
    sizeSpinBox->setSuffix(tr(" pt"));

    sizeSpinBox->setValue(equationWidget->mathFontSize());

    sizeSpinBox->setToolTip(tr("Tamaño de la ecuación"));

    toolBar->addWidget(sizeSpinBox);

    connect(sizeSpinBox, &QSpinBox::valueChanged, equationWidget, &EquationWidget::setMathFontSize);

    // ========================================================
    // Cursiva para variables
    // ========================================================

    auto *italicVariablesAction = toolBar->addAction(tr("Variables en cursiva"));

    italicVariablesAction->setCheckable(true);

    italicVariablesAction->setChecked(equationWidget->variablesItalic());

    italicVariablesAction->setToolTip(tr("Aplica cursiva solamente a variables "
                                         "de una letra"));

    connect(italicVariablesAction, &QAction::toggled, equationWidget, &EquationWidget::setVariablesItalic);

    toolBar->addSeparator();

    // ========================================================
    // Exportación y limpieza
    // ========================================================

    auto *copyLatexAction = toolBar->addAction(tr("Copiar LaTeX"));

    connect(copyLatexAction, &QAction::triggered, this, [this, equationWidget]() {
        const QString latex = equationWidget->toLatex();

        QApplication::clipboard()->setText(latex);

        statusBar()->showMessage(tr("LaTeX copiado: %1").arg(latex), 3000);
    });

    auto *clearAction = toolBar->addAction(tr("Limpiar"));

    connect(clearAction, &QAction::triggered, equationWidget, &EquationWidget::clearEquation);

    setWindowTitle(tr("Fermat"));

    resize(1100, 500);

    equationWidget->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}
