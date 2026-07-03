#include "gateitem.h"

#include <QBrush>
#include <QDebug>
#include <QPen>
#include <QGraphicsSceneMouseEvent>

AnchorItem::AnchorItem(GateItem* parent, AnchorRole role)
    : QGraphicsEllipseItem(), m_parent(parent), m_role(role) {
    setRect(-5, -5, 10, 10);
    setBrush(Qt::white);
    setPen(QPen(Qt::black, 1));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(true);
    setParentItem(parent);
    setZValue(2);
}

GateItem* AnchorItem::gate() const {
    return m_parent;
}

AnchorItem::AnchorRole AnchorItem::role() const {
    return m_role;
}

int AnchorItem::inputSlot() const {
    return m_role == AnchorRole::InputB ? 1 : 0;
}

GateItem::GateItem(ItemKind kind, GateType type, QGraphicsItem* parent)
    : QGraphicsObject(parent), m_kind(kind), m_type(type), m_value(false), m_inputA(false), m_inputB(false), m_output(false), m_connected(false),
      m_rect(kind == ItemKind::Gate ? QRectF(0, 0, 98, 63) : QRectF(0, 0, 77, 49)), m_inputSourceA(nullptr), m_inputSourceB(nullptr),
      m_inputAnchorA(nullptr), m_inputAnchorB(nullptr), m_outputAnchor(nullptr) {
    setFlags(ItemIsMovable | ItemIsSelectable);
    setAcceptHoverEvents(true);

    if (m_kind == ItemKind::InputSource) {
        m_outputAnchor = new AnchorItem(this, AnchorItem::AnchorRole::Output);
    } else if (m_kind == ItemKind::OutputSink) {
        m_inputAnchorA = new AnchorItem(this, AnchorItem::AnchorRole::InputA);
    } else {
        m_inputAnchorA = new AnchorItem(this, AnchorItem::AnchorRole::InputA);
        if (m_type != GateType::NOT) {
            m_inputAnchorB = new AnchorItem(this, AnchorItem::AnchorRole::InputB);
        }
        m_outputAnchor = new AnchorItem(this, AnchorItem::AnchorRole::Output);
    }

    updateAnchorPositions();
    evaluate();
}

QRectF GateItem::boundingRect() const {
    return m_rect.adjusted(-6, -6, 6, 6);
}

static void drawNode(QPainter* painter, const QPointF& center) {
    painter->setBrush(Qt::white);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawEllipse(center, 5, 5);
}

ItemKind GateItem::itemKind() const {
    return m_kind;
}

GateType GateItem::gateType() const {
    return m_type;
}

bool GateItem::output() const {
    return m_output;
}

void GateItem::toggleValue() {
    if (m_kind == ItemKind::InputSource) {
        m_value = !m_value;
        evaluate();
    }
}

void GateItem::setInputA(bool value) {
    m_inputA = value;
    evaluate();
}

void GateItem::setInputB(bool value) {
    m_inputB = value;
    evaluate();
}

void GateItem::toggleInputA() {
    if (m_kind == ItemKind::Gate) {
        m_inputA = !m_inputA;
        evaluate();
    }
}

void GateItem::toggleInputB() {
    if (m_kind == ItemKind::Gate && m_type != GateType::NOT) {
        m_inputB = !m_inputB;
        evaluate();
    }
}

void GateItem::setInputSourceA(GateItem* source) {
    m_inputSourceA = source;
}

void GateItem::setInputSourceB(GateItem* source) {
    m_inputSourceB = source;
}

void GateItem::clearInputSources() {
    m_inputSourceA = nullptr;
    m_inputSourceB = nullptr;
    evaluate();
}

void GateItem::setConnected(bool connected) {
    if (m_connected != connected) {
        m_connected = connected;
        if (scene()) {
            update();
        }
    }
}

bool GateItem::isConnected() const {
    return m_connected;
}

QPointF GateItem::localOutputAnchor() const {
    if (m_kind == ItemKind::OutputSink) {
        return QPointF(0, m_rect.height() / 2);
    }
    return QPointF(m_rect.width(), m_rect.height() / 2);
}

QPointF GateItem::localInputAnchor(int slot) const {
    if (m_kind == ItemKind::InputSource) {
        return QPointF(m_rect.width(), m_rect.height() / 2);
    }
    if (m_kind == ItemKind::OutputSink) {
        return QPointF(0, m_rect.height() / 2);
    }
    if (m_type == GateType::NOT) {
        return QPointF(0, m_rect.height() / 2);
    }
    const qreal y = slot == 0 ? m_rect.height() * 0.33 : m_rect.height() * 0.66;
    return QPointF(0, y);
}

void GateItem::updateAnchorPositions() {
    if (m_inputAnchorA) {
        m_inputAnchorA->setPos(localInputAnchor(0));
    }
    if (m_inputAnchorB) {
        m_inputAnchorB->setPos(localInputAnchor(1));
    }
    if (m_outputAnchor) {
        m_outputAnchor->setPos(localOutputAnchor());
    }
}

QPointF GateItem::outputAnchor() const {
    return mapToScene(localOutputAnchor());
}

QPointF GateItem::inputAnchor(int slot) const {
    return mapToScene(localInputAnchor(slot));
}

