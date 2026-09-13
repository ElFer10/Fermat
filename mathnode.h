#pragma once

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
    qreal baseline = 0;
};

class MathNode
{
  public:
    virtual ~MathNode() = default;

    virtual NodeLayout layout(const QFontMetricsF &metrics) const = 0;

    virtual void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const = 0;
};

class TextNode final : public MathNode
{
  public:
    explicit TextNode(QString text);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

  private:
    QString m_text;
};

class FractionNode;

enum class RowRole
{
    Root,
    Numerator,
    Denominator
};

class RowNode final : public MathNode
{
  public:
    explicit RowNode(FractionNode *ownerFraction = nullptr, RowRole role = RowRole::Root);

    FractionNode *ownerFraction() const;
    RowRole role() const;
    qsizetype childCount() const;
    MathNode *childAt(qsizetype index);
    const MathNode *childAt(qsizetype index) const;
    qsizetype indexOf(const MathNode *node) const;

    void appendNode(std::unique_ptr<MathNode> node);

    void insertNode(qsizetype index, std::unique_ptr<MathNode> node);

    void removeNode(qsizetype index);

    NodeLayout layout(const QFontMetricsF &metrics) const override;

    void draw(QPainter &painter, const QPointF &topLeft, const QFontMetricsF &metrics) const override;

    qreal cursorOffset(qsizetype cursorPosition, const QFontMetricsF &metrics) const;

  private:
    std::vector<std::unique_ptr<MathNode>> m_children;
    FractionNode *m_ownerFraction = nullptr;
    RowRole m_role = RowRole::Root;
};

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

  private:
    std::unique_ptr<RowNode> m_numerator;
    std::unique_ptr<RowNode> m_denominator;

    // No es propietario.
    RowNode *m_parentRow = nullptr;
};
