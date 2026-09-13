#include "equationwidget.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

// ---------------------------------------------------------------------------
// Funciones auxiliares (namespace anónimo)
// ---------------------------------------------------------------------------

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

        if (const auto *fraction = dynamic_cast<const FractionNode *>(child))
        {
            const QPointF numeratorPosition = fraction->numeratorPosition(childPosition, metrics);

            if (findRowPosition(fraction->numeratorRow(), targetRow, numeratorPosition, metrics, result))
                return true;

            const QPointF denominatorPosition = fraction->denominatorPosition(childPosition, metrics);

            if (findRowPosition(fraction->denominatorRow(), targetRow, denominatorPosition, metrics, result))
                return true;
        }

        const auto *root = dynamic_cast<const RootNode *>(child);

        if (root)
        {
            const QPointF radicandPosition = root->radicandPosition(childPosition, metrics);

            if (findRowPosition(root->radicandRow(), targetRow, radicandPosition, metrics, result))
            {
                return true;
            }
        }

        currentX += childLayout.size.width();
    }

    return false;
}

bool hitTestRow(RowNode *row, const QPointF &rowPosition, const QPointF &mousePosition, const QFontMetricsF &metrics,
                RowNode *&selectedRow, QPointF &selectedRowPosition)
{
    const NodeLayout rowLayout = row->layout(metrics);
    qreal currentX = rowPosition.x();

    for (qsizetype index = 0; index < row->childCount(); ++index)
    {
        MathNode *child = row->childAt(index);
        const NodeLayout childLayout = child->layout(metrics);
        const QPointF childPosition(currentX, rowPosition.y() + rowLayout.baseline - childLayout.baseline);

        if (auto *fraction = dynamic_cast<FractionNode *>(child))
        {
            QRectF fractionRectangle(childPosition, childLayout.size);

            // Ampliamos un poco el área para que sea más fácil hacer clic en la fracción.
            fractionRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (fractionRectangle.contains(mousePosition))
            {
                const NodeLayout numeratorLayout = fraction->numeratorRow()->layout(metrics);
                const qreal divisionLineY = childPosition.y() + numeratorLayout.size.height() + 4.0;

                RowNode *targetRow = nullptr;
                QPointF targetPosition;

                if (mousePosition.y() < divisionLineY)
                {
                    targetRow = fraction->numeratorRow();
                    targetPosition = fraction->numeratorPosition(childPosition, metrics);
                }
                else
                {
                    targetRow = fraction->denominatorRow();
                    targetPosition = fraction->denominatorPosition(childPosition, metrics);
                }

                // Buscamos otra fracción dentro de la fila seleccionada.
                return hitTestRow(targetRow, targetPosition, mousePosition, metrics, selectedRow, selectedRowPosition);
            }
        }

        auto *root = dynamic_cast<RootNode *>(child);

        if (root)
        {
            QRectF rootRectangle(childPosition, childLayout.size);

            rootRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (rootRectangle.contains(mousePosition))
            {
                const QPointF radicandPosition = root->radicandPosition(childPosition, metrics);

                return hitTestRow(root->radicandRow(), radicandPosition, mousePosition, metrics, selectedRow,
                                  selectedRowPosition);
            }
        }

        currentX += childLayout.size.width();
    }

    selectedRow = row;
    selectedRowPosition = rowPosition;
    return true;
}

qsizetype closestCursorPosition(const RowNode *row, const QPointF &rowPosition, qreal mouseX,
                                const QFontMetricsF &metrics)
{
    qreal currentX = rowPosition.x();

    for (qsizetype index = 0; index < row->childCount(); ++index)
    {
        const MathNode *child = row->childAt(index);
        const qreal childWidth = child->layout(metrics).size.width();
        const qreal childMiddle = currentX + childWidth / 2.0;

        if (mouseX < childMiddle)
            return index;

        currentX += childWidth;
    }

    return row->childCount();
}

} // namespace

// ---------------------------------------------------------------------------
// Construcción
// ---------------------------------------------------------------------------

EquationWidget::EquationWidget(QWidget *parent)
    : QWidget(parent), m_rootNode(std::make_unique<RowNode>()), m_activeRow(m_rootNode.get())
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(240);
    setCursor(Qt::IBeamCursor);
    m_cursorTimer.setInterval(500);

    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
}

// ---------------------------------------------------------------------------
// Acciones públicas
// ---------------------------------------------------------------------------

