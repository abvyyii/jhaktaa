#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPointF>
#include <QPushButton>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <vector>

#include "gateitem.h"

struct Connection {
    GateItem* source = nullptr;
    GateItem* target = nullptr;
    int inputSlot = -1;
    QGraphicsLineItem* line = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void toggleInputA();
    void toggleInputB();
    void updateSelectedGate();
    void startWireToInputA();
    void startWireToInputB();
    void deleteSelectedItem();

private:
    void createToolbar();
    void createCanvas();
    void createInspector();
    void refreshCircuit();
    void refreshTruthTable();
    void updateWirePositions();
    bool addConnection(GateItem* source, GateItem* target, int inputSlot);
    void clearConnections();
    void addSceneItem(const QString& type, const QPointF& scenePos);
    void removeItemWithConnections(GateItem* item);
    void onNodeClicked(int slot, bool output);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    GateItem* m_selectedItem;
    GateItem* m_pendingSourceItem;
    QLabel* m_statusLabel;
    QPushButton* m_toggleAButton;
    QPushButton* m_toggleBButton;
    QPushButton* m_connectAButton;
    QPushButton* m_connectBButton;
    QPushButton* m_deleteButton;
    QListWidget* m_gatePalette;
    QTableWidget* m_truthTable;
    std::vector<Connection> m_connections;
    int m_pendingInputSlot;
    bool m_refreshingCircuit;
};
