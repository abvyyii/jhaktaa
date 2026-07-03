#pragma once

#include <QGraphicsEllipseItem>
#include <QGraphicsObject>
#include <QPointer>
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

class AnchorItem : public QGraphicsEllipseItem {
public:
    enum class AnchorRole {
        InputA,
        InputB,
        Output
    };

    explicit AnchorItem(GateItem* parent, AnchorRole role);

    GateItem* gate() const;
    AnchorRole role() const;
    int inputSlot() const;
    enum { Type = UserType + 2 };

    int type() const override { return Type; }    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
private:
    GateItem* m_parent;
    AnchorRole m_role;
};

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
    void setConnected(bool connected);
    bool isConnected() const;
    enum { Type = QGraphicsItem::UserType + 1 };

    QPointF outputAnchor() const;
    QPointF inputAnchor(int slot) const;
    void evaluate();
    bool hasInputSource(int slot) const;
    void updateAnchorPositions();
    QPointF localOutputAnchor() const;
    QPointF localInputAnchor(int slot) const;

signals:
    // Selection-based connection handling is managed by MainWindow.

protected:
    int type() const override { return Type; }
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    ItemKind m_kind;
    GateType m_type;
    bool m_value;
    bool m_inputA;
    bool m_inputB;
    bool m_output;
    bool m_connected;
    QRectF m_rect;
    QPointer<GateItem> m_inputSourceA;
    QPointer<GateItem> m_inputSourceB;
    AnchorItem* m_inputAnchorA;
    AnchorItem* m_inputAnchorB;
    AnchorItem* m_outputAnchor;
};
