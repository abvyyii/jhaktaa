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
#include <QShortcut>
#include <QStatusBar>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_scene(new QGraphicsScene(this)),
      m_view(new QGraphicsView(m_scene, this)),
      m_selectedItem(nullptr),
      m_statusLabel(new QLabel("Drag inputs, gates, or outputs from the palette", this)),
      m_toggleAButton(new QPushButton("Toggle Input", this)),
      m_toggleBButton(nullptr),
      m_connectButton(new QPushButton("Connect selected", this)),
      m_deleteButton(new QPushButton("Delete", this)),
      m_gatePalette(new QListWidget(this)),
      m_truthTable(new QTableWidget(this)),
      m_refreshingCircuit(false),
      m_updatingSelection(false) {
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
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::connectSelectedItems);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedItem);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateSelectedGate);

    auto* connectShortcut = new QShortcut(QKeySequence(Qt::Key_C), this);
    connect(connectShortcut, &QShortcut::activated, this, &MainWindow::connectSelectedItems);

    m_connectButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_toggleAButton->setEnabled(false);
}

void MainWindow::createToolbar() {
    auto* toolbar = addToolBar("Tools");
    toolbar->setMovable(false);

    auto* clearButton = new QPushButton("Clear", this);
    auto* connectToolbarButton = new QPushButton("Connect selected", this);
    toolbar->addWidget(clearButton);
    toolbar->addSeparator();
    toolbar->addWidget(connectToolbarButton);

    connect(clearButton, &QPushButton::clicked, this, [this]() {
        clearConnections();
        m_scene->clear();
        m_selectedItem = nullptr;
        m_statusLabel->setText("Canvas cleared");
        refreshTruthTable();
    });
    connect(connectToolbarButton, &QPushButton::clicked, this, &MainWindow::connectSelectedItems);
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
    inspectorLayout->addWidget(m_connectButton);
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
    inspectorDock->show();

    auto* hintLabel = new QLabel("C = connect selected", this);
    statusBar()->addPermanentWidget(hintLabel);
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
        qDebug() << "refreshCircuit reentered";
        return;
    }

    m_refreshingCircuit = true;
    const QList<QGraphicsItem*> sceneItems = m_scene->items();
    qDebug() << "refreshCircuit sceneItems=" << sceneItems.size();
    QList<GateItem*> gates;
    for (int i = 0; i < sceneItems.size(); ++i) {
        QGraphicsItem* item = sceneItems.at(i);
        qDebug() << "refreshCircuit item" << i << item << "type=" << item->type() << "parent=" << item->parentItem();
        auto* gate = qgraphicsitem_cast<GateItem*>(item);
        if (gate) {
            if (!gate->scene() || gate->scene() != m_scene) {
                qDebug() << "refreshCircuit skipping detached gate" << gate << "scene=" << gate->scene();
                continue;
            }
            qDebug() << "refreshCircuit found gate" << gate << "kind=" << static_cast<int>(gate->itemKind()) << "type=" << static_cast<int>(gate->gateType()) << "scene=" << gate->scene();
            gates.append(gate);
        }
    }
    qDebug() << "refreshCircuit" << "gateCount=" << gates.size();
    fprintf(stderr, "refreshCircuit gateCount=%lu\n", static_cast<unsigned long>(gates.size()));
    fflush(stderr);

    fprintf(stderr, "refreshCircuit before_loop\n");
    fflush(stderr);
    for (int pass = 0; pass < 10; ++pass) {
        bool changed = false;
        qDebug() << "refreshCircuit pass" << pass;
        fprintf(stderr, "refreshCircuit pass=%d\n", pass);
        fflush(stderr);
        for (GateItem* gate : gates) {
            if (!gate) {
                qDebug() << "refreshCircuit skipping null gate";
                continue;
            }
            if (gate->itemKind() == ItemKind::InputSource) {
                qDebug() << "refreshCircuit skipping input source" << gate;
                continue;
            }
            qDebug() << "refreshCircuit evaluating" << gate << "kind=" << static_cast<int>(gate->itemKind()) << "type=" << static_cast<int>(gate->gateType()) << "scene=" << gate->scene();
            const bool previous = gate->output();
            gate->evaluate();
            qDebug() << "refreshCircuit evaluated" << gate << "output=" << gate->output();
            if (gate->output() != previous) {
                changed = true;
                qDebug() << "refreshCircuit changed" << gate << "from" << previous << "to" << gate->output();
            }
        }
        qDebug() << "refreshCircuit pass end" << pass << "changed=" << changed;
        if (!changed) {
            qDebug() << "refreshCircuit stabilized";
            break;
        }
    }

    for (GateItem* gate : gates) {
        qDebug() << "refreshCircuit updating gate" << gate << "scene=" << (gate ? gate->scene() : nullptr);
        if (gate && gate->scene() && !gate->scene()->views().isEmpty()) {
            gate->update();
        }
    }

    qDebug() << "refreshCircuit before updateWirePositions";
    updateWirePositions();
    qDebug() << "refreshCircuit before refreshTruthTable";
    refreshTruthTable();
    qDebug() << "refreshCircuit complete";
    m_refreshingCircuit = false;
}

