#pragma once

#include <QWidget>

class EquationWidget final : public QWidget
{
  public:
    explicit EquationWidget(QWidget *parent = nullptr);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

  private:
    QString m_text;
    qsizetype m_cursorPosition = 0;
};
