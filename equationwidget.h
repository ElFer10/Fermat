#pragma once

#include "mathnode.h"

#include <QTimer>
#include <QWidget>

#include <memory>

class EquationWidget final : public QWidget
{
  public:
    explicit EquationWidget(QWidget *parent = nullptr);

    void insertRoot();
    void insertSuperscript();
    void insertSubscript();
    void insertFraction();
    void clearEquation();
    void insertIntegral();
    void insertSummation();

    QString toLatex() const;

  protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

  private:
    // Navegación del cursor
    void moveCursorLeft();
    void moveCursorRight();
    void moveToParent(bool placeAfterFraction);
    void resetCursorBlink();
    void insertLargeOperator(LargeOperatorType type);

    // Parpadeo del cursor
    QTimer m_cursorTimer;
    bool m_cursorVisible = true;

    // Modelo de la ecuación
    std::unique_ptr<RowNode> m_rootNode;

    RowNode *m_activeRow = nullptr; // Apunta a una fila que pertenece a m_rootNode.
    qsizetype m_cursorPosition = 0; // Posición entre los hijos de m_activeRow.
};
