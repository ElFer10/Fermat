#pragma once

#include "mathnode.h"
#include "mathstyle.h"

#include <QFont>
#include <QTimer>
#include <QWidget>

#include <memory>

class EquationWidget final : public QWidget
{
  public:
    explicit EquationWidget(QWidget *parent = nullptr);

    void insertFraction();
    void insertRoot();
    void insertSuperscript();
    void insertSubscript();
    void insertIntegral();
    void insertSummation();

    void clearEquation();

    QString toLatex() const;

    void setMathFont(const QFont &font);
    void setMathFontSize(int pointSize);
    void setVariablesItalic(bool enabled);

    QFont mathFont() const;
    int mathFontSize() const;
    bool variablesItalic() const;
    bool exportPdf(const QString &fileName, QString *errorMessage = nullptr) const;
    bool exportSvg(const QString &fileName, QString *errorMessage = nullptr) const;

  protected:
    void paintEvent(QPaintEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void focusInEvent(QFocusEvent *event) override;

    void focusOutEvent(QFocusEvent *event) override;

  private:
    void insertLargeOperator(LargeOperatorType type);

    void applyMathStyle();

    void resetCursorBlink();
    void moveCursorLeft();
    void moveCursorRight();

    void moveToParent(bool placeAfterStructure);

    std::unique_ptr<RowNode> m_rootNode;

    RowNode *m_activeRow = nullptr;
    qsizetype m_cursorPosition = 0;

    QTimer m_cursorTimer;
    bool m_cursorVisible = true;

    MathStyle m_mathStyle;
};
