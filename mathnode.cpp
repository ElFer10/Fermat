#include "mathnode.h"

#include <QApplication>
#include <QPainterPath>

#include <algorithm>
#include <cstddef>
#include <utility>

// ============================================================
// TextNode
// ============================================================

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

QString TextNode::toLatex() const
{
    QString result;

    for (const QChar character : m_text)
    {
        switch (character.unicode())
        {
        case '\\':
            result += QStringLiteral("\\backslash ");
            break;

        case '{':
            result += QStringLiteral("\\{");
            break;

        case '}':
            result += QStringLiteral("\\}");
            break;

        case '_':
            result += QStringLiteral("\\_");
            break;

        case '^':
            result += QStringLiteral("\\^{}");
            break;

        case '%':
            result += QStringLiteral("\\%");
            break;

        case '#':
            result += QStringLiteral("\\#");
            break;

        case '&':
            result += QStringLiteral("\\&");
            break;

        case '$':
            result += QStringLiteral("\\$");
            break;

        default:
            result += character;
            break;
        }
    }

    return result;
}

// ============================================================
// RowNode
// ============================================================

RowNode::RowNode(FractionNode *ownerFraction, RowRole role) : m_ownerFraction(ownerFraction), m_role(role)
{
}

RowNode::RowNode(RootNode *ownerRoot, RowRole role) : m_role(role), m_ownerRoot(ownerRoot)
{
}

RowNode::RowNode(ScriptNode *ownerScript, RowRole role) : m_role(role), m_ownerScript(ownerScript)
{
}

FractionNode *RowNode::ownerFraction() const
{
    return m_ownerFraction;
}

RootNode *RowNode::ownerRoot() const
{
    return m_ownerRoot;
}

