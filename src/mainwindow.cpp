#include "mainwindow.h"

#include <QAbstractItemView>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QMimeData>
#include <QMessageBox>
#include <QPen>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_scene(new QGraphicsScene(this)),
      m_view(new QGraphicsView(m_scene, this)),
      m_selectedItem(nullptr),
      m_pendingSourceItem(nullptr),
      m_statusLabel(new QLabel("Drag inputs, gates, or outputs from the palette", this)),
      m_toggleAButton(new QPushButton("Toggle A", this)),
      m_toggleBButton(new QPushButton("Toggle B", this)),
      m_connectAButton(new QPushButton("Wire to A", this)),
      m_connectBButton(new QPushButton("Wire to B", this)),
      m_deleteButton(new QPushButton("Delete", this)),
      m_gatePalette(new QListWidget(this)),
      m_truthTable(new QTableWidget(this)),
      m_pendingInputSlot(-1),
      m_refreshingCircuit(false) {
    setWindowTitle("Jhatkaa - Digital Logic Gate Simulator");
    resize(1280, 800);

    createToolbar();
    createCanvas();
    createInspector();

    m_gatePalette->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gatePalette->setDragEnabled(true);
    m_gatePalette->setDragDropMode(QAbstractItemView::DragOnly);

    m_view->viewport()->setAcceptDrops(true);
    m_view->viewport()->installEventFilter(this);

    connect(m_gatePalette, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        addSceneItem(item->text(), QPointF(100, 100));
    });
    connect(m_toggleAButton, &QPushButton::clicked, this, &MainWindow::toggleInputA);
    connect(m_toggleBButton, &QPushButton::clicked, this, &MainWindow::toggleInputB);
    connect(m_connectAButton, &QPushButton::clicked, this, &MainWindow::startWireToInputA);
    connect(m_connectBButton, &QPushButton::clicked, this, &MainWindow::startWireToInputB);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedItem);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateSelectedGate);
}

void MainWindow::createToolbar() {
    auto* toolbar = addToolBar("Tools");
    toolbar->setMovable(false);

    auto* clearButton = new QPushButton("Clear", this);
    toolbar->addWidget(clearButton);
    toolbar->addSeparator();
    toolbar->addWidget(m_connectAButton);
    toolbar->addWidget(m_connectBButton);

    connect(clearButton, &QPushButton::clicked, this, [this]() {
        clearConnections();
        m_scene->clear();
        m_selectedItem = nullptr;
        m_pendingSourceItem = nullptr;
        m_pendingInputSlot = -1;
        m_statusLabel->setText("Canvas cleared");
        refreshTruthTable();
    });
}

void MainWindow::createCanvas() {
    auto* content = new QWidget(this);
    auto* layout = new QHBoxLayout(content);

    auto* palettePanel = new QWidget(this);
    auto* paletteLayout = new QVBoxLayout(palettePanel);
    paletteLayout->addWidget(new QLabel("Drag items onto the canvas"));
    paletteLayout->addWidget(m_gatePalette);
    paletteLayout->addStretch();

    m_gatePalette->setFixedHeight(260);
    m_gatePalette->addItem("INPUT");
    m_gatePalette->addItem("OUTPUT");
    m_gatePalette->addItem("AND");
    m_gatePalette->addItem("OR");
    m_gatePalette->addItem("NOT");
    m_gatePalette->addItem("XOR");
    m_gatePalette->addItem("XNOR");
    m_gatePalette->addItem("NAND");

    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setSceneRect(0, 0, 900, 600);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setMinimumSize(700, 600);

    layout->addWidget(palettePanel, 1);
    layout->addWidget(m_view, 4);
    setCentralWidget(content);
}