void MainWindow::refreshTruthTable() {
    if (m_updatingSelection) {
        return;
    }

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
    qDebug() << "updateWirePositions connections=" << m_connections.size();
    for (Connection& connection : m_connections) {
        qDebug() << " updateWirePositions checking" << connection.source << connection.target << connection.line << connection.inputSlot;
        if (!connection.source || !connection.target || !connection.line) {
            qDebug() << "  updateWirePositions skipping invalid connection";
            continue;
        }
        if (!connection.source->scene() || !connection.target->scene() || connection.source->scene() != m_scene || connection.target->scene() != m_scene) {
            qDebug() << "  updateWirePositions skipping detached connection";
            continue;
        }
        if (!connection.line->scene() || connection.line->scene() != m_scene) {
            qDebug() << "  updateWirePositions skipping detached line";
            continue;
        }
        const QPointF start = connection.source->outputAnchor();
        const QPointF end = connection.target->inputAnchor(connection.inputSlot);
        qDebug() << "  updateWirePositions line" << start << end;
        connection.line->setLine(QLineF(start, end));
    }
    qDebug() << "updateWirePositions done";
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

    if ((inputSlot == 0 && target->hasInputSource(0)) || (inputSlot == 1 && target->hasInputSource(1))) {
        m_statusLabel->setText("That input already has a connection");
        qDebug() << "addConnection target already has a source on slot" << inputSlot;
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
    line->setFlag(QGraphicsItem::ItemIsSelectable, false);
    line->setAcceptedMouseButtons(Qt::NoButton);

    qDebug() << "addConnection pushing connection";
    m_connections.push_back({source, target, inputSlot, line});
    source->setConnected(true);
    target->setConnected(true);
    m_statusLabel->setText("Wire added");
    updateWirePositions();
    qDebug() << "addConnection before refreshCircuit";
    refreshCircuit();
    qDebug() << "addConnection after refreshCircuit";
    return true;
}

void MainWindow::clearConnections() {
    for (const Connection& connection : std::as_const(m_connections)) {
        if (connection.line) {
            m_scene->removeItem(connection.line);
            delete connection.line;
        }
        if (connection.target) {
            if (connection.inputSlot == 0) {
                connection.target->setInputSourceA(nullptr);
            } else {
                connection.target->setInputSourceB(nullptr);
            }
        }
    }
    m_connections.clear();
    for (QGraphicsItem* graphicsItem : m_scene->items()) {
        if (auto* gate = qgraphicsitem_cast<GateItem*>(graphicsItem)) {
            gate->setConnected(false);
        }
    }
}

void MainWindow::removeItemWithConnections(GateItem* item) {
    std::vector<Connection> remaining;
    for (const Connection& connection : std::as_const(m_connections)) {
        if (connection.source == item || connection.target == item) {
            if (connection.line) {
                m_scene->removeItem(connection.line);
                delete connection.line;
            }
            if (connection.target) {
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
    for (QGraphicsItem* graphicsItem : m_scene->items()) {
        if (auto* gate = qgraphicsitem_cast<GateItem*>(graphicsItem)) {
            gate->setConnected(false);
        }
    }
    for (const Connection& connection : std::as_const(m_connections)) {
        if (connection.source) {
            connection.source->setConnected(true);
        }
        if (connection.target) {
            connection.target->setConnected(true);
        }
    }
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
    refreshCircuit();
    m_statusLabel->setText(QString("Added %1").arg(type));
}

void MainWindow::toggleInputA() {
    if (!m_selectedItem) {
        QMessageBox::information(this, "Jhatkaa", "Select an input item first.");
        return;
    }

    if (m_selectedItem->itemKind() == ItemKind::InputSource) {
        m_selectedItem->toggleValue();
        refreshCircuit();
        m_statusLabel->setText("Input toggled");
    } else {
        m_statusLabel->setText("Select an input source to toggle.");
    }
}

void MainWindow::toggleInputB() {
    Q_UNUSED(m_selectedItem);
}

void MainWindow::updateSelectedGate() {
    m_updatingSelection = true;
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    qDebug() << "updateSelectedGate selectedItems count=" << selectedItems.count();
    for (QGraphicsItem* item : selectedItems) {
        qDebug() << "  selected item:" << item;
    }

    QList<GateItem*> selectedGates;
    for (QGraphicsItem* item : selectedItems) {
        if (auto* gate = qgraphicsitem_cast<GateItem*>(item)) {
            selectedGates.append(gate);
        }
    }

    if (!selectedGates.isEmpty()) {
        m_selectedItem = selectedGates.last();
    } else {
        m_selectedItem = nullptr;
    }

    const int selectedCount = selectedGates.size();
    if (selectedCount == 1 && m_selectedItem) {
        const QString label = m_selectedItem->itemKind() == ItemKind::Gate
            ? QString::fromStdString(LogicEngine::gateName(m_selectedItem->gateType()))
            : (m_selectedItem->itemKind() == ItemKind::InputSource ? "INPUT" : "OUTPUT");
        m_statusLabel->setText(QString("Selected %1").arg(label));
    } else if (selectedCount == 2) {
        m_statusLabel->setText("Two items selected. Press Connect selected to wire them.");
    } else if (selectedCount > 2) {
        m_statusLabel->setText(QString("%1 items selected").arg(selectedCount));
    } else {
        m_statusLabel->setText("Drag inputs, gates, or outputs from the palette");
    }

    m_connectButton->setEnabled(selectedCount == 2);
    m_deleteButton->setEnabled(selectedCount >= 1);
    m_toggleAButton->setEnabled(m_selectedItem && m_selectedItem->itemKind() == ItemKind::InputSource);

    refreshTruthTable();
    m_updatingSelection = false;
}

void MainWindow::connectSelectedItems() {
    const QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    QList<GateItem*> selectedGates;
    for (QGraphicsItem* item : selectedItems) {
        if (auto* gate = qgraphicsitem_cast<GateItem*>(item)) {
            selectedGates.append(gate);
        }
    }

    if (selectedGates.size() != 2) {
        QMessageBox::information(this, "Jhatkaa", "Select exactly two items to connect.");
        return;
    }

    GateItem* item1 = selectedGates[0];
    GateItem* item2 = selectedGates[1];
    GateItem* source = nullptr;
    GateItem* target = nullptr;

    const auto isSourceKind = [](GateItem* gate) {
        return gate->itemKind() == ItemKind::InputSource || gate->itemKind() == ItemKind::Gate;
    };
    const auto isTargetKind = [](GateItem* gate) {
        return gate->itemKind() == ItemKind::Gate || gate->itemKind() == ItemKind::OutputSink;
    };

    if (item1->itemKind() == ItemKind::Gate && item2->itemKind() == ItemKind::Gate) {
        if (m_selectedItem == item1 || m_selectedItem == item2) {
            target = m_selectedItem;
            source = (target == item1) ? item2 : item1;
        } else {
            target = item2;
            source = item1;
        }
    } else if (isSourceKind(item1) && isTargetKind(item2)) {
        source = item1;
        target = item2;
    } else if (isSourceKind(item2) && isTargetKind(item1)) {
        source = item2;
        target = item1;
    }

    qDebug() << "connectSelectedItems source/target" << source << target << "item1=" << item1 << "item2=" << item2;
    if (!source || !target || source == target) {
        QMessageBox::information(this, "Jhatkaa", "Cannot connect the selected items. Select a source and a target.");
        return;
    }

    int inputSlot = 0;
    qDebug() << "connectSelectedItems target kind=" << static_cast<int>(target->itemKind()) << "gateType=" << static_cast<int>(target->gateType());
    if (target->itemKind() == ItemKind::OutputSink) {
        inputSlot = 0;
    } else if (target->gateType() == GateType::NOT) {
        inputSlot = 0;
    } else if (!target->hasInputSource(0)) {
        inputSlot = 0;
    } else if (!target->hasInputSource(1)) {
        inputSlot = 1;
    } else {
        QMessageBox::information(this, "Jhatkaa", "Selected gate has no available input slots.");
        return;
    }

    m_updatingSelection = true;
    bool success = addConnection(source, target, inputSlot);
    if (success) {
        const auto labelFor = [](GateItem* item) {
            if (item->itemKind() == ItemKind::InputSource) return QString("INPUT");
            if (item->itemKind() == ItemKind::OutputSink) return QString("OUTPUT");
            return QString::fromStdString(LogicEngine::gateName(item->gateType()));
        };
        m_statusLabel->setText(QString("Connected %1 to %2").arg(labelFor(source), labelFor(target)));
    } else {
        QMessageBox::information(this, "Jhatkaa", "Failed to connect the selected items.");
    }
    m_updatingSelection = false;
}