ScriptNode *RowNode::ownerScript() const
{
    return m_ownerScript;
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

qsizetype RowNode::indexOf(const MathNode *node) const
{
    for (qsizetype index = 0; index < childCount(); ++index)
    {
        if (childAt(index) == node)
            return index;
    }

    return -1;
}

void RowNode::appendNode(std::unique_ptr<MathNode> node)
{
    if (!node)
        return;

    if (auto *fraction = dynamic_cast<FractionNode *>(node.get()))
    {
        fraction->setParentRow(this);
    }

    if (auto *root = dynamic_cast<RootNode *>(node.get()))
    {
        root->setParentRow(this);
    }

    if (auto *script = dynamic_cast<ScriptNode *>(node.get()))
    {
        script->setParentRow(this);
    }
    if (auto *largeOperator = dynamic_cast<LargeOperatorNode *>(node.get()))
    {
        largeOperator->setParentRow(this);
    }

    m_children.push_back(std::move(node));
}

void RowNode::insertNode(qsizetype index, std::unique_ptr<MathNode> node)
{
    if (!node)
        return;

    index = std::clamp<qsizetype>(index, 0, childCount());

    if (auto *fraction = dynamic_cast<FractionNode *>(node.get()))
    {
        fraction->setParentRow(this);
    }

    if (auto *root = dynamic_cast<RootNode *>(node.get()))
    {
        root->setParentRow(this);
    }

    if (auto *script = dynamic_cast<ScriptNode *>(node.get()))
    {
        script->setParentRow(this);
    }

    const auto iterator = m_children.begin() + static_cast<std::ptrdiff_t>(index);

    m_children.insert(iterator, std::move(node));
}

std::unique_ptr<MathNode> RowNode::takeNode(qsizetype index)
{
    if (index < 0 || index >= childCount())
        return nullptr;

    auto iterator = m_children.begin() + static_cast<std::ptrdiff_t>(index);

    std::unique_ptr<MathNode> result = std::move(*iterator);

    m_children.erase(iterator);
    return result;
}

void RowNode::removeNode(qsizetype index)
{
    if (index < 0 || index >= childCount())
        return;

    const auto iterator = m_children.begin() + static_cast<std::ptrdiff_t>(index);

    m_children.erase(iterator);
}

NodeLayout RowNode::layout(const QFontMetricsF &metrics) const
{
    if (m_children.empty())
    {
        return {QSizeF(24.0, metrics.height()), metrics.ascent()};
    }

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
    {
        const MathNode *child = childAt(index);

        if (child)
        {
            offset += child->layout(metrics).size.width();
        }
    }

    return offset;
}

QString RowNode::toLatex() const
{
    QString result;

    for (const auto &child : m_children)
        result += child->toLatex();

    return result;
}

// ============================================================
// FractionNode
// ============================================================

FractionNode::FractionNode()
    : m_numerator(std::make_unique<RowNode>(this, RowRole::Numerator)),
      m_denominator(std::make_unique<RowNode>(this, RowRole::Denominator))
{
}

FractionNode::FractionNode(std::unique_ptr<MathNode> numerator, std::unique_ptr<MathNode> denominator) : FractionNode()
{
    if (numerator)
    {
        m_numerator->appendNode(std::move(numerator));
    }

    if (denominator)
    {
        m_denominator->appendNode(std::move(denominator));
    }
}

RowNode *FractionNode::numeratorRow()
{
    return m_numerator.get();
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

QString FractionNode::toLatex() const
{
    return QStringLiteral("\\frac{%1}{%2}").arg(m_numerator->toLatex(), m_denominator->toLatex());
}

// ============================================================
// RootNode
// ============================================================

RootNode::RootNode() : m_radicand(std::make_unique<RowNode>(this, RowRole::Radicand))
{
}

RowNode *RootNode::radicandRow()
{
    return m_radicand.get();
}

const RowNode *RootNode::radicandRow() const
{
    return m_radicand.get();
}

RowNode *RootNode::parentRow() const
{
    return m_parentRow;
}

void RootNode::setParentRow(RowNode *parentRow)
{
    m_parentRow = parentRow;
}

NodeLayout RootNode::layout(const QFontMetricsF &metrics) const
{
    const NodeLayout radicandLayout = m_radicand->layout(metrics);

    return {QSizeF(radicandLayout.size.width() + 18.0, radicandLayout.size.height() + 6.0),
            radicandLayout.baseline + 4.0};
}

QPointF RootNode::radicandPosition(const QPointF &topLeft, const QFontMetricsF &) const
{
    return {topLeft.x() + 18.0, topLeft.y() + 4.0};
}

void RootNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout rootLayout = layout(metrics);

    const QPointF contentPosition = radicandPosition(topLeft, metrics);

    m_radicand->draw(painter, contentPosition, metrics);

    const qreal x = topLeft.x();
    const qreal y = topLeft.y();

    const qreal bottom = y + rootLayout.size.height() - 1.0;

    QPainterPath rootPath;

    rootPath.moveTo(x + 1.0, y + rootLayout.size.height() * 0.58);

    rootPath.lineTo(x + 5.0, y + rootLayout.size.height() * 0.58);

    rootPath.lineTo(x + 9.0, bottom);
    rootPath.lineTo(x + 14.0, y + 2.0);

    rootPath.lineTo(x + rootLayout.size.width(), y + 2.0);

    QPen rootPen = painter.pen();
    rootPen.setWidthF(1.8);

    painter.save();
    painter.setPen(rootPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(rootPath);
    painter.restore();
}

QString RootNode::toLatex() const
{
    return QStringLiteral("\\sqrt{%1}").arg(m_radicand->toLatex());
}

// ============================================================
// ScriptNode
// ============================================================

ScriptNode::ScriptNode(bool hasSuperscript, bool hasSubscript)
    : m_base(std::make_unique<RowNode>(this, RowRole::ScriptBase)),
      m_superscript(std::make_unique<RowNode>(this, RowRole::Superscript)),
      m_subscript(std::make_unique<RowNode>(this, RowRole::Subscript)), m_hasSuperscript(hasSuperscript),
      m_hasSubscript(hasSubscript)
{
}

RowNode *ScriptNode::baseRow()
{
    return m_base.get();
}

RowNode *ScriptNode::superscriptRow()
{
    return m_superscript.get();
}

RowNode *ScriptNode::subscriptRow()
{
    return m_subscript.get();
}

const RowNode *ScriptNode::baseRow() const
{
    return m_base.get();
}

const RowNode *ScriptNode::superscriptRow() const
{
    return m_superscript.get();
}

const RowNode *ScriptNode::subscriptRow() const
{
    return m_subscript.get();
}

bool ScriptNode::hasSuperscript() const
{
    return m_hasSuperscript;
}

bool ScriptNode::hasSubscript() const
{
    return m_hasSubscript;
}

void ScriptNode::enableSuperscript()
{
    m_hasSuperscript = true;
}

void ScriptNode::enableSubscript()
{
    m_hasSubscript = true;
}

RowNode *ScriptNode::parentRow() const
{
    return m_parentRow;
}

void ScriptNode::setParentRow(RowNode *parentRow)
{
    m_parentRow = parentRow;
}

QFont ScriptNode::superscriptFont(const QFontMetricsF &baseMetrics) const
{
    QFont font = QApplication::font();
    const QFontMetricsF applicationMetrics(font);

    if (applicationMetrics.height() <= 0.0)
        return font;

    const qreal targetHeight = baseMetrics.height() * 0.70;

    const qreal scale = targetHeight / applicationMetrics.height();

    if (font.pointSizeF() > 0.0)
    {
        font.setPointSizeF(qMax(1.0, font.pointSizeF() * scale));
    }
    else if (font.pixelSize() > 0)
    {
        font.setPixelSize(qMax(1, qRound(font.pixelSize() * scale)));
    }

    return font;
}

NodeLayout ScriptNode::layout(const QFontMetricsF &metrics) const
{
    const NodeLayout baseLayout = m_base->layout(metrics);

    const QFont scriptFont = superscriptFont(metrics);

    const QFontMetricsF scriptMetrics(scriptFont);

    NodeLayout superscriptLayout;
    NodeLayout subscriptLayout;

    if (m_hasSuperscript)
    {
        superscriptLayout = m_superscript->layout(scriptMetrics);
    }

    if (m_hasSubscript)
    {
        subscriptLayout = m_subscript->layout(scriptMetrics);
    }

    const qreal baseTop = m_hasSuperscript ? superscriptLayout.size.height() * 0.45 : 0.0;

    const qreal baseline = baseTop + baseLayout.baseline;

    const qreal subscriptTop = baseline + metrics.descent() * 0.15;

    const qreal rightWidth = std::max(m_hasSuperscript ? superscriptLayout.size.width() : 0.0,
                                      m_hasSubscript ? subscriptLayout.size.width() : 0.0);

    qreal height = baseTop + baseLayout.size.height();

    if (m_hasSuperscript)
    {
        height = std::max(height, superscriptLayout.size.height());
    }

    if (m_hasSubscript)
    {
        height = std::max(height, subscriptTop + subscriptLayout.size.height());
    }

    return {QSizeF(baseLayout.size.width() + rightWidth, height), baseline};
}

QPointF ScriptNode::basePosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    qreal baseTop = 0.0;

    if (m_hasSuperscript)
    {
        const QFontMetricsF scriptMetrics(superscriptFont(metrics));

        const NodeLayout superscriptLayout = m_superscript->layout(scriptMetrics);

        baseTop = superscriptLayout.size.height() * 0.45;
    }

    return {topLeft.x(), topLeft.y() + baseTop};
}

QPointF ScriptNode::superscriptPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout baseLayout = m_base->layout(metrics);

    return {topLeft.x() + baseLayout.size.width(), topLeft.y()};
}

