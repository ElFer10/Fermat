#include "equationwidget.h"
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMarginsF>
#include <QMouseEvent>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPalette>
#include <QPdfWriter>
#include <QRectF>
#include <QSvgGenerator>
#include <QtMath>

namespace
{

bool findRowPosition(const RowNode *currentRow, const RowNode *targetRow, const QPointF &currentPosition,
                     const QFont &currentFont, const QFontMetricsF &metrics, QPointF &result, QFont &resultFont)
{
    if (currentRow == targetRow)
    {
        result = currentPosition;
        resultFont = currentFont;
        return true;
    }

    const NodeLayout rowLayout = currentRow->layout(metrics);

    qreal currentX = currentPosition.x();

    for (qsizetype index = 0; index < currentRow->childCount(); ++index)
    {
        const MathNode *child = currentRow->childAt(index);

        if (!child)
            continue;

        const NodeLayout childLayout = child->layout(metrics);

        const QPointF childPosition(currentX, currentPosition.y() + rowLayout.baseline - childLayout.baseline);

        if (const auto *fraction = dynamic_cast<const FractionNode *>(child))
        {
            if (findRowPosition(fraction->numeratorRow(), targetRow,
                                fraction->numeratorPosition(childPosition, metrics), currentFont, metrics, result,
                                resultFont))
            {
                return true;
            }

            if (findRowPosition(fraction->denominatorRow(), targetRow,
                                fraction->denominatorPosition(childPosition, metrics), currentFont, metrics, result,
                                resultFont))
            {
                return true;
            }
        }

        if (const auto *root = dynamic_cast<const RootNode *>(child))
        {
            if (findRowPosition(root->radicandRow(), targetRow, root->radicandPosition(childPosition, metrics),
                                currentFont, metrics, result, resultFont))
            {
                return true;
            }
        }

        if (const auto *script = dynamic_cast<const ScriptNode *>(child))
        {
            if (findRowPosition(script->baseRow(), targetRow, script->basePosition(childPosition, metrics), currentFont,
                                metrics, result, resultFont))
            {
                return true;
            }

            const QFont smallerFont = script->superscriptFont(metrics);

            const QFontMetricsF smallerMetrics(smallerFont);

            if (script->hasSuperscript())
            {
                if (findRowPosition(script->superscriptRow(), targetRow,
                                    script->superscriptPosition(childPosition, metrics), smallerFont, smallerMetrics,
                                    result, resultFont))
                {
                    return true;
                }
            }

            if (script->hasSubscript())
            {
                if (findRowPosition(script->subscriptRow(), targetRow,
                                    script->subscriptPosition(childPosition, metrics), smallerFont, smallerMetrics,
                                    result, resultFont))
                {
                    return true;
                }
            }
        }

        if (const auto *largeOperator = dynamic_cast<const LargeOperatorNode *>(child))
        {
            const QFont smallFont = largeOperator->limitFont(metrics);

            const QFontMetricsF smallMetrics(smallFont);

            if (findRowPosition(largeOperator->lowerLimitRow(), targetRow,
                                largeOperator->lowerLimitPosition(childPosition, metrics), smallFont, smallMetrics,
                                result, resultFont))
            {
                return true;
            }

            if (findRowPosition(largeOperator->upperLimitRow(), targetRow,
                                largeOperator->upperLimitPosition(childPosition, metrics), smallFont, smallMetrics,
                                result, resultFont))
            {
                return true;
            }

            if (findRowPosition(largeOperator->bodyRow(), targetRow,
                                largeOperator->bodyPosition(childPosition, metrics), currentFont, metrics, result,
                                resultFont))
            {
                return true;
            }
        }

        currentX += childLayout.size.width();
    }

    return false;
}

bool hitTestRow(RowNode *row, const QPointF &rowPosition, const QPointF &mousePosition, const QFont &currentFont,
                const QFontMetricsF &metrics, RowNode *&selectedRow, QPointF &selectedRowPosition, QFont &selectedFont)
{
    const NodeLayout rowLayout = row->layout(metrics);

    qreal currentX = rowPosition.x();

    for (qsizetype index = 0; index < row->childCount(); ++index)
    {
        MathNode *child = row->childAt(index);

        if (!child)
            continue;

        const NodeLayout childLayout = child->layout(metrics);

        const QPointF childPosition(currentX, rowPosition.y() + rowLayout.baseline - childLayout.baseline);

        if (auto *fraction = dynamic_cast<FractionNode *>(child))
        {
            QRectF fractionRectangle(childPosition, childLayout.size);

            fractionRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (fractionRectangle.contains(mousePosition))
            {
                const NodeLayout numeratorLayout = fraction->numeratorRow()->layout(metrics);

                const qreal divisionLineY = childPosition.y() + numeratorLayout.size.height() + 4.0;

                if (mousePosition.y() < divisionLineY)
                {
                    return hitTestRow(fraction->numeratorRow(), fraction->numeratorPosition(childPosition, metrics),
                                      mousePosition, currentFont, metrics, selectedRow, selectedRowPosition,
                                      selectedFont);
                }

                return hitTestRow(fraction->denominatorRow(), fraction->denominatorPosition(childPosition, metrics),
                                  mousePosition, currentFont, metrics, selectedRow, selectedRowPosition, selectedFont);
            }
        }

        if (auto *root = dynamic_cast<RootNode *>(child))
        {
            QRectF rootRectangle(childPosition, childLayout.size);

            rootRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (rootRectangle.contains(mousePosition))
            {
                return hitTestRow(root->radicandRow(), root->radicandPosition(childPosition, metrics), mousePosition,
                                  currentFont, metrics, selectedRow, selectedRowPosition, selectedFont);
            }
        }

        if (auto *script = dynamic_cast<ScriptNode *>(child))
        {
            QRectF scriptRectangle(childPosition, childLayout.size);

            scriptRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (scriptRectangle.contains(mousePosition))
            {
                const QPointF basePosition = script->basePosition(childPosition, metrics);

                const QFont smallerFont = script->superscriptFont(metrics);

                const QFontMetricsF smallerMetrics(smallerFont);

                if (script->hasSubscript())
                {
                    const QPointF lowerPosition = script->subscriptPosition(childPosition, metrics);

                    const NodeLayout lowerLayout = script->subscriptRow()->layout(smallerMetrics);

                    QRectF lowerRectangle(lowerPosition, lowerLayout.size);

                    lowerRectangle.adjust(-3.0, -3.0, 3.0, 3.0);

                    if (lowerRectangle.contains(mousePosition))
                    {
                        return hitTestRow(script->subscriptRow(), lowerPosition, mousePosition, smallerFont,
                                          smallerMetrics, selectedRow, selectedRowPosition, selectedFont);
                    }
                }

                if (script->hasSuperscript())
                {
                    const QPointF upperPosition = script->superscriptPosition(childPosition, metrics);

                    const NodeLayout upperLayout = script->superscriptRow()->layout(smallerMetrics);

                    QRectF upperRectangle(upperPosition, upperLayout.size);

                    upperRectangle.adjust(-3.0, -3.0, 3.0, 3.0);

                    if (upperRectangle.contains(mousePosition))
                    {
                        return hitTestRow(script->superscriptRow(), upperPosition, mousePosition, smallerFont,
                                          smallerMetrics, selectedRow, selectedRowPosition, selectedFont);
                    }
                }

                return hitTestRow(script->baseRow(), basePosition, mousePosition, currentFont, metrics, selectedRow,
                                  selectedRowPosition, selectedFont);
            }
        }

        if (auto *largeOperator = dynamic_cast<LargeOperatorNode *>(child))
        {
            QRectF operatorRectangle(childPosition, childLayout.size);

            operatorRectangle.adjust(-4.0, -4.0, 4.0, 4.0);

            if (operatorRectangle.contains(mousePosition))
            {
                const QFont smallFont = largeOperator->limitFont(metrics);

                const QFontMetricsF smallMetrics(smallFont);

                const QPointF lowerPosition = largeOperator->lowerLimitPosition(childPosition, metrics);

                const NodeLayout lowerLayout = largeOperator->lowerLimitRow()->layout(smallMetrics);

                QRectF lowerRectangle(lowerPosition, lowerLayout.size);

                lowerRectangle.adjust(-3.0, -3.0, 3.0, 3.0);

                if (lowerRectangle.contains(mousePosition))
                {
                    return hitTestRow(largeOperator->lowerLimitRow(), lowerPosition, mousePosition, smallFont,
                                      smallMetrics, selectedRow, selectedRowPosition, selectedFont);
                }

                const QPointF upperPosition = largeOperator->upperLimitPosition(childPosition, metrics);

                const NodeLayout upperLayout = largeOperator->upperLimitRow()->layout(smallMetrics);

                QRectF upperRectangle(upperPosition, upperLayout.size);

                upperRectangle.adjust(-3.0, -3.0, 3.0, 3.0);

                if (upperRectangle.contains(mousePosition))
                {
                    return hitTestRow(largeOperator->upperLimitRow(), upperPosition, mousePosition, smallFont,
                                      smallMetrics, selectedRow, selectedRowPosition, selectedFont);
                }

                return hitTestRow(largeOperator->bodyRow(), largeOperator->bodyPosition(childPosition, metrics),
                                  mousePosition, currentFont, metrics, selectedRow, selectedRowPosition, selectedFont);
            }
        }

        currentX += childLayout.size.width();
    }

    selectedRow = row;
    selectedRowPosition = rowPosition;
    selectedFont = currentFont;

    return true;
}

qsizetype closestCursorPosition(const RowNode *row, const QPointF &rowPosition, qreal mouseX,
                                const QFontMetricsF &metrics)
{
    qreal currentX = rowPosition.x();

    for (qsizetype index = 0; index < row->childCount(); ++index)
    {
        const MathNode *child = row->childAt(index);

        if (!child)
            continue;

        const qreal childWidth = child->layout(metrics).size.width();

        const qreal childMiddle = currentX + childWidth / 2.0;

        if (mouseX < childMiddle)
            return index;

        currentX += childWidth;
    }

    return row->childCount();
}

} // namespace

