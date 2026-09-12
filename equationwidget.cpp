#include "equationwidget.h"

#include <QFocusEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

EquationWidget::EquationWidget(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(160);
    setCursor(Qt::IBeamCursor);
}

void EquationWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    QFont mathFont = font();
    mathFont.setPointSize(24);
    painter.setFont(mathFont);
    painter.setPen(palette().color(QPalette::Text));

    const QFontMetrics metrics(mathFont);
    const int startX = 24;
    const int baseline = height() / 2 + (metrics.ascent() - metrics.descent()) / 2;

    painter.drawText(startX, baseline, m_text);

    if (hasFocus())
    {
        const QString textBeforeCursor = m_text.left(m_cursorPosition);

        const int cursorX = startX + metrics.horizontalAdvance(textBeforeCursor);

        painter.drawLine(cursorX, baseline - metrics.ascent(), cursorX, baseline + metrics.descent());
    }
}

void EquationWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Left:
        if (m_cursorPosition > 0)
            --m_cursorPosition;
        break;

    case Qt::Key_Right:
        if (m_cursorPosition < m_text.size())
            ++m_cursorPosition;
        break;

    case Qt::Key_Home:
        m_cursorPosition = 0;
        break;

    case Qt::Key_End:
        m_cursorPosition = m_text.size();
        break;

    case Qt::Key_Backspace:
        if (m_cursorPosition > 0)
        {
            m_text.remove(m_cursorPosition - 1, 1);
            --m_cursorPosition;
        }
        break;

    case Qt::Key_Delete:
        if (m_cursorPosition < m_text.size())
            m_text.remove(m_cursorPosition, 1);
        break;

    default: {
        const QString input = event->text();

        if (!input.isEmpty() && input.front().isPrint())
        {
            m_text.insert(m_cursorPosition, input);
            m_cursorPosition += input.size();
        }
        else
        {
            QWidget::keyPressEvent(event);
            return;
        }
        break;
    }
    }

    update();
}

void EquationWidget::mousePressEvent(QMouseEvent *)
{
    setFocus();
    update();
}

void EquationWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    update();
}

void EquationWidget::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    update();
}
