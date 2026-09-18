#pragma once

#include "mathstyle.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPointF>
#include <QSizeF>
#include <QString>

#include <memory>
#include <vector>

struct NodeLayout
{
    QSizeF size;
    qreal baseline = 0.0;
};

class FractionNode;
class RootNode;
class ScriptNode;
class LargeOperatorNode;

enum class RowRole
{
    Root,
    Numerator,
    Denominator,
    Radicand,
    ScriptBase,
    Superscript,
    Subscript,
    UpperLimit,
    LowerLimit,
    OperatorBody
};

enum class LargeOperatorType
{
    Integral,
    Summation
};

// ============================================================
// MathNode
// ============================================================

class MathNode
{
  public:
    virtual ~MathNode() = default;

    virtual NodeLayout layout(const QFontMetricsF &metrics) const = 0;

    virtual void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const = 0;

    virtual QString toLatex() const = 0;

    virtual void setMathStyle(const MathStyle &style) = 0;
};

// ============================================================
// TextNode
// ============================================================

class TextNode final : public MathNode
{
  public:
    explicit TextNode(QString text);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_hasMathStyle = true;
    }

  private:
    QString m_text;

    MathStyle m_style;
    bool m_hasMathStyle = false;
};

// ============================================================
// RowNode
// ============================================================

class RowNode final : public MathNode
{
  public:
    explicit RowNode(FractionNode *ownerFraction = nullptr, RowRole role = RowRole::Root);

    RowNode(RootNode *ownerRoot, RowRole role);

    RowNode(ScriptNode *ownerScript, RowRole role);

    RowNode(LargeOperatorNode *ownerOperator, RowRole role);

    FractionNode *ownerFraction() const;
    RootNode *ownerRoot() const;
    ScriptNode *ownerScript() const;

    LargeOperatorNode *ownerOperator() const;

    RowRole role() const;

    qsizetype childCount() const;

    MathNode *childAt(qsizetype index);

    const MathNode *childAt(qsizetype index) const;

    qsizetype indexOf(const MathNode *node) const;

    void appendNode(std::unique_ptr<MathNode> node);

    void insertNode(qsizetype index, std::unique_ptr<MathNode> node);

    std::unique_ptr<MathNode> takeNode(qsizetype index);

    void removeNode(qsizetype index);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    qreal cursorOffset(qsizetype cursorPosition, const QFontMetricsF &metrics) const;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_hasMathStyle = true;

        for (const auto &child : m_children)
        {
            child->setMathStyle(style);
        }
    }

  private:
    std::vector<std::unique_ptr<MathNode>> m_children;

    FractionNode *m_ownerFraction = nullptr;
    RowRole m_role = RowRole::Root;

    RootNode *m_ownerRoot = nullptr;
    ScriptNode *m_ownerScript = nullptr;

    LargeOperatorNode *m_ownerOperator = nullptr;

    MathStyle m_style;
    bool m_hasMathStyle = false;
};

// ============================================================
// FractionNode
// ============================================================

class FractionNode final : public MathNode
{
  public:
    FractionNode();

    FractionNode(std::unique_ptr<MathNode> numerator, std::unique_ptr<MathNode> denominator);

    RowNode *numeratorRow();
    RowNode *denominatorRow();

    const RowNode *numeratorRow() const;
    const RowNode *denominatorRow() const;

    RowNode *parentRow() const;

    void setParentRow(RowNode *parentRow);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    QPointF numeratorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QPointF denominatorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_numerator->setMathStyle(style);
        m_denominator->setMathStyle(style);
    }

  private:
    std::unique_ptr<RowNode> m_numerator;
    std::unique_ptr<RowNode> m_denominator;

    RowNode *m_parentRow = nullptr;

    MathStyle m_style;
};

// ============================================================
// RootNode
// ============================================================

class RootNode final : public MathNode
{
  public:
    RootNode();

    RowNode *radicandRow();

    const RowNode *radicandRow() const;

    RowNode *parentRow() const;

    void setParentRow(RowNode *parentRow);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    QPointF radicandPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_radicand->setMathStyle(style);
    }

  private:
    std::unique_ptr<RowNode> m_radicand;

    RowNode *m_parentRow = nullptr;

    MathStyle m_style;
};

// ============================================================
// ScriptNode
// ============================================================

class ScriptNode final : public MathNode
{
  public:
    explicit ScriptNode(bool hasSuperscript = true, bool hasSubscript = false);

    RowNode *baseRow();
    RowNode *superscriptRow();
    RowNode *subscriptRow();

    const RowNode *baseRow() const;
    const RowNode *superscriptRow() const;
    const RowNode *subscriptRow() const;

    bool hasSuperscript() const;
    bool hasSubscript() const;

    void enableSuperscript();
    void enableSubscript();

    RowNode *parentRow() const;

    void setParentRow(RowNode *parentRow);

    QFont superscriptFont(const QFontMetricsF &baseMetrics) const;

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    QPointF basePosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QPointF superscriptPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QPointF subscriptPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_hasMathStyle = true;

        m_base->setMathStyle(style);

        const MathStyle scriptStyle = style.scaled(0.70);

        m_superscript->setMathStyle(scriptStyle);

        m_subscript->setMathStyle(scriptStyle);
    }

  private:
    std::unique_ptr<RowNode> m_base;
    std::unique_ptr<RowNode> m_superscript;
    std::unique_ptr<RowNode> m_subscript;

    bool m_hasSuperscript = true;
    bool m_hasSubscript = false;

    RowNode *m_parentRow = nullptr;

    MathStyle m_style;
    bool m_hasMathStyle = false;
};

// ============================================================
// LargeOperatorNode
// ============================================================

class LargeOperatorNode final : public MathNode
{
  public:
    explicit LargeOperatorNode(LargeOperatorType type);

    LargeOperatorType type() const;

    RowNode *upperLimitRow();
    RowNode *lowerLimitRow();
    RowNode *bodyRow();

    const RowNode *upperLimitRow() const;
    const RowNode *lowerLimitRow() const;
    const RowNode *bodyRow() const;

    RowNode *parentRow() const;

    void setParentRow(RowNode *parentRow);

    QFont limitFont(const QFontMetricsF &baseMetrics) const;

    QFont symbolFont(const QFontMetricsF &baseMetrics) const;

    QPointF upperLimitPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QPointF lowerLimitPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    QPointF bodyPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    QString toLatex() const override;

    void setMathStyle(const MathStyle &style) override
    {
        m_style = style;
        m_hasMathStyle = true;

        const MathStyle limitStyle = style.scaled(0.65);

        m_upperLimit->setMathStyle(limitStyle);

        m_lowerLimit->setMathStyle(limitStyle);

        m_body->setMathStyle(style);
    }

  private:
    QString symbolText() const;

    QFont scaledFont(const QFontMetricsF &baseMetrics, qreal factor) const;

    LargeOperatorType m_type;

    std::unique_ptr<RowNode> m_upperLimit;
    std::unique_ptr<RowNode> m_lowerLimit;
    std::unique_ptr<RowNode> m_body;

    RowNode *m_parentRow = nullptr;

    MathStyle m_style;
    bool m_hasMathStyle = false;
};
