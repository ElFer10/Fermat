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

enum class RowRole
{
    Root,
    Numerator,
    Denominator,
    Radicand
};

class RowNode final : public MathNode
{
  public:
    explicit RowNode(FractionNode *ownerFraction = nullptr, RowRole role = RowRole::Root);
    RowNode(RootNode *ownerRoot, RowRole role);

    // Propietario / rol
    RootNode *ownerRoot() const;
    FractionNode *ownerFraction() const;
    RowRole role() const;

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