QPointF ScriptNode::subscriptPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const NodeLayout baseLayout = m_base->layout(metrics);

    const QPointF baseTopLeft = basePosition(topLeft, metrics);

    const qreal baseBaseline = baseTopLeft.y() + baseLayout.baseline;

    return {topLeft.x() + baseLayout.size.width(), baseBaseline + metrics.descent() * 0.15};
}

void ScriptNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    m_base->draw(painter, basePosition(topLeft, metrics), metrics);

    const QFont smallerFont = superscriptFont(metrics);

    const QFontMetricsF smallerMetrics(smallerFont);

    painter.save();
    painter.setFont(smallerFont);

    if (m_hasSuperscript)
    {
        m_superscript->draw(painter, superscriptPosition(topLeft, metrics), smallerMetrics);
    }

    if (m_hasSubscript)
    {
        m_subscript->draw(painter, subscriptPosition(topLeft, metrics), smallerMetrics);
    }

    painter.restore();
}

QString ScriptNode::toLatex() const
{
    QString result = QStringLiteral("{%1}").arg(m_base->toLatex());

    if (m_hasSubscript)
    {
        result += QStringLiteral("_{%1}").arg(m_subscript->toLatex());
    }

    if (m_hasSuperscript)
    {
        result += QStringLiteral("^{%1}").arg(m_superscript->toLatex());
    }

    return result;
}

