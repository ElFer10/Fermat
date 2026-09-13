#include "equationwidget.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

namespace
{

bool findRowPosition(const RowNode *currentRow, const RowNode *targetRow, const QPointF &currentPosition,
                     const QFontMetricsF &metrics, QPointF &result)
{
    if (currentRow == targetRow)
    {
        result = currentPosition;
        return true;
    }

    const NodeLayout rowLayout = currentRow->layout(metrics);

    qreal currentX = currentPosition.x();

    for (qsizetype index = 0; index < currentRow->childCount(); ++index)
    {
        const MathNode *child = currentRow->childAt(index);

        const NodeLayout childLayout = child->layout(metrics);

        const QPointF childPosition(currentX, currentPosition.y() + rowLayout.baseline - childLayout.baseline);

        const auto *fraction = dynamic_cast<const FractionNode *>(child);

        if (fraction)
        {
            const QPointF numeratorPosition = fraction->numeratorPosition(childPosition, metrics);

            if (findRowPosition(fraction->numeratorRow(), targetRow, numeratorPosition, metrics, result))
                return true;

            const QPointF denominatorPosition = fraction->denominatorPosition(childPosition, metrics);

            if (findRowPosition(fraction->denominatorRow(), targetRow, denominatorPosition, metrics, result))
                return true;
        }

        currentX += childLayout.size.width();
    }

    return false;
}

} // namespace

EquationWidget::EquationWidget(QWidget *parent)
    : QWidget(parent), m_rootNode(std::make_unique<RowNode>()), m_activeRow(m_rootNode.get())
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(240);
    setCursor(Qt::IBeamCursor);
}

void EquationWidget::insertFraction()
{
    auto fraction = std::make_unique<FractionNode>();
    FractionNode *fractionPointer = fraction.get();

    m_activeRow->insertNode(m_cursorPosition, std::move(fraction));

    // Al insertar, entramos directamente al numerador.
    m_activeRow = fractionPointer->numeratorRow();
    m_cursorPosition = 0;

    setFocus();
    update();
}

void EquationWidget::clearEquation()
{
    m_rootNode = std::make_unique<RowNode>();
    m_activeRow = m_rootNode.get();
    m_cursorPosition = 0;

    setFocus();
    update();
}

void EquationWidget::moveToParent(bool placeAfterFraction)
{
    FractionNode *owner = m_activeRow->ownerFraction();

    if (!owner)
        return;

    RowNode *parent = owner->parentRow();

    if (!parent)
        return;

    const qsizetype fractionIndex = parent->indexOf(owner);

    if (fractionIndex < 0)
        return;

    m_activeRow = parent;

    m_cursorPosition = placeAfterFraction ? fractionIndex + 1 : fractionIndex;
}

void EquationWidget::moveCursorLeft()
{
    if (m_cursorPosition > 0)
    {
        MathNode *previousNode = m_activeRow->childAt(m_cursorPosition - 1);

        // Si encontramos una fracción, entramos por
        // su extremo derecho: el denominador.
        if (auto *fraction = dynamic_cast<FractionNode *>(previousNode))
        {
            m_activeRow = fraction->denominatorRow();

            m_cursorPosition = m_activeRow->childCount();

            return;
        }

        --m_cursorPosition;
        return;
    }

    FractionNode *owner = m_activeRow->ownerFraction();

    if (!owner)
        return;

    if (m_activeRow->role() == RowRole::Denominator)
    {
        m_activeRow = owner->numeratorRow();
        m_cursorPosition = m_activeRow->childCount();
    }
    else
    {
        moveToParent(false);
    }
}

void EquationWidget::moveCursorRight()
{
    if (m_cursorPosition < m_activeRow->childCount())
    {
        MathNode *nextNode = m_activeRow->childAt(m_cursorPosition);

        // Si encontramos una fracción, entramos por
        // su extremo izquierdo: el numerador.
        if (auto *fraction = dynamic_cast<FractionNode *>(nextNode))
        {
            m_activeRow = fraction->numeratorRow();

            m_cursorPosition = 0;
            return;
        }

        ++m_cursorPosition;
        return;
    }

    FractionNode *owner = m_activeRow->ownerFraction();

    if (!owner)
        return;

    if (m_activeRow->role() == RowRole::Numerator)
    {
        m_activeRow = owner->denominatorRow();

        m_cursorPosition = 0;
    }
    else
    {
        moveToParent(true);
    }
}

void EquationWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), palette().color(QPalette::Base));

    painter.setPen(palette().color(QPalette::Text));

    QFont mathFont = font();
    mathFont.setPointSize(24);
    painter.setFont(mathFont);

    const QFontMetricsF metrics(mathFont);
    const NodeLayout rootLayout = m_rootNode->layout(metrics);

    const QPointF rootPosition(24.0, (height() - rootLayout.size.height()) / 2.0);

    m_rootNode->draw(painter, rootPosition, metrics);

    if (!hasFocus())
        return;

    QPointF activeRowPosition;

    const bool rowFound = findRowPosition(m_rootNode.get(), m_activeRow, rootPosition, metrics, activeRowPosition);

    if (!rowFound)
        return;

    const NodeLayout activeLayout = m_activeRow->layout(metrics);

    const qreal cursorX = activeRowPosition.x() + m_activeRow->cursorOffset(m_cursorPosition, metrics);

    // El cursor conserva el tamaño normal del texto,
    // aunque la fila contenga una fracción grande.
    const qreal cursorTop = activeRowPosition.y() + activeLayout.baseline - metrics.ascent();

    painter.drawLine(QPointF(cursorX, cursorTop), QPointF(cursorX, cursorTop + metrics.height()));
}

void EquationWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Left:
        moveCursorLeft();
        break;

    case Qt::Key_Right:
        moveCursorRight();
        break;

    case Qt::Key_Up: {
        FractionNode *owner = m_activeRow->ownerFraction();

        if (owner && m_activeRow->role() == RowRole::Denominator)
        {
            m_activeRow = owner->numeratorRow();

            m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
        }
        break;
    }

    case Qt::Key_Down: {
        FractionNode *owner = m_activeRow->ownerFraction();

        if (owner && m_activeRow->role() == RowRole::Numerator)
        {
            m_activeRow = owner->denominatorRow();

            m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
        }
        break;
    }

    case Qt::Key_Home:
        m_cursorPosition = 0;
        break;

    case Qt::Key_End:
        m_cursorPosition = m_activeRow->childCount();
        break;

    case Qt::Key_Backspace:
        if (m_cursorPosition > 0)
        {
            m_activeRow->removeNode(m_cursorPosition - 1);

            --m_cursorPosition;
        }
        else
        {
            moveCursorLeft();
        }
        break;

    case Qt::Key_Delete:
        if (m_cursorPosition < m_activeRow->childCount())
        {
            m_activeRow->removeNode(m_cursorPosition);
        }
        break;

    case Qt::Key_Tab: {
        FractionNode *owner = m_activeRow->ownerFraction();

        if (!owner)
            break;

        if (m_activeRow->role() == RowRole::Numerator)
        {
            m_activeRow = owner->denominatorRow();

            m_cursorPosition = 0;
        }
        else
        {
            moveToParent(true);
        }
        break;
    }

    case Qt::Key_Slash:
        insertFraction();
        return;

    default: {
        const QString input = event->text();

        if (!input.isEmpty() && input.front().isPrint())
        {
            m_activeRow->insertNode(m_cursorPosition, std::make_unique<TextNode>(input));

            ++m_cursorPosition;
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