void EquationWidget::insertRoot()
{
    auto root = std::make_unique<RootNode>();
    RootNode *rootPointer = root.get();

    m_activeRow->insertNode(m_cursorPosition, std::move(root));

    m_activeRow = rootPointer->radicandRow();
    m_cursorPosition = 0;

    setFocus();
    resetCursorBlink();
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

// ---------------------------------------------------------------------------
// Eventos de Qt
// ---------------------------------------------------------------------------

void EquationWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    QFont mathFont = font();
    mathFont.setPointSize(24);
    painter.setFont(mathFont);

    const QFontMetricsF metrics(mathFont);
    const NodeLayout rootLayout = m_rootNode->layout(metrics);
    const QPointF rootPosition(24.0, (height() - rootLayout.size.height()) / 2.0);

    QPointF activeRowPosition;
    const bool rowFound = findRowPosition(m_rootNode.get(), m_activeRow, rootPosition, metrics, activeRowPosition);

    // Dibuja primero el fondo de la fila activa, para que quede detrás de la ecuación.
    if (hasFocus() && rowFound)
    {
        const NodeLayout activeLayout = m_activeRow->layout(metrics);

        QRectF activeRectangle(activeRowPosition, activeLayout.size);
        activeRectangle.adjust(-4.0, -2.0, 4.0, 2.0);

        QColor highlightColor = palette().color(QPalette::Highlight);
        highlightColor.setAlpha(35);

        painter.setPen(Qt::NoPen);
        painter.setBrush(highlightColor);
        painter.drawRoundedRect(activeRectangle, 4.0, 4.0);
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(palette().color(QPalette::Text));
    m_rootNode->draw(painter, rootPosition, metrics);

    if (!hasFocus() || !m_cursorVisible || !rowFound)
        return;

    const NodeLayout activeLayout = m_activeRow->layout(metrics);
    const qreal cursorX = activeRowPosition.x() + m_activeRow->cursorOffset(m_cursorPosition, metrics);
    const qreal cursorTop = activeRowPosition.y() + activeLayout.baseline - metrics.ascent();

    QPen cursorPen(palette().color(QPalette::Text));
    cursorPen.setWidthF(1.5);
    painter.setPen(cursorPen);
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
            m_activeRow->removeNode(m_cursorPosition);
        break;

    case Qt::Key_Tab: {
        FractionNode *owner = m_activeRow->ownerFraction();

        if (!owner)
            break;

        if (owner)
        {
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
        else if (m_activeRow->ownerRoot())
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

    resetCursorBlink();
}

void EquationWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    setFocus();

    QFont mathFont = font();
    mathFont.setPointSize(24);

    const QFontMetricsF metrics(mathFont);
    const NodeLayout rootLayout = m_rootNode->layout(metrics);
    const QPointF rootPosition(24.0, (height() - rootLayout.size.height()) / 2.0);

    RowNode *selectedRow = nullptr;
    QPointF selectedRowPosition;

    hitTestRow(m_rootNode.get(), rootPosition, event->position(), metrics, selectedRow, selectedRowPosition);

    if (selectedRow)
    {
        m_activeRow = selectedRow;
        m_cursorPosition = closestCursorPosition(selectedRow, selectedRowPosition, event->position().x(), metrics);
    }

    resetCursorBlink();
}

void EquationWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    resetCursorBlink();
}

void EquationWidget::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);

    m_cursorTimer.stop();
    m_cursorVisible = false;
    update();
}

// ---------------------------------------------------------------------------
// Navegación del cursor
// ---------------------------------------------------------------------------

void EquationWidget::moveCursorLeft()
{
    if (m_cursorPosition > 0)
    {
        MathNode *previousNode = m_activeRow->childAt(m_cursorPosition - 1);

        // Si encontramos una fracción, entramos por su extremo derecho: el denominador.
        if (auto *fraction = dynamic_cast<FractionNode *>(previousNode))
        {
            m_activeRow = fraction->denominatorRow();
            m_cursorPosition = m_activeRow->childCount();
            return;
        }

        if (auto *root = dynamic_cast<RootNode *>(previousNode))
        {
            m_activeRow = root->radicandRow();
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
    if (m_activeRow->ownerRoot())
        moveToParent(false);
}

void EquationWidget::moveCursorRight()
{
    if (m_cursorPosition < m_activeRow->childCount())
    {
        MathNode *nextNode = m_activeRow->childAt(m_cursorPosition);

        // Si encontramos una fracción, entramos por su extremo izquierdo: el numerador.
        if (auto *fraction = dynamic_cast<FractionNode *>(nextNode))
        {
            m_activeRow = fraction->numeratorRow();
            m_cursorPosition = 0;
            return;
        }

        if (auto *root = dynamic_cast<RootNode *>(nextNode))
        {
            m_activeRow = root->radicandRow();
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

    if (m_activeRow->ownerRoot())
        moveToParent(true);
}

void EquationWidget::moveToParent(bool placeAfterStructure)
{
    RowNode *parent = nullptr;
    MathNode *structure = nullptr;

    if (FractionNode *fraction = m_activeRow->ownerFraction())
    {
        parent = fraction->parentRow();
        structure = fraction;
    }
    else if (RootNode *root = m_activeRow->ownerRoot())
    {
        parent = root->parentRow();
        structure = root;
    }

    if (!parent || !structure)
        return;

    const qsizetype structureIndex = parent->indexOf(structure);

    if (structureIndex < 0)
        return;

    m_activeRow = parent;

    m_cursorPosition = placeAfterStructure ? structureIndex + 1 : structureIndex;
}

void EquationWidget::resetCursorBlink()
{
    m_cursorVisible = true;

    if (hasFocus())
        m_cursorTimer.start();

    update();
}

QString EquationWidget::toLatex() const
{
    return m_rootNode->toLatex();
}