RowNode::RowNode(LargeOperatorNode *ownerOperator, RowRole role) : m_role(role), m_ownerOperator(ownerOperator)
{
}

LargeOperatorNode *RowNode::ownerOperator() const
{
    return m_ownerOperator;
}

// ============================================================
// LargeOperatorNode
// ============================================================

LargeOperatorNode::LargeOperatorNode(LargeOperatorType type)
    : m_type(type), m_upperLimit(std::make_unique<RowNode>(this, RowRole::UpperLimit)),
      m_lowerLimit(std::make_unique<RowNode>(this, RowRole::LowerLimit)),
      m_body(std::make_unique<RowNode>(this, RowRole::OperatorBody))
{
}

LargeOperatorType LargeOperatorNode::type() const
{
    return m_type;
}

RowNode *LargeOperatorNode::upperLimitRow()
{
    return m_upperLimit.get();
}

RowNode *LargeOperatorNode::lowerLimitRow()
{
    return m_lowerLimit.get();
}

RowNode *LargeOperatorNode::bodyRow()
{
    return m_body.get();
}

const RowNode *LargeOperatorNode::upperLimitRow() const
{
    return m_upperLimit.get();
}

const RowNode *LargeOperatorNode::lowerLimitRow() const
{
    return m_lowerLimit.get();
}

const RowNode *LargeOperatorNode::bodyRow() const
{
    return m_body.get();
}

RowNode *LargeOperatorNode::parentRow() const
{
    return m_parentRow;
}

void LargeOperatorNode::setParentRow(RowNode *parentRow)
{
    m_parentRow = parentRow;
}

QString LargeOperatorNode::symbolText() const
{
    if (m_type == LargeOperatorType::Integral)
        return QStringLiteral("∫");

    return QStringLiteral("∑");
}

QFont LargeOperatorNode::scaledFont(const QFontMetricsF &baseMetrics, qreal factor) const
{
    QFont font = QApplication::font();
    const QFontMetricsF applicationMetrics(font);

    if (applicationMetrics.height() <= 0.0)
        return font;

    const qreal targetHeight = baseMetrics.height() * factor;

    const qreal scale = targetHeight / applicationMetrics.height();

    if (font.pointSizeF() > 0.0)
    {
        font.setPointSizeF(qMax(1.0, font.pointSizeF() * scale));
    }
    else if (font.pixelSize() > 0)
    {
        font.setPixelSize(qMax(1, qRound(font.pixelSize() * scale)));
    }

    return font;
}

QFont LargeOperatorNode::limitFont(const QFontMetricsF &baseMetrics) const
{
    return scaledFont(baseMetrics, 0.65);
}

QFont LargeOperatorNode::symbolFont(const QFontMetricsF &baseMetrics) const
{
    return scaledFont(baseMetrics, 1.55);
}

NodeLayout LargeOperatorNode::layout(const QFontMetricsF &metrics) const
{
    const QFontMetricsF limitMetrics(limitFont(metrics));

    const QFontMetricsF largeSymbolMetrics(symbolFont(metrics));

    const NodeLayout upperLayout = m_upperLimit->layout(limitMetrics);

    const NodeLayout lowerLayout = m_lowerLimit->layout(limitMetrics);

    const NodeLayout bodyLayout = m_body->layout(metrics);

    const qreal symbolWidth = largeSymbolMetrics.horizontalAdvance(symbolText());

    const qreal operatorWidth =
        std::max(symbolWidth, std::max(upperLayout.size.width(), lowerLayout.size.width())) + 6.0;

    const qreal symbolTop = upperLayout.size.height() + 2.0;

    const qreal operatorBaseline = symbolTop + largeSymbolMetrics.ascent();

    const qreal lowerTop = symbolTop + largeSymbolMetrics.height() + 2.0;

    const qreal bodyTop = operatorBaseline - bodyLayout.baseline;

    const qreal height = std::max(lowerTop + lowerLayout.size.height(), bodyTop + bodyLayout.size.height());

    return {QSizeF(operatorWidth + 8.0 + bodyLayout.size.width(), height), operatorBaseline};
}