void MainWindow::createInspector() {
    auto* inspector = new QWidget(this);
    auto* inspectorLayout = new QVBoxLayout(inspector);

    inspectorLayout->addWidget(new QLabel("Selected item controls"));
    inspectorLayout->addWidget(m_toggleAButton);
    inspectorLayout->addWidget(m_toggleBButton);
    inspectorLayout->addWidget(m_connectAButton);
    inspectorLayout->addWidget(m_connectBButton);
    inspectorLayout->addWidget(m_deleteButton);
    inspectorLayout->addWidget(new QLabel("Truth Table / State"));
    inspectorLayout->addWidget(m_truthTable);
    inspectorLayout->addWidget(m_statusLabel);
    inspectorLayout->addStretch();

    m_truthTable->setColumnCount(3);
    m_truthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_truthTable->setHorizontalHeaderLabels({"A", "B", "Output"});
    m_truthTable->horizontalHeader()->setStretchLastSection(true);
    m_truthTable->verticalHeader()->setVisible(false);
    m_truthTable->setMinimumHeight(220);

    auto* inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(inspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_view->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto* dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasText()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        if (event->type() == QEvent::DragMove) {
            auto* dragEvent = static_cast<QDragMoveEvent*>(event);
            if (dragEvent->mimeData()->hasText()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
        if (event->type() == QEvent::Drop) {
            auto* dropEvent = static_cast<QDropEvent*>(event);
            const QString type = dropEvent->mimeData()->text().trimmed();
            const QPointF scenePos = m_view->mapToScene(dropEvent->position().toPoint());
            addSceneItem(type, scenePos);
            dropEvent->acceptProposedAction();
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            updateWirePositions();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::refreshCircuit() {
    if (m_refreshingCircuit) {
        return;
    }

    m_refreshingCircuit = true;
    QList<GateItem*> gates;
    for (QGraphicsItem* item : m_scene->items()) {
        auto* gate = qgraphicsitem_cast<GateItem*>(item);
        if (gate) {
            gates.append(gate);
        }
    }

    for (int pass = 0; pass < 10; ++pass) {
        bool changed = false;
        for (GateItem* gate : gates) {
            if (!gate) {
                continue;
            }
            const bool previous = gate->output();
            gate->evaluate();
            if (gate->output() != previous) {
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    updateWirePositions();
    refreshTruthTable();
    m_refreshingCircuit = false;
}

void MainWindow::refreshTruthTable() {
    qDebug() << "refreshTruthTable selected" << m_selectedItem;
    if (!m_selectedItem) {
        m_truthTable->setRowCount(0);
        return;
    }

    if (!m_selectedItem->scene()) {
        m_selectedItem = nullptr;
        m_truthTable->setRowCount(0);
        return;
    }

    qDebug() << "refreshTruthTable clear contents";
    m_truthTable->clearContents();
    m_truthTable->setRowCount(0);
    if (m_selectedItem->itemKind() == ItemKind::InputSource) {
        m_truthTable->setRowCount(1);
        m_truthTable->setItem(0, 0, new QTableWidgetItem(""));
        m_truthTable->setItem(0, 1, new QTableWidgetItem(""));
        m_truthTable->setItem(0, 2, new QTableWidgetItem(m_selectedItem->output() ? "1" : "0"));
    } else if (m_selectedItem->itemKind() == ItemKind::OutputSink) {
        m_truthTable->setRowCount(1);
        m_truthTable->setItem(0, 0, new QTableWidgetItem(""));
        m_truthTable->setItem(0, 1, new QTableWidgetItem(""));
        m_truthTable->setItem(0, 2, new QTableWidgetItem(m_selectedItem->output() ? "1" : "0"));
    } else {
        const auto type = m_selectedItem->gateType();
        if (type == GateType::NOT) {
            m_truthTable->setRowCount(2);
            for (int row = 0; row < 2; ++row) {
                const bool a = row == 1;
                const bool output = LogicEngine::evaluateGate(type, a);
                m_truthTable->setItem(row, 0, new QTableWidgetItem(QString::number(a)));
                m_truthTable->setItem(row, 1, new QTableWidgetItem("-"));
                m_truthTable->setItem(row, 2, new QTableWidgetItem(output ? "1" : "0"));
            }
        } else {
            m_truthTable->setRowCount(4);
            static const bool rows[4][2] = {{false, false}, {false, true}, {true, false}, {true, true}};
            for (int row = 0; row < 4; ++row) {
                const bool a = rows[row][0];
                const bool b = rows[row][1];
                const bool output = LogicEngine::evaluateGate(type, a, b);
                m_truthTable->setItem(row, 0, new QTableWidgetItem(a ? "1" : "0"));
                m_truthTable->setItem(row, 1, new QTableWidgetItem(b ? "1" : "0"));
                m_truthTable->setItem(row, 2, new QTableWidgetItem(output ? "1" : "0"));
            }
        }
    }
    m_truthTable->resizeColumnsToContents();
}

void MainWindow::updateWirePositions() {
    for (Connection& connection : m_connections) {
        if (!connection.source || !connection.target || !connection.line) {
            continue;
        }
        const QPointF start = connection.source->outputAnchor();
        const QPointF end = connection.target->inputAnchor(connection.inputSlot);
        connection.line->setLine(QLineF(start, end));
    }
}

bool MainWindow::addConnection(GateItem* source, GateItem* target, int inputSlot) {
    qDebug() << "addConnection start" << source << target << inputSlot;
    if (!source || !target || source == target || inputSlot < 0 || inputSlot > 1) {
        qDebug() << "addConnection invalid args";
        return false;
    }

    if (target->itemKind() == ItemKind::OutputSink && inputSlot != 0) {
        m_statusLabel->setText("Output only has one input slot");
        qDebug() << "addConnection invalid output slot";
        return false;
    }

    if (target->itemKind() == ItemKind::Gate && target->gateType() == GateType::NOT && inputSlot != 0) {
        m_statusLabel->setText("NOT gate only has one input slot");
        qDebug() << "addConnection invalid NOT gate slot";
        return false;
    }

    if (source->itemKind() == ItemKind::OutputSink) {
        m_statusLabel->setText("Cannot use an output sink as a source");
        qDebug() << "addConnection invalid source kind";
        return false;
    }

    for (const Connection& connection : m_connections) {
        if (connection.target == target && connection.inputSlot == inputSlot) {
            m_statusLabel->setText("That input already has a connection");
            qDebug() << "addConnection duplicate input";
            return false;
        }
    }

    qDebug() << "addConnection setting input source";
    if (inputSlot == 0) {
        target->setInputSourceA(source);
    } else {
        target->setInputSourceB(source);
    }

    qDebug() << "addConnection creating line";
    auto* line = m_scene->addLine(QLineF(source->outputAnchor(), target->inputAnchor(inputSlot)));
    line->setPen(QPen(Qt::darkCyan, 3));
    line->setZValue(-1);

    qDebug() << "addConnection pushing connection";
    m_connections.push_back({source, target, inputSlot, line});
    m_statusLabel->setText("Wire added");
    updateWirePositions();
    refreshCircuit();
    qDebug() << "addConnection complete";
    return true;
}

void MainWindow::clearConnections() {
    for (const Connection& connection : std::as_const(m_connections)) {
        if (connection.line) {
            m_scene->removeItem(connection.line);
            delete connection.line;
        }
    }
    m_connections.clear();
}

void MainWindow::removeItemWithConnections(GateItem* item) {
    std::vector<Connection> remaining;
    for (const Connection& connection : std::as_const(m_connections)) {
        if (connection.source == item || connection.target == item) {
            if (connection.line) {
                m_scene->removeItem(connection.line);
                delete connection.line;
            }
            if (connection.target == item) {
                if (connection.inputSlot == 0) {
                    connection.target->setInputSourceA(nullptr);
                } else {
                    connection.target->setInputSourceB(nullptr);
                }
            }
            if (connection.source == item && connection.target != item) {
                if (connection.inputSlot == 0) {
                    connection.target->setInputSourceA(nullptr);
                } else {
                    connection.target->setInputSourceB(nullptr);
                }
            }
            continue;
        }
        remaining.push_back(connection);
    }
    m_connections = std::move(remaining);
}

void MainWindow::deleteSelectedItem() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select an item to delete.");
        return;
    }
    removeItemWithConnections(m_selectedItem);
    m_scene->removeItem(m_selectedItem);
    delete m_selectedItem;
    m_selectedItem = nullptr;
    m_pendingSourceItem = nullptr;
    m_pendingInputSlot = -1;
    m_statusLabel->setText("Item deleted");
    refreshCircuit();
}

void MainWindow::addSceneItem(const QString& type, const QPointF& scenePos) {
    const QString kind = type.trimmed().toUpper();
    GateItem* item = nullptr;
    if (kind == "INPUT") {
        item = new GateItem(ItemKind::InputSource);
    } else if (kind == "OUTPUT") {
        item = new GateItem(ItemKind::OutputSink);
    } else {
        GateType gateType = GateType::AND;
        if (kind == "OR") gateType = GateType::OR;
        else if (kind == "NOT") gateType = GateType::NOT;
        else if (kind == "XOR") gateType = GateType::XOR;
        else if (kind == "XNOR") gateType = GateType::XNOR;
        else if (kind == "NAND") gateType = GateType::NAND;
        item = new GateItem(ItemKind::Gate, gateType);
    }
    item->setPos(scenePos);
    item->setSelected(true);
    m_scene->addItem(item);
    m_selectedItem = item;
    connect(item, &GateItem::nodeClicked, this, &MainWindow::onNodeClicked);
    refreshCircuit();
    m_statusLabel->setText(QString("Added %1").arg(type));
}

void MainWindow::toggleInputA() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select an item first.");
        return;
    }

    if (m_selectedItem->itemKind() == ItemKind::InputSource) {
        m_selectedItem->toggleValue();
    } else if (m_selectedItem->itemKind() == ItemKind::Gate) {
        m_selectedItem->toggleInputA();
    } else {
        m_statusLabel->setText("This item has no toggleable input");
        return;
    }

    refreshCircuit();
    m_statusLabel->setText("Input A toggled");
}

void MainWindow::toggleInputB() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select an item first.");
        return;
    }

    if (m_selectedItem->itemKind() == ItemKind::InputSource) {
        m_selectedItem->toggleValue();
    } else if (m_selectedItem->itemKind() == ItemKind::Gate) {
        m_selectedItem->toggleInputB();
    } else {
        m_statusLabel->setText("This item has no toggleable input");
        return;
    }

    refreshCircuit();
    m_statusLabel->setText("Input B toggled");
}

void MainWindow::startWireToInputA() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select a source item first.");
        return;
    }
    m_pendingSourceItem = m_selectedItem;
    m_pendingInputSlot = 0;
    m_statusLabel->setText("Now select a destination item");
}