// ============================================================
// Constructor
// ============================================================

EquationWidget::EquationWidget(QWidget *parent)
    : QWidget(parent), m_rootNode(std::make_unique<RowNode>()), m_activeRow(m_rootNode.get())
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(300);
    setCursor(Qt::IBeamCursor);

    m_mathStyle.baseFont = font();
    m_mathStyle.baseFont.setPointSize(24);
    m_mathStyle.baseFont.setItalic(false);

    m_mathStyle.italicVariables = true;

    m_rootNode->setMathStyle(m_mathStyle);

    m_cursorTimer.setInterval(500);

    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;

        update();
    });
}
// ============================================================
// Exportación
// ============================================================

QString EquationWidget::toLatex() const
{
    return m_rootNode->toLatex();
}

// ============================================================
// Inserción de estructuras
// ============================================================

void EquationWidget::insertFraction()
{
    auto fraction = std::make_unique<FractionNode>();

    FractionNode *fractionPointer = fraction.get();

    m_activeRow->insertNode(m_cursorPosition, std::move(fraction));

    m_activeRow = fractionPointer->numeratorRow();

    m_cursorPosition = 0;

    setFocus();
    resetCursorBlink();
}

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

void EquationWidget::insertSuperscript()
{
    if (m_cursorPosition > 0)
    {
        MathNode *previousNode = m_activeRow->childAt(m_cursorPosition - 1);

        if (auto *existingScript = dynamic_cast<ScriptNode *>(previousNode))
        {
            existingScript->enableSuperscript();

            m_activeRow = existingScript->superscriptRow();

            m_cursorPosition = m_activeRow->childCount();

            setFocus();
            resetCursorBlink();
            return;
        }
    }

    auto script = std::make_unique<ScriptNode>(true, false);

    ScriptNode *scriptPointer = script.get();

    if (m_cursorPosition > 0)
    {
        const qsizetype insertionPosition = m_cursorPosition - 1;

        std::unique_ptr<MathNode> baseNode = m_activeRow->takeNode(insertionPosition);

        scriptPointer->baseRow()->appendNode(std::move(baseNode));

        m_activeRow->insertNode(insertionPosition, std::move(script));

        m_activeRow = scriptPointer->superscriptRow();

        m_cursorPosition = 0;
    }
    else
    {
        m_activeRow->insertNode(m_cursorPosition, std::move(script));

        m_activeRow = scriptPointer->baseRow();

        m_cursorPosition = 0;
    }

    setFocus();
    resetCursorBlink();
}

