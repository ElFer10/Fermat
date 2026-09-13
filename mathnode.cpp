#include "mathnode.h"

#include <algorithm>
#include <utility>

TextNode::TextNode(QString text) : m_text(std::move(text))
{
}

NodeLayout TextNode::layout(const QFontMetricsF &metrics) const
{
    const qreal width = m_text.isEmpty() ? 24.0 : metrics.horizontalAdvance(m_text);

    return {QSizeF(width, metrics.height()), metrics.ascent()};
}

void TextNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    painter.drawText(QPointF(topLeft.x(), topLeft.y() + metrics.ascent()), m_text);
}

FractionNode::FractionNode()
    : m_numerator(std::make_unique<RowNode>(this, RowRole::Numerator)),
      m_denominator(std::make_unique<RowNode>(this, RowRole::Denominator))
{
}

FractionNode::FractionNode(std::unique_ptr<MathNode> numerator, std::unique_ptr<MathNode> denominator) : FractionNode()
{
    if (numerator)
        m_numerator->appendNode(std::move(numerator));

    if (denominator)
        m_denominator->appendNode(std::move(denominator));
}

RowNode *FractionNode::numeratorRow()
{
    return m_numerator.get();
}

qsizetype RowNode::indexOf(const MathNode *node) const
{
    for (qsizetype index = 0; index < childCount(); ++index)
    {
        if (childAt(index) == node)
            return index;
    }

    return -1;
}

RowNode *FractionNode::denominatorRow()
{
    return m_denominator.get();
}

const RowNode *FractionNode::numeratorRow() const
{
    return m_numerator.get();
}

const RowNode *FractionNode::denominatorRow() const
{
    return m_denominator.get();
}

RowNode *FractionNode::parentRow() const
{
    return m_parentRow;
}

void FractionNode::setParentRow(RowNode *parentRow)
{
    m_parentRow = parentRow;
}

NodeLayout FractionNode::layout(const QFontMetricsF &metrics) const
{
    const NodeLayout numeratorLayout = m_numerator->layout(metrics);

    const NodeLayout denominatorLayout = m_denominator->layout(metrics);

    const qreal width = std::max(numeratorLayout.size.width(), denominatorLayout.size.width()) + 12.0;

    const qreal height = numeratorLayout.size.height() + 8.0 + denominatorLayout.size.height();

    return {QSizeF(width, height), numeratorLayout.size.height() + 4.0};
}

QPointF FractionNode::numeratorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout fractionLayout = layout(metrics);
    const NodeLayout numeratorLayout = m_numerator->layout(metrics);

    return {topLeft.x() + (fractionLayout.size.width() - numeratorLayout.size.width()) / 2.0, topLeft.y()};
}

QPointF FractionNode::denominatorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout fractionLayout = layout(metrics);
    const NodeLayout numeratorLayout = m_numerator->layout(metrics);

    const NodeLayout denominatorLayout = m_denominator->layout(metrics);

    return {topLeft.x() + (fractionLayout.size.width() - denominatorLayout.size.width()) / 2.0,
            topLeft.y() + numeratorLayout.size.height() + 8.0};
}

void FractionNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout fractionLayout = layout(metrics);
    const NodeLayout numeratorLayout = m_numerator->layout(metrics);

    m_numerator->draw(painter, numeratorPosition(topLeft, metrics), metrics);

    m_denominator->draw(painter, denominatorPosition(topLeft, metrics), metrics);

    const qreal lineY = topLeft.y() + numeratorLayout.size.height() + 4.0;

    painter.drawLine(QPointF(topLeft.x(), lineY), QPointF(topLeft.x() + fractionLayout.size.width(), lineY));
}

RowNode::RowNode(FractionNode *ownerFraction, RowRole role) : m_ownerFraction(ownerFraction), m_role(role)
{
}

FractionNode *RowNode::ownerFraction() const
{
    return m_ownerFraction;
}

RowRole RowNode::role() const
{
    return m_role;
}

qsizetype RowNode::childCount() const
{
    return static_cast<qsizetype>(m_children.size());
}

MathNode *RowNode::childAt(qsizetype index)
{
    if (index < 0 || index >= childCount())
        return nullptr;

    return m_children[static_cast<std::size_t>(index)].get();
}

const MathNode *RowNode::childAt(qsizetype index) const
{
    if (index < 0 || index >= childCount())
        return nullptr;

    return m_children[static_cast<std::size_t>(index)].get();
}

void RowNode::appendNode(std::unique_ptr<MathNode> node)
{
    if (!node)
        return;

    if (auto *fraction = dynamic_cast<FractionNode *>(node.get()))
        fraction->setParentRow(this);

    m_children.push_back(std::move(node));
}

void RowNode::insertNode(qsizetype index, std::unique_ptr<MathNode> node)
{
    if (!node)
        return;

    index = std::clamp<qsizetype>(index, 0, childCount());

    if (auto *fraction = dynamic_cast<FractionNode *>(node.get()))
        fraction->setParentRow(this);

    m_children.insert(m_children.begin() + index, std::move(node));
}

void RowNode::removeNode(qsizetype index)
{
    if (index < 0 || index >= childCount())
        return;

    m_children.erase(m_children.begin() + index);
}

NodeLayout RowNode::layout(const QFontMetricsF &metrics) const
{
    if (m_children.empty())
        return {QSizeF(24.0, metrics.height()), metrics.ascent()};

    qreal totalWidth = 0.0;
    qreal maximumBaseline = 0.0;
    qreal maximumDescent = 0.0;

    for (const auto &child : m_children)
    {
        const NodeLayout childLayout = child->layout(metrics);

        totalWidth += childLayout.size.width();
        maximumBaseline = std::max(maximumBaseline, childLayout.baseline);

        maximumDescent = std::max(maximumDescent, childLayout.size.height() - childLayout.baseline);
    }

    return {QSizeF(totalWidth, maximumBaseline + maximumDescent), maximumBaseline};
}

void RowNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout rowLayout = layout(metrics);
    qreal currentX = topLeft.x();

    for (const auto &child : m_children)
    {
        const NodeLayout childLayout = child->layout(metrics);

        const QPointF childPosition(currentX, topLeft.y() + rowLayout.baseline - childLayout.baseline);

        child->draw(painter, childPosition, metrics);
        currentX += childLayout.size.width();
    }
}

qreal RowNode::cursorOffset(qsizetype cursorPosition, const QFontMetricsF &metrics) const
{
    cursorPosition = std::clamp<qsizetype>(cursorPosition, 0, childCount());

    qreal offset = 0.0;

    for (qsizetype index = 0; index < cursorPosition; ++index)
        offset += childAt(index)->layout(metrics).size.width();

    return offset;
}
