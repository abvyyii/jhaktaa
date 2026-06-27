#pragma once

#include <QGraphicsObject>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>

#include "logicengine.h"

enum class ItemKind {
    InputSource,
    Gate,
    OutputSink
};

class GateItem;

class GateItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit GateItem(ItemKind kind, GateType type = GateType::AND, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    ItemKind itemKind() const;
    GateType gateType() const;
    bool output() const;
    void toggleValue();
    void setInputA(bool value);
    void setInputB(bool value);
    void toggleInputA();
    void toggleInputB();
    void setInputSourceA(GateItem* source);
    void setInputSourceB(GateItem* source);
    void clearInputSources();
    QPointF outputAnchor() const;
    QPointF inputAnchor(int slot) const;
    void evaluate();
    bool hasInputSource(int slot) const;

signals:
    void nodeClicked(int slot, bool output);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    bool nodeAt(const QPointF& localPos, int& slot, bool& output) const;
    ItemKind m_kind;
    GateType m_type;
    bool m_value;
    bool m_inputA;
    bool m_inputB;
    bool m_output;
    QRectF m_rect;
    GateItem* m_inputSourceA;
    GateItem* m_inputSourceB;
};