void GateItem::evaluate() {
    fprintf(stderr, "GateItem::evaluate this=%p kind=%d type=%d\n", this, static_cast<int>(m_kind), static_cast<int>(m_type));
    fflush(stderr);
    bool newOutput = m_output;
    if (m_kind == ItemKind::InputSource) {
        newOutput = m_value;
    } else if (m_kind == ItemKind::OutputSink) {
        fprintf(stderr, " GateItem::evaluate output sink source=%p\n", m_inputSourceA.data());
        fflush(stderr);
        newOutput = m_inputSourceA ? m_inputSourceA->output() : false;
    } else {
        fprintf(stderr, " GateItem::evaluate gate sourceA=%p sourceB=%p\n", m_inputSourceA.data(), m_inputSourceB.data());
        fflush(stderr);
        const bool a = m_inputSourceA ? m_inputSourceA->output() : m_inputA;
        const bool b = m_inputSourceB ? m_inputSourceB->output() : m_inputB;
        if (m_type == GateType::NOT) {
            newOutput = LogicEngine::evaluateGate(m_type, a);
        } else {
            newOutput = LogicEngine::evaluateGate(m_type, a, b);
        }
    }

    if (m_output != newOutput) {
        m_output = newOutput;
        if (scene()) {
            update();
        }
    }
}

bool GateItem::hasInputSource(int slot) const {
    return slot == 0 ? m_inputSourceA != nullptr : m_inputSourceB != nullptr;
}

QVariant GateItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        update();
        updateAnchorPositions();
    }
    return QGraphicsObject::itemChange(change, value);
}
void GateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!painter || !painter->isActive() || !painter->device()) {
        return;
    }

    const QColor fill = isSelected() ? QColor(200, 230, 255) : QColor(245, 245, 245);
    const QColor border = m_connected ? QColor(34, 139, 34) : Qt::black;
    painter->setPen(QPen(border, m_connected ? 3 : 2));
    painter->setBrush(fill);

    if (m_kind == ItemKind::InputSource) {
        painter->drawRoundedRect(m_rect, 10, 10);
        painter->setPen(Qt::black);
        painter->drawText(QRectF(0, 0, m_rect.width(), m_rect.height()), Qt::AlignCenter,
                         QString("INPUT\n%1").arg(m_output ? "1" : "0"));
    } else if (m_kind == ItemKind::OutputSink) {
        painter->drawRoundedRect(m_rect, 10, 10);
        painter->setPen(Qt::black);
        painter->drawText(QRectF(0, 0, m_rect.width(), m_rect.height()), Qt::AlignCenter,
                         QString("OUTPUT\n%1").arg(m_output ? "1" : "0"));
    } else {
        const qreal w = m_rect.width();
        const qreal h = m_rect.height();
        QPainterPath path;
        if (m_type == GateType::AND || m_type == GateType::NAND) {
            path.moveTo(0, 0);
            path.lineTo(w * 0.55, 0);
            path.arcTo(w * 0.15, 0, w * 0.7, h, 90, -180);
            path.lineTo(0, h);
            path.closeSubpath();
        } else if (m_type == GateType::OR || m_type == GateType::XNOR) {
            path.moveTo(w * 0.15, 0);
            path.quadTo(w * 0.01, h / 2, w * 0.15, h);
            path.lineTo(w * 0.55, h);
            path.quadTo(w * 0.95, h / 2, w * 0.55, 0);
            path.closeSubpath();
        } else if (m_type == GateType::XOR) {
            path.moveTo(w * 0.15, 0);
            path.quadTo(w * 0.02, h / 2, w * 0.15, h);
            painter->setPen(QPen(Qt::black, 1));
            painter->drawPath(path);
            path = QPainterPath();
            path.moveTo(w * 0.2, 0);
            path.lineTo(w * 0.55, 0);
            path.quadTo(w * 0.95, h / 2, w * 0.55, h);
            path.lineTo(w * 0.2, h);
            path.quadTo(w * 0.1, h / 2, w * 0.2, 0);
            path.closeSubpath();
        } else if (m_type == GateType::NOT) {
            path.moveTo(0, 0);
            path.lineTo(w * 0.8, h / 2);
            path.lineTo(0, h);
            path.closeSubpath();
        }
        painter->drawPath(path);
        if (m_type == GateType::NAND || m_type == GateType::XNOR || m_type == GateType::NOT) {
            painter->setBrush(Qt::white);
            painter->drawEllipse(QPointF(w - 12, h / 2), 8, 8);
        }
        painter->setPen(Qt::black);
        painter->drawText(QRectF(0, 0, w, h), Qt::AlignCenter, QString::fromStdString(LogicEngine::gateName(m_type)));
    }
}

void AnchorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    const QColor fill = isSelected() ? QColor(255, 215, 0) : QColor(255, 255, 255);
    const QColor border = isSelected() ? QColor(255, 140, 0) : QColor(0, 0, 0);
    painter->setBrush(fill);
    painter->setPen(QPen(border, isSelected() ? 2 : 1));
    painter->drawEllipse(rect());
}
void GateItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);
    // Ignore double-clicks on gate items so only the Toggle Input button changes output.
}
