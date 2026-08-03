#include "tripeditdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/appdatetimeedit.h"

namespace {

QDoubleValidator* kmValidator(QObject* parent)
{
    auto* v = new QDoubleValidator(0.0, 100000.0, 2, parent);
    v->setNotation(QDoubleValidator::StandardNotation);
    return v;
}

} // namespace

TripEditDialog::TripEditDialog(const QList<TransportMode>& modes, const QList<TripTag>& tags,
                               const Trip& existing, QWidget* parent)
    : QDialog(parent)
    , m_modes(modes)
    , m_tags(tags)
    , m_existing(existing)
{
    setWindowTitle(m_existing.id.isEmpty() ? QStringLiteral("记一段行程")
                                           : QStringLiteral("编辑行程"));
    buildUI();

    // 预填数据（编辑）
    if (!m_existing.id.isEmpty()) {
        const int modeIdx = m_modeCombo->findData(m_existing.modeCode);
        if (modeIdx >= 0)
            m_modeCombo->setCurrentIndex(modeIdx);

        m_startPlace->setText(m_existing.startPlace);
        m_endPlace->setText(m_existing.endPlace);
        m_startTime->setDateTime(m_existing.startTime.toLocalTime());
        if (m_existing.isInProgress()) {
            m_inProgress->setChecked(true);
            m_endTime->setEnabled(false);
        } else {
            m_endTime->setDateTime(m_existing.endTime.toLocalTime());
        }
        if (m_existing.distanceM)
            m_distance->setText(QString::number(*m_existing.distanceM / 1000.0, 'f', 2));
        if (m_existing.costFen)
            m_cost->setText(QString::number(*m_existing.costFen / 100.0, 'f', 2));
        const int tagIdx = m_tagCombo->findData(m_existing.tagId);
        if (tagIdx >= 0)
            m_tagCombo->setCurrentIndex(tagIdx);
        m_note->setText(m_existing.note);
    }
}

void TripEditDialog::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(10);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    // 交通方式（纯文本，不带图标）
    m_modeCombo = new QComboBox(this);
    for (const TransportMode& m : m_modes)
        m_modeCombo->addItem(m.name, m.code);
    if (m_modeCombo->count() == 0)
        m_modeCombo->addItem(QStringLiteral("其他"), QStringLiteral("OTHER"));
    form->addRow(QStringLiteral("交通方式"), m_modeCombo);

    // 起终点
    m_startPlace = new QLineEdit(this);
    m_startPlace->setPlaceholderText(QStringLiteral("如：海淀黄庄"));
    m_endPlace = new QLineEdit(this);
    m_endPlace->setPlaceholderText(QStringLiteral("如：北京大学东门"));
    form->addRow(QStringLiteral("起点"), m_startPlace);
    form->addRow(QStringLiteral("终点"), m_endPlace);

    // 时间（AppDateTimeEdit 自带与 QComboBox 同款下拉箭头）
    m_startTime = new AppDateTimeEdit(QDateTime::currentDateTime(), this);
    m_startTime->setCalendarPopup(true);
    m_startTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_endTime = new AppDateTimeEdit(QDateTime::currentDateTime(), this);
    m_endTime->setCalendarPopup(true);
    m_endTime->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_inProgress = new QCheckBox(QStringLiteral("进行中（未到达）"), this);
    form->addRow(QStringLiteral("开始时间"), m_startTime);
    form->addRow(QStringLiteral("结束时间"), m_endTime);
    form->addRow(QString(), m_inProgress);

    // 里程 / 费用（单位：km、元，保存时换算成 m、分）
    m_distance = new QLineEdit(this);
    m_distance->setPlaceholderText(QStringLiteral("可留空"));
    m_distance->setValidator(kmValidator(this));
    m_cost = new QLineEdit(this);
    m_cost->setPlaceholderText(QStringLiteral("可留空"));
    m_cost->setValidator(kmValidator(this));
    form->addRow(QStringLiteral("里程 (km)"), m_distance);
    form->addRow(QStringLiteral("费用 (元)"), m_cost);

    // 标签
    m_tagCombo = new QComboBox(this);
    m_tagCombo->addItem(QStringLiteral("无标签"), QString());
    for (const TripTag& t : m_tags)
        m_tagCombo->addItem(t.name, t.code);
    form->addRow(QStringLiteral("标签"), m_tagCombo);

    // 备注
    m_note = new QLineEdit(this);
    m_note->setPlaceholderText(QStringLiteral("可选"));
    form->addRow(QStringLiteral("备注"), m_note);

    root->addLayout(form);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("hintLabel"));
    m_errorLabel->setStyleSheet(QStringLiteral("color: #E24B4A;"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    auto* saveBtn = buttons->button(QDialogButtonBox::Save);
    saveBtn->setText(QStringLiteral("保存"));
    saveBtn->setObjectName(QStringLiteral("primaryButton"));
    auto* cancelBtn = buttons->button(QDialogButtonBox::Cancel);
    cancelBtn->setText(QStringLiteral("取消"));
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &TripEditDialog::onSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_inProgress, &QCheckBox::toggled, this, &TripEditDialog::onToggleInProgress);
}

void TripEditDialog::onToggleInProgress(bool checked)
{
    m_endTime->setEnabled(!checked);
    if (checked)
        m_endTime->clear();
}

bool TripEditDialog::validate(QString& error) const
{
    const bool hasPlace = !m_startPlace->text().trimmed().isEmpty()
                       || !m_endPlace->text().trimmed().isEmpty();
    const bool hasOther = !m_distance->text().trimmed().isEmpty()
                       || !m_cost->text().trimmed().isEmpty()
                       || !m_note->text().trimmed().isEmpty()
                       || !m_tagCombo->currentData().toString().isEmpty();
    if (!hasPlace && !hasOther) {
        error = QStringLiteral("请至少填写起点/终点、或里程、费用、标签、备注中的一项");
        return false;
    }

    if (!m_inProgress->isChecked() && m_endTime->dateTime() < m_startTime->dateTime()) {
        error = QStringLiteral("结束时间不能早于开始时间");
        return false;
    }
    return true;
}

void TripEditDialog::onSave()
{
    QString error;
    if (!validate(error)) {
        m_errorLabel->setText(error);
        m_errorLabel->show();
        return;
    }

    m_result.id = m_existing.id;
    m_result.userId = m_existing.userId;
    m_result.modeCode = m_modeCombo->currentData().toString();
    m_result.startTime = m_startTime->dateTime().toUTC();
    if (m_inProgress->isChecked())
        m_result.endTime = QDateTime();
    else
        m_result.endTime = m_endTime->dateTime().toUTC();
    m_result.startPlace = m_startPlace->text().trimmed();
    m_result.endPlace = m_endPlace->text().trimmed();

    const QString distText = m_distance->text().trimmed();
    if (!distText.isEmpty()) {
        bool ok = false;
        const double km = distText.toDouble(&ok);
        if (ok)
            m_result.distanceM = qRound64(km * 1000.0);
    }
    const QString costText = m_cost->text().trimmed();
    if (!costText.isEmpty()) {
        bool ok = false;
        const double yuan = costText.toDouble(&ok);
        if (ok)
            m_result.costFen = qRound64(yuan * 100.0);
    }

    m_result.tagId = m_tagCombo->currentData().toString();
    m_result.note = m_note->text().trimmed();
    m_result.createdAt = m_existing.createdAt;

    accept();
}

Trip TripEditDialog::trip() const
{
    return m_result;
}