void EquationWidget::insertSubscript()
{
    if (m_cursorPosition > 0)
    {
        MathNode *previousNode = m_activeRow->childAt(m_cursorPosition - 1);

        if (auto *existingScript = dynamic_cast<ScriptNode *>(previousNode))
        {
            existingScript->enableSubscript();

            m_activeRow = existingScript->subscriptRow();

            m_cursorPosition = m_activeRow->childCount();

            setFocus();
            resetCursorBlink();
            return;
        }
    }

    auto script = std::make_unique<ScriptNode>(false, true);

    ScriptNode *scriptPointer = script.get();

    if (m_cursorPosition > 0)
    {
        const qsizetype insertionPosition = m_cursorPosition - 1;

        std::unique_ptr<MathNode> baseNode = m_activeRow->takeNode(insertionPosition);

        scriptPointer->baseRow()->appendNode(std::move(baseNode));

        m_activeRow->insertNode(insertionPosition, std::move(script));

        m_activeRow = scriptPointer->subscriptRow();

        m_cursorPosition = 0;
    }
    else
    {
        m_activeRow->insertNode(m_cursorPosition, std::move(script));

        m_activeRow = scriptPointer->baseRow();

        m_cursorPosition = 0;
    }

    setFocus();
    resetCursorBlink();
}

