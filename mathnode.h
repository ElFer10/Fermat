#pragma once

#include <QFontMetricsF>
#include <QPainter>
#include <QPointF>
#include <QSizeF>
#include <QString>

#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

class FractionNode;
class RootNode;
class RowNode;
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

// ---------------------------------------------------------------------------
// NodeLayout
// ---------------------------------------------------------------------------

struct NodeLayout
{
    QSizeF size;
    qreal baseline = 0;
};

// ---------------------------------------------------------------------------
// MathNode — interfaz base
// ---------------------------------------------------------------------------

class MathNode
{
  public:
    virtual ~MathNode() = default;

    virtual NodeLayout layout(const QFontMetricsF &metrics) const = 0;
    virtual void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const = 0;
    virtual QString toLatex() const = 0;
};

// ---------------------------------------------------------------------------
// TextNode
// ---------------------------------------------------------------------------

class TextNode final : public MathNode
{
  public:
    explicit TextNode(QString text);

    // MathNode
    NodeLayout layout(const QFontMetricsF &metrics) const override;
    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;
    QString toLatex() const override;

  private:
    QString m_text;
};

// ---------------------------------------------------------------------------
// RowNode
// ---------------------------------------------------------------------------

class RowNode final : public MathNode
{
  public:
    explicit RowNode(FractionNode *ownerFraction = nullptr, RowRole role = RowRole::Root);
    RowNode(RootNode *ownerRoot, RowRole role);
    RowNode(ScriptNode *ownerScript, RowRole role);
    RowNode(LargeOperatorNode *ownerOperator, RowRole role);

    LargeOperatorNode *ownerOperator() const;

    // Propietario / rol

    ScriptNode *ownerScript() const;
    RootNode *ownerRoot() const;
    FractionNode *ownerFraction() const;
    RowRole role() const;
    std::unique_ptr<MathNode> takeNode(qsizetype index);

    // Gestión de hijos
    qsizetype childCount() const;
    MathNode *childAt(qsizetype index);
    const MathNode *childAt(qsizetype index) const;
    qsizetype indexOf(const MathNode *node) const;

    void appendNode(std::unique_ptr<MathNode> node);
    void insertNode(qsizetype index, std::unique_ptr<MathNode> node);
    void removeNode(qsizetype index);

    // MathNode
    NodeLayout layout(const QFontMetricsF &metrics) const override;
    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;
    QString toLatex() const override;

    // Edición
    qreal cursorOffset(qsizetype cursorPosition, const QFontMetricsF &metrics) const;

  private:
    std::vector<std::unique_ptr<MathNode>> m_children;
    FractionNode *m_ownerFraction = nullptr;
    RootNode *m_ownerRoot = nullptr;
    RowRole m_role = RowRole::Root;
    ScriptNode *m_ownerScript = nullptr;
    LargeOperatorNode *m_ownerOperator = nullptr;
};

// ---------------------------------------------------------------------------
// FractionNode
// ---------------------------------------------------------------------------

class FractionNode final : public MathNode
{
  public:
    FractionNode();
    FractionNode(std::unique_ptr<MathNode> numerator, std::unique_ptr<MathNode> denominator);

    // Acceso a filas internas
    RowNode *numeratorRow();
    const RowNode *numeratorRow() const;
    RowNode *denominatorRow();
    const RowNode *denominatorRow() const;

    // Fila contenedora
    RowNode *parentRow() const;
    void setParentRow(RowNode *parentRow);

    // MathNode
    NodeLayout layout(const QFontMetricsF &metrics) const override;
    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;
    QString toLatex() const override;

    // Posicionamiento
    QPointF numeratorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;
    QPointF denominatorPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

  private:
    std::unique_ptr<RowNode> m_numerator;
    std::unique_ptr<RowNode> m_denominator;

    RowNode *m_parentRow = nullptr; // No es propietario.
};

// ---------------------------------------------------------------------------
// RootNode
// ---------------------------------------------------------------------------

class RootNode final : public MathNode
{
  public:
    RootNode();

    // Acceso a fila interna
    RowNode *radicandRow();
    const RowNode *radicandRow() const;

    // Fila contenedora
    RowNode *parentRow() const;
    void setParentRow(RowNode *parentRow);

    // MathNode
    NodeLayout layout(const QFontMetricsF &metrics) const override;
    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;
    QString toLatex() const override;

    // Posicionamiento
    QPointF radicandPosition(const QPointF &topLeft, const QFontMetricsF &metrics) const;

  private:
    std::unique_ptr<RowNode> m_radicand;

    RowNode *m_parentRow = nullptr; // No es propietario.
};

// ---------------------------------------------------------------------------
// ScriptNode
// ---------------------------------------------------------------------------
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

  private:
    std::unique_ptr<RowNode> m_base;
    std::unique_ptr<RowNode> m_superscript;
    std::unique_ptr<RowNode> m_subscript;

    bool m_hasSuperscript = true;
    bool m_hasSubscript = false;

    // No es propietario.
    RowNode *m_parentRow = nullptr;
};

// ---------------------------------------------------------------------------
// LargeOperatorNode
// ---------------------------------------------------------------------------
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

  private:
    QString symbolText() const;

    QFont scaledFont(const QFontMetricsF &baseMetrics, qreal factor) const;

    LargeOperatorType m_type;

    std::unique_ptr<RowNode> m_upperLimit;
    std::unique_ptr<RowNode> m_lowerLimit;
    std::unique_ptr<RowNode> m_body;

    // No es propietario.
    RowNode *m_parentRow = nullptr;
};
