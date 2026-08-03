#pragma once

#include <QDialog>
#include <QList>

#include "model/entities.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
class AppDateTimeEdit;

// 新增 / 编辑行程对话框。只负责收集输入并做基本校验，写库由外部 Repository 完成。
class TripEditDialog : public QDialog {
    Q_OBJECT
public:
    TripEditDialog(const QList<TransportMode>& modes, const QList<TripTag>& tags,
                   const Trip& existing, QWidget* parent = nullptr);

    Trip trip() const;   // 校验通过后调用，返回填好的 Trip（单位已换算）

private slots:
    void onSave();
    void onToggleInProgress(bool checked);
    void goNext();    // 第 1 页 -> 第 2 页（车次/车型）
    void goBack();

private:
    void buildUI();
    bool validate(QString& error) const;   // 第 1 页校验（仅时间合法性）

    QList<TransportMode> m_modes;
    QList<TripTag> m_tags;
    Trip m_existing;
    Trip m_result;

    QStackedWidget* m_stack = nullptr;

    QComboBox* m_modeCombo = nullptr;
    QLineEdit* m_startPlace = nullptr;
    QLineEdit* m_endPlace = nullptr;
    AppDateTimeEdit* m_startTime = nullptr;
    AppDateTimeEdit* m_endTime = nullptr;
    QCheckBox* m_inProgress = nullptr;
    QLineEdit* m_distance = nullptr;
    QLineEdit* m_cost = nullptr;
    QComboBox* m_tagCombo = nullptr;
    QLineEdit* m_vehicleNo = nullptr;      // 车次/航班号（可空）
    QLineEdit* m_vehicleModel = nullptr;   // 车型（可空）
    QLineEdit* m_note = nullptr;
    QLabel* m_errorLabel = nullptr;
};