void EquationWidget::insertLargeOperator(LargeOperatorType type)
{
    auto largeOperator = std::make_unique<LargeOperatorNode>(type);

    LargeOperatorNode *operatorPointer = largeOperator.get();

    m_activeRow->insertNode(m_cursorPosition, std::move(largeOperator));

    m_activeRow = operatorPointer->lowerLimitRow();

    m_cursorPosition = 0;

    setFocus();
    resetCursorBlink();
}

void EquationWidget::insertIntegral()
{
    insertLargeOperator(LargeOperatorType::Integral);
}

void EquationWidget::insertSummation()
{
    insertLargeOperator(LargeOperatorType::Summation);
}

void EquationWidget::clearEquation()
{
    m_rootNode = std::make_unique<RowNode>();

    m_activeRow = m_rootNode.get();
    m_cursorPosition = 0;

    m_rootNode->setMathStyle(m_mathStyle);

    setFocus();
    resetCursorBlink();
}

// ============================================================
// Cursor
// ============================================================

void EquationWidget::resetCursorBlink()
{
    m_cursorVisible = true;

    if (hasFocus())
        m_cursorTimer.start();

    update();
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
    else if (ScriptNode *script = m_activeRow->ownerScript())
    {
        parent = script->parentRow();
        structure = script;
    }
    else if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
    {
        parent = largeOperator->parentRow();
        structure = largeOperator;
    }

    if (!parent || !structure)
        return;

    const qsizetype structureIndex = parent->indexOf(structure);

    if (structureIndex < 0)
        return;

    m_activeRow = parent;

    m_cursorPosition = placeAfterStructure ? structureIndex + 1 : structureIndex;
}

