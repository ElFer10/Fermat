#pragma once

#include "mathnode.h"

#include <QWidget>

#include <memory>

class EquationWidget final : public QWidget
{
  public:
    explicit EquationWidget(QWidget *parent = nullptr);

    void insertFraction();
    void clearEquation();

  protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

  private:
    void moveCursorLeft();
    void moveCursorRight();
    void moveToParent(bool placeAfterFraction);

    std::unique_ptr<RowNode> m_rootNode;

    // Apunta a una fila que pertenece a m_rootNode.
    RowNode *m_activeRow = nullptr;

    // Posición entre los hijos de m_activeRow.
    qsizetype m_cursorPosition = 0;
};
