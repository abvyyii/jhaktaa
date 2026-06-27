#include "gateitem.h"

#include <QBrush>
#include <QDebug>
#include <QPen>
#include <QGraphicsSceneMouseEvent>

GateItem::GateItem(ItemKind kind, GateType type, QGraphicsItem* parent)
    : QGraphicsObject(parent), m_kind(kind), m_type(type), m_value(false), m_inputA(false), m_inputB(false), m_output(false), m_connected(false),
      m_rect(kind == ItemKind::Gate ? QRectF(0, 0, 56, 36) : QRectF(0, 0, 44, 28)), m_inputSourceA(nullptr), m_inputSourceB(nullptr) {
    setFlags(ItemIsMovable | ItemIsSelectable);
    setAcceptHoverEvents(true);
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
        painter->drawText(QRectF(0, 0, m_rect.width(), 24), Qt::AlignCenter, "INPUT");
        painter->drawText(QRectF(0, 28, m_rect.width(), 24), Qt::AlignCenter, QString::number(m_value ? 1 : 0));
        if (m_connected) {
            painter->drawText(QRectF(0, 48, m_rect.width(), 16), Qt::AlignCenter, "CONNECTED");
        }
        drawNode(painter, QPointF(m_rect.width(), m_rect.height() / 2));
    } else if (m_kind == ItemKind::OutputSink) {
        painter->drawRoundedRect(m_rect, 10, 10);
        painter->setPen(Qt::black);
        painter->drawText(QRectF(0, 0, m_rect.width(), 24), Qt::AlignCenter, "OUTPUT");
        painter->drawText(QRectF(0, 28, m_rect.width(), 24), Qt::AlignCenter, QString::number(m_output ? 1 : 0));
        if (m_connected) {
            painter->drawText(QRectF(0, 48, m_rect.width(), 16), Qt::AlignCenter, "CONNECTED");
        }
        drawNode(painter, QPointF(0, m_rect.height() / 2));
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
        painter->drawText(QRectF(0, 0, w, 24), Qt::AlignCenter, QString::fromStdString(LogicEngine::gateName(m_type)));
        if (m_type == GateType::NOT) {
            painter->drawText(QRectF(0, 30, w, 24), Qt::AlignCenter, QString("A:%1").arg(m_inputA ? 1 : 0));
            painter->drawText(QRectF(0, 50, w, 24), Qt::AlignCenter, QString("Q:%1").arg(m_output ? 1 : 0));
        } else {
            painter->drawText(QRectF(0, 30, w, 24), Qt::AlignCenter, QString("A:%1 B:%2").arg(m_inputA ? 1 : 0).arg(m_inputB ? 1 : 0));
            painter->drawText(QRectF(0, 50, w, 24), Qt::AlignCenter, QString("Q:%1").arg(m_output ? 1 : 0));
        }
        drawNode(painter, QPointF(0, h * 0.33));
        if (m_type != GateType::NOT) {
            drawNode(painter, QPointF(0, h * 0.66));
        }
        drawNode(painter, QPointF(w, h / 2));
    }
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

QPointF GateItem::outputAnchor() const {
    if (m_kind == ItemKind::OutputSink) {
        return mapToScene(QPointF(0, m_rect.height() / 2));
    }
    return mapToScene(QPointF(m_rect.width(), m_rect.height() / 2));
}

QPointF GateItem::inputAnchor(int slot) const {
    if (m_kind == ItemKind::InputSource) {
        return mapToScene(QPointF(m_rect.width(), m_rect.height() / 2));
    }
    if (m_kind == ItemKind::OutputSink) {
        return mapToScene(QPointF(0, m_rect.height() / 2));
    }
    if (m_type == GateType::NOT) {
        return mapToScene(QPointF(0, m_rect.height() / 2));
    }
    const qreal y = slot == 0 ? m_rect.height() * 0.33 : m_rect.height() * 0.66;
    return mapToScene(QPointF(0, y));
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
    }
    return QGraphicsObject::itemChange(change, value);
}

void GateItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);
    // Ignore double-clicks on gate items so only the Toggle Input button changes output.
}