void EquationWidget::moveCursorLeft()
{
    if (m_cursorPosition > 0)
    {
        MathNode *previousNode = m_activeRow->childAt(m_cursorPosition - 1);

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

        if (auto *script = dynamic_cast<ScriptNode *>(previousNode))
        {
            if (script->hasSuperscript())
            {
                m_activeRow = script->superscriptRow();
            }
            else if (script->hasSubscript())
            {
                m_activeRow = script->subscriptRow();
            }
            else
            {
                m_activeRow = script->baseRow();
            }

            m_cursorPosition = m_activeRow->childCount();

            return;
        }

        if (auto *largeOperator = dynamic_cast<LargeOperatorNode *>(previousNode))
        {
            m_activeRow = largeOperator->bodyRow();

            m_cursorPosition = m_activeRow->childCount();

            return;
        }

        --m_cursorPosition;
        return;
    }

    if (FractionNode *fraction = m_activeRow->ownerFraction())
    {
        if (m_activeRow->role() == RowRole::Denominator)
        {
            m_activeRow = fraction->numeratorRow();

            m_cursorPosition = m_activeRow->childCount();
        }
        else
        {
            moveToParent(false);
        }

        return;
    }

    if (m_activeRow->ownerRoot())
    {
        moveToParent(false);
        return;
    }

    if (ScriptNode *script = m_activeRow->ownerScript())
    {
        if (m_activeRow->role() == RowRole::Superscript)
        {
            if (script->hasSubscript())
            {
                m_activeRow = script->subscriptRow();
            }
            else
            {
                m_activeRow = script->baseRow();
            }

            m_cursorPosition = m_activeRow->childCount();
        }
        else if (m_activeRow->role() == RowRole::Subscript)
        {
            m_activeRow = script->baseRow();

            m_cursorPosition = m_activeRow->childCount();
        }
        else
        {
            moveToParent(false);
        }

        return;
    }

    if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
    {
        if (m_activeRow->role() == RowRole::OperatorBody)
        {
            m_activeRow = largeOperator->upperLimitRow();

            m_cursorPosition = m_activeRow->childCount();
        }
        else if (m_activeRow->role() == RowRole::UpperLimit)
        {
            m_activeRow = largeOperator->lowerLimitRow();

            m_cursorPosition = m_activeRow->childCount();
        }
        else
        {
            moveToParent(false);
        }
    }
}

void EquationWidget::moveCursorRight()
{
    if (m_cursorPosition < m_activeRow->childCount())
    {
        MathNode *nextNode = m_activeRow->childAt(m_cursorPosition);

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

        if (auto *script = dynamic_cast<ScriptNode *>(nextNode))
        {
            m_activeRow = script->baseRow();

            m_cursorPosition = 0;
            return;
        }

        if (auto *largeOperator = dynamic_cast<LargeOperatorNode *>(nextNode))
        {
            m_activeRow = largeOperator->lowerLimitRow();

            m_cursorPosition = 0;
            return;
        }

        ++m_cursorPosition;
        return;
    }

    if (FractionNode *fraction = m_activeRow->ownerFraction())
    {
        if (m_activeRow->role() == RowRole::Numerator)
        {
            m_activeRow = fraction->denominatorRow();

            m_cursorPosition = 0;
        }
        else
        {
            moveToParent(true);
        }

        return;
    }

    if (m_activeRow->ownerRoot())
    {
        moveToParent(true);
        return;
    }

    if (ScriptNode *script = m_activeRow->ownerScript())
    {
        if (m_activeRow->role() == RowRole::ScriptBase)
        {
            if (script->hasSubscript())
            {
                m_activeRow = script->subscriptRow();

                m_cursorPosition = 0;
            }
            else if (script->hasSuperscript())
            {
                m_activeRow = script->superscriptRow();

                m_cursorPosition = 0;
            }
            else
            {
                moveToParent(true);
            }
        }
        else if (m_activeRow->role() == RowRole::Subscript)
        {
            if (script->hasSuperscript())
            {
                m_activeRow = script->superscriptRow();

                m_cursorPosition = 0;
            }
            else
            {
                moveToParent(true);
            }
        }
        else
        {
            moveToParent(true);
        }

        return;
    }

    if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
    {
        if (m_activeRow->role() == RowRole::LowerLimit)
        {
            m_activeRow = largeOperator->upperLimitRow();

            m_cursorPosition = 0;
        }
        else if (m_activeRow->role() == RowRole::UpperLimit)
        {
            m_activeRow = largeOperator->bodyRow();

            m_cursorPosition = 0;
        }
        else
        {
            moveToParent(true);
        }
    }
}