QPointF LargeOperatorNode::upperLimitPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const QFontMetricsF limitMetrics(limitFont(metrics));

    const QFontMetricsF largeSymbolMetrics(symbolFont(metrics));

    const NodeLayout upperLayout = m_upperLimit->layout(limitMetrics);

    const NodeLayout lowerLayout = m_lowerLimit->layout(limitMetrics);

    const qreal symbolWidth = largeSymbolMetrics.horizontalAdvance(symbolText());

    const qreal operatorWidth =
        std::max(symbolWidth, std::max(upperLayout.size.width(), lowerLayout.size.width())) + 6.0;

    return {topLeft.x() + (operatorWidth - upperLayout.size.width()) / 2.0, topLeft.y()};
}

QPointF LargeOperatorNode::lowerLimitPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const QFontMetricsF limitMetrics(limitFont(metrics));

    const QFontMetricsF largeSymbolMetrics(symbolFont(metrics));

    const NodeLayout upperLayout = m_upperLimit->layout(limitMetrics);

    const NodeLayout lowerLayout = m_lowerLimit->layout(limitMetrics);

    const qreal symbolWidth = largeSymbolMetrics.horizontalAdvance(symbolText());

    const qreal operatorWidth =
        std::max(symbolWidth, std::max(upperLayout.size.width(), lowerLayout.size.width())) + 6.0;

    const qreal symbolTop = upperLayout.size.height() + 2.0;

    const qreal lowerTop = symbolTop + largeSymbolMetrics.height() + 2.0;

    return {topLeft.x() + (operatorWidth - lowerLayout.size.width()) / 2.0, topLeft.y() + lowerTop};
}

QPointF LargeOperatorNode::bodyPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const QFontMetricsF limitMetrics(limitFont(metrics));

    const QFontMetricsF largeSymbolMetrics(symbolFont(metrics));

    const NodeLayout upperLayout = m_upperLimit->layout(limitMetrics);

    const NodeLayout lowerLayout = m_lowerLimit->layout(limitMetrics);

    const NodeLayout bodyLayout = m_body->layout(metrics);

    const qreal symbolWidth = largeSymbolMetrics.horizontalAdvance(symbolText());

    const qreal operatorWidth =
        std::max(symbolWidth, std::max(upperLayout.size.width(), lowerLayout.size.width())) + 6.0;

    const qreal symbolTop = upperLayout.size.height() + 2.0;

    const qreal operatorBaseline = symbolTop + largeSymbolMetrics.ascent();

    return {topLeft.x() + operatorWidth + 8.0, topLeft.y() + operatorBaseline - bodyLayout.baseline};
}

void LargeOperatorNode::draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const
{
    const QFont smallFont = limitFont(metrics);

    const QFont largeFont = symbolFont(metrics);

    const QFontMetricsF smallMetrics(smallFont);

    const QFontMetricsF largeMetrics(largeFont);

    const NodeLayout upperLayout = m_upperLimit->layout(smallMetrics);

    const NodeLayout lowerLayout = m_lowerLimit->layout(smallMetrics);

    const qreal symbolWidth = largeMetrics.horizontalAdvance(symbolText());

    const qreal operatorWidth =
        std::max(symbolWidth, std::max(upperLayout.size.width(), lowerLayout.size.width())) + 6.0;

    const qreal symbolTop = upperLayout.size.height() + 2.0;

    const qreal symbolX = topLeft.x() + (operatorWidth - symbolWidth) / 2.0;

    painter.save();
    painter.setFont(smallFont);

    m_upperLimit->draw(painter, upperLimitPosition(topLeft, metrics), smallMetrics);

    m_lowerLimit->draw(painter, lowerLimitPosition(topLeft, metrics), smallMetrics);

    painter.restore();

    painter.save();
    painter.setFont(largeFont);

    painter.drawText(QPointF(symbolX, topLeft.y() + symbolTop + largeMetrics.ascent()), symbolText());

    painter.restore();

    m_body->draw(painter, bodyPosition(topLeft, metrics), metrics);
}

QString LargeOperatorNode::toLatex() const
{
    const QString command = m_type == LargeOperatorType::Integral ? QStringLiteral("\\int") : QStringLiteral("\\sum");

    return QStringLiteral("%1_{%2}^{%3} %4")
        .arg(command, m_lowerLimit->toLatex(), m_upperLimit->toLatex(), m_body->toLatex());
}
