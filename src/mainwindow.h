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
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <vector>

#include "gateitem.h"

struct Connection {
    QPointer<GateItem> source;
    QPointer<GateItem> target;
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
    void connectSelectedItems();
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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QPointer<GateItem> m_selectedItem;
    QLabel* m_statusLabel;
    QPushButton* m_toggleAButton;
    QPushButton* m_toggleBButton;
    QPushButton* m_connectButton;
    QPushButton* m_deleteButton;
    QListWidget* m_gatePalette;
    QTableWidget* m_truthTable;
    std::vector<Connection> m_connections;
    bool m_refreshingCircuit;
    bool m_updatingSelection;
};