// ============================================================
// Dibujo
// ============================================================

void EquationWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), palette().color(QPalette::Base));

    const QFont mathFont = m_mathStyle.baseFont;

    painter.setFont(mathFont);

    const QFontMetricsF metrics(mathFont);

    const NodeLayout rootLayout = m_rootNode->layout(metrics);

    const QPointF rootPosition(24.0, (height() - rootLayout.size.height()) / 2.0);

    QPointF activeRowPosition;
    QFont activeFont = mathFont;

    const bool rowFound =
        findRowPosition(m_rootNode.get(), m_activeRow, rootPosition, mathFont, metrics, activeRowPosition, activeFont);

    const QFontMetricsF activeMetrics(activeFont);

    if (hasFocus() && rowFound)
    {
        const NodeLayout activeLayout = m_activeRow->layout(activeMetrics);

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

    painter.setFont(mathFont);

    m_rootNode->draw(painter, rootPosition, metrics);

    if (!hasFocus() || !m_cursorVisible || !rowFound)
    {
        return;
    }

    const NodeLayout activeLayout = m_activeRow->layout(activeMetrics);

    const qreal cursorX = activeRowPosition.x() + m_activeRow->cursorOffset(m_cursorPosition, activeMetrics);

    const qreal cursorTop = activeRowPosition.y() + activeLayout.baseline - activeMetrics.ascent();

    QPen cursorPen(palette().color(QPalette::Text));

    cursorPen.setWidthF(1.5);
    painter.setPen(cursorPen);

    painter.drawLine(QPointF(cursorX, cursorTop), QPointF(cursorX, cursorTop + activeMetrics.height()));
}

// ============================================================
// Teclado
// ============================================================

void EquationWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->text() == QStringLiteral("^"))
    {
        insertSuperscript();
        return;
    }

    if (event->text() == QStringLiteral("_"))
    {
        insertSubscript();
        return;
    }

    switch (event->key())
    {
    case Qt::Key_Left:
        moveCursorLeft();
        break;

    case Qt::Key_Right:
        moveCursorRight();
        break;

    case Qt::Key_Up: {
        if (FractionNode *fraction = m_activeRow->ownerFraction())
        {
            if (m_activeRow->role() == RowRole::Denominator)
            {
                m_activeRow = fraction->numeratorRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
        }
        else if (ScriptNode *script = m_activeRow->ownerScript())
        {
            if (m_activeRow->role() == RowRole::Subscript)
            {
                if (script->hasSuperscript())
                {
                    m_activeRow = script->superscriptRow();
                }
                else
                {
                    m_activeRow = script->baseRow();
                }

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
            else if (m_activeRow->role() == RowRole::ScriptBase && script->hasSuperscript())
            {
                m_activeRow = script->superscriptRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
        }
        else if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
        {
            if (m_activeRow->role() == RowRole::LowerLimit)
            {
                m_activeRow = largeOperator->upperLimitRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
        }

        break;
    }

    case Qt::Key_Down: {
        if (FractionNode *fraction = m_activeRow->ownerFraction())
        {
            if (m_activeRow->role() == RowRole::Numerator)
            {
                m_activeRow = fraction->denominatorRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
        }
        else if (ScriptNode *script = m_activeRow->ownerScript())
        {
            if (m_activeRow->role() == RowRole::Superscript)
            {
                if (script->hasSubscript())
                {
                    m_activeRow = script->subscriptRow();
                }
                else
                {
                    m_activeRow = script->baseRow();
                }

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
            else if (m_activeRow->role() == RowRole::ScriptBase && script->hasSubscript())
            {
                m_activeRow = script->subscriptRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
        }
        else if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
        {
            if (m_activeRow->role() == RowRole::UpperLimit)
            {
                m_activeRow = largeOperator->lowerLimitRow();

                m_cursorPosition = qMin(m_cursorPosition, m_activeRow->childCount());
            }
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
        if (FractionNode *fraction = m_activeRow->ownerFraction())
        {
            if (m_activeRow->role() == RowRole::Numerator)
            {
                m_activeRow = fraction->denominatorRow();

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
        else if (ScriptNode *script = m_activeRow->ownerScript())
        {
            if (m_activeRow->role() == RowRole::ScriptBase)
            {
                if (script->hasSubscript())
                {
                    m_activeRow = script->subscriptRow();

                    m_cursorPosition = 0;
                }
                else if (script->hasSuperscript())
                {
                    m_activeRow = script->superscriptRow();

                    m_cursorPosition = 0;
                }
                else
                {
                    moveToParent(true);
                }
            }
            else if (m_activeRow->role() == RowRole::Subscript && script->hasSuperscript())
            {
                m_activeRow = script->superscriptRow();

                m_cursorPosition = 0;
            }
            else
            {
                moveToParent(true);
            }
        }
        else if (LargeOperatorNode *largeOperator = m_activeRow->ownerOperator())
        {
            if (m_activeRow->role() == RowRole::LowerLimit)
            {
                m_activeRow = largeOperator->upperLimitRow();

                m_cursorPosition = 0;
            }
            else if (m_activeRow->role() == RowRole::UpperLimit)
            {
                m_activeRow = largeOperator->bodyRow();

                m_cursorPosition = 0;
            }
            else
            {
                moveToParent(true);
            }
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

// ============================================================
// Ratón
// ============================================================

void EquationWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    setFocus();

    const QFont mathFont = m_mathStyle.baseFont;

    const QFontMetricsF metrics(mathFont);

    const NodeLayout rootLayout = m_rootNode->layout(metrics);

    const QPointF rootPosition(24.0, (height() - rootLayout.size.height()) / 2.0);

    RowNode *selectedRow = nullptr;
    QPointF selectedRowPosition;
    QFont selectedFont = mathFont;

    hitTestRow(m_rootNode.get(), rootPosition, event->position(), mathFont, metrics, selectedRow, selectedRowPosition,
               selectedFont);

    if (selectedRow)
    {
        const QFontMetricsF selectedMetrics(selectedFont);

        m_activeRow = selectedRow;

        m_cursorPosition =
            closestCursorPosition(selectedRow, selectedRowPosition, event->position().x(), selectedMetrics);
    }

    resetCursorBlink();
}

// ============================================================
// Foco
// ============================================================

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

void EquationWidget::applyMathStyle()
{
    m_rootNode->setMathStyle(m_mathStyle);

    update();
}

void EquationWidget::setMathFont(const QFont &font)
{
    const qreal currentPointSize = m_mathStyle.baseFont.pointSizeF();

    const int currentPixelSize = m_mathStyle.baseFont.pixelSize();

    QFont newFont = font;
    newFont.setItalic(false);

    if (currentPointSize > 0.0)
    {
        newFont.setPointSizeF(currentPointSize);
    }
    else if (currentPixelSize > 0)
    {
        newFont.setPixelSize(currentPixelSize);
    }

    m_mathStyle.baseFont = newFont;

    applyMathStyle();
}

void EquationWidget::setMathFontSize(int pointSize)
{
    pointSize = qBound(8, pointSize, 96);

    m_mathStyle.baseFont.setPointSize(pointSize);

    applyMathStyle();
}

void EquationWidget::setVariablesItalic(bool enabled)
{
    m_mathStyle.italicVariables = enabled;
    applyMathStyle();
}

QFont EquationWidget::mathFont() const
{
    return m_mathStyle.baseFont;
}

int EquationWidget::mathFontSize() const
{
    return m_mathStyle.baseFont.pointSize();
}

bool EquationWidget::variablesItalic() const
{
    return m_mathStyle.italicVariables;
}

bool EquationWidget::exportPdf(const QString &fileName, QString *errorMessage) const
{
    if (fileName.isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = tr("No se indicó un archivo PDF.");
        }

        return false;
    }

    const QFont exportFont = m_mathStyle.baseFont;

    const QFontMetricsF metrics(exportFont);

    const NodeLayout equationLayout = m_rootNode->layout(metrics);

    constexpr qreal padding = 18.0;

    const QSizeF pixelSize(equationLayout.size.width() + padding * 2.0, equationLayout.size.height() + padding * 2.0);

    const int dpi = qMax(72, logicalDpiX());

    /*
     * QPageSize espera puntos físicos.
     * El dibujo usa píxeles lógicos, por eso
     * convertimos según la resolución elegida.
     */
    const QSizeF pageSizePoints(pixelSize.width() * 72.0 / dpi, pixelSize.height() * 72.0 / dpi);

    QPdfWriter writer(fileName);

    writer.setResolution(dpi);
    writer.setTitle(tr("Ecuación"));
    writer.setCreator(tr("Clon de MathType"));

    writer.setPageSize(QPageSize(pageSizePoints, QPageSize::Point, QStringLiteral("Equation"), QPageSize::ExactMatch));

    writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);

    QPainter painter;

    if (!painter.begin(&writer))
    {
        if (errorMessage)
        {
            *errorMessage = tr("No se pudo crear el archivo PDF.");
        }

        return false;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(QRectF(QPointF(0, 0), pixelSize), Qt::white);

    painter.setFont(exportFont);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::NoBrush);

    m_rootNode->draw(painter, QPointF(padding, padding), metrics);

    painter.end();
    return true;
}

bool EquationWidget::exportSvg(const QString &fileName, QString *errorMessage) const
{
    if (fileName.isEmpty())
    {
        if (errorMessage)
        {
            *errorMessage = tr("No se indicó un archivo SVG.");
        }

        return false;
    }

    const QFont exportFont = m_mathStyle.baseFont;

    const QFontMetricsF metrics(exportFont);

    const NodeLayout equationLayout = m_rootNode->layout(metrics);

    constexpr qreal padding = 18.0;

    const QSizeF documentSize(equationLayout.size.width() + padding * 2.0,
                              equationLayout.size.height() + padding * 2.0);

    const QSize integerSize(qCeil(documentSize.width()), qCeil(documentSize.height()));

    QSvgGenerator generator;

    generator.setFileName(fileName);
    generator.setSize(integerSize);

    generator.setViewBox(QRectF(QPointF(0, 0), documentSize));

    generator.setResolution(qMax(72, logicalDpiX()));
    generator.setTitle(tr("Ecuación"));

    generator.setDescription(tr("Ecuación vectorial creada con "
                                "Clon de MathType"));

    QPainter painter;

    if (!painter.begin(&generator))
    {
        if (errorMessage)
        {
            *errorMessage = tr("No se pudo crear el archivo SVG.");
        }

        return false;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    /*
     * No dibujamos un fondo para conservar
     * la transparencia del SVG.
     */
    painter.setFont(exportFont);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::NoBrush);

    m_rootNode->draw(painter, QPointF(padding, padding), metrics);

    painter.end();
    return true;
}
