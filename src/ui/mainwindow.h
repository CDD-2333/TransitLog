#pragma once

#include <QMainWindow>

#include "model/entities.h"
#include "model/triplistmodel.h"

class QLabel;
class QListView;
class QPushButton;
class QStackedWidget;
class StatsWidget;
class TripRepository;

// 主窗口：只做编排（Model/View 绑定、对话框调度），所有 SQL 在 Repository 层。
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onShowTrips();
    void onShowStats();
    void onAddTrip();
    void onEditTrip(const QModelIndex& index);
    void onDeleteTrip(const QModelIndex& index);
    void onDeleteSelected();
    void onShowContextMenu(const QPoint& pos);
    void onToggleTheme();
    void onOpenSettings();
    void onLogout();

private:
    void setupUI();
    void reloadTrips();

    TripRepository* m_tripRepo = nullptr;
    QStackedWidget* m_pages = nullptr;
    QListView* m_tripList = nullptr;
    TripListModel* m_tripModel = nullptr;
    QStackedWidget* m_emptyStack = nullptr;
    QLabel* m_tripCountLabel = nullptr;
    QLabel* m_userLabel = nullptr;
    QPushButton* m_navTrips = nullptr;
    QPushButton* m_navStats = nullptr;
    StatsWidget* m_statsWidget = nullptr;
};