void MainWindow::startWireToInputB() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select a source item first.");
        return;
    }
    m_pendingSourceItem = m_selectedItem;
    m_pendingInputSlot = 1;
    m_statusLabel->setText("Now select a destination item");
}

void MainWindow::updateSelectedGate() {
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    m_selectedItem = selectedItems.isEmpty() ? nullptr : qgraphicsitem_cast<GateItem*>(selectedItems.first());

    if (m_selectedItem) {
        const QString label = m_selectedItem->itemKind() == ItemKind::Gate
            ? QString::fromStdString(LogicEngine::gateName(m_selectedItem->gateType()))
            : (m_selectedItem->itemKind() == ItemKind::InputSource ? "INPUT" : "OUTPUT");
        m_statusLabel->setText(QString("Selected %1").arg(label));
    }
    refreshTruthTable();
}

void MainWindow::onNodeClicked(int slot, bool output) {
    auto* item = qobject_cast<GateItem*>(sender());
    qDebug() << "onNodeClicked" << "sender" << item << "slot" << slot << "output" << output;
    if (!item) {
        qDebug() << "onNodeClicked: sender is null";
        return;
    }

    if (output) {
        m_pendingSourceItem = item;
        m_pendingInputSlot = 0;
        m_statusLabel->setText("Output node selected. Now click an input node to connect.");
        return;
    }

    if (!m_pendingSourceItem) {
        m_statusLabel->setText("Select an output node first, then click an input node.");
        return;
    }

    if (item == m_pendingSourceItem) {
        m_statusLabel->setText("Cannot connect a node to itself.");
        return;
    }

    if (addConnection(m_pendingSourceItem, item, slot)) {
        qDebug() << "Connection added from" << m_pendingSourceItem << "to" << item << "slot" << slot;
        m_pendingSourceItem = nullptr;
        m_pendingInputSlot = -1;
    }
}
