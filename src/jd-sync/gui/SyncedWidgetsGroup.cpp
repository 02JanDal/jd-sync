#include "SyncedWidgetsGroup.h"

#include <QLineEdit>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QLabel>
#include <QGroupBox>

#include "client/SyncedList.h"

inline static uint qHash(const QVariant &value, uint seed = 0)
{
	return qHash(value.toString(), seed);
}

class SyncedWidgetsGroupEntry
{
public:
	explicit SyncedWidgetsGroupEntry(const QString &property, SyncedWidgetsGroup *group)
		: m_property(property), m_group(group) {}
	virtual ~SyncedWidgetsGroupEntry() {}

	virtual QVariant value() const = 0;
	virtual void setValue(const QVariant &value) = 0;
	virtual void setEnabled(const bool enabled) = 0;

protected:
	friend class SyncedWidgetsGroup;
	QString m_property;
	SyncedWidgetsGroup *m_group;
	bool m_doNotify = true;

	void notifyEdited()
	{
		if (m_doNotify) {
			m_group->edited(this);
		}
	}
};
class LabelEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit LabelEntry(QLabel *label, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_label(label) {}

	QVariant value() const override { return m_label->text(); }
	void setValue(const QVariant &value) override { m_label->setText(value.toString()); }
	void setEnabled(const bool enabled) override { m_label->setEnabled(enabled); }

private:
	QLabel *m_label;
};
class GroupBoxEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit GroupBoxEntry(QGroupBox *box, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_box(box) {}

	QVariant value() const override { return m_box->title(); }
	void setValue(const QVariant &value) override { m_box->setTitle(value.toString()); }
	void setEnabled(const bool enabled) override { m_box->setEnabled(enabled); }

private:
	QGroupBox *m_box;
};
class LineEditEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit LineEditEntry(QLineEdit *edit, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_edit(edit)
	{
		QObject::connect(m_edit, &QLineEdit::textEdited, group, [this]() { notifyEdited(); });
	}

	QVariant value() const override { return m_edit->text(); }
	void setValue(const QVariant &value) override { m_doNotify = false; m_edit->setText(value.toString()); m_doNotify = true; }
	void setEnabled(const bool enabled) override { m_edit->setEnabled(enabled); }

private:
	QLineEdit *m_edit;
};
class DateEditEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit DateEditEntry(QDateEdit *edit, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_edit(edit)
	{
		QObject::connect(m_edit, &QDateEdit::dateChanged, group, [this]() { notifyEdited(); });
	}

	QVariant value() const override { return m_edit->date(); }
	void setValue(const QVariant &value) override { m_doNotify = false; m_edit->setDate(value.toDate()); m_doNotify = true; }
	void setEnabled(const bool enabled) override { m_edit->setEnabled(enabled); }

private:
	QDateEdit *m_edit;
};
class DateTimeEditEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit DateTimeEditEntry(QDateTimeEdit *edit, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_edit(edit)
	{
		QObject::connect(m_edit, &QDateTimeEdit::dateTimeChanged, group, [this]() { notifyEdited(); });
	}

	QVariant value() const override { return m_edit->dateTime(); }
	void setValue(const QVariant &value) override { m_doNotify = false; m_edit->setDateTime(value.toDateTime()); m_doNotify = true; }
	void setEnabled(const bool enabled) override { m_edit->setEnabled(enabled); }

private:
	QDateTimeEdit *m_edit;
};
class CheckBoxEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit CheckBoxEntry(QCheckBox *box, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_box(box)
	{
		QObject::connect(m_box, &QCheckBox::clicked, group, [this]() { notifyEdited(); });
	}

	QVariant value() const override { return m_box->isChecked(); }
	void setValue(const QVariant &value) override { m_doNotify = false; m_box->setChecked(value.toBool()); m_doNotify = true; }
	void setEnabled(const bool enabled) override { m_box->setEnabled(enabled); }

private:
	QCheckBox *m_box;
};
class RadioButtonsEntry : public SyncedWidgetsGroupEntry
{
public:
	explicit RadioButtonsEntry(const QHash<QVariant, QRadioButton *> &buttons, const QString &property, SyncedWidgetsGroup *group)
		: SyncedWidgetsGroupEntry(property, group), m_buttons(buttons)
	{
		for (QRadioButton *button : m_buttons) {
			QObject::connect(button, &QRadioButton::clicked, group, [this]() { notifyEdited(); });
		}
	}

	QVariant value() const override
	{
		for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
			if (it.value()->isChecked()) {
				return it.key();
			}
		}
		return QVariant();
	}
	void setValue(const QVariant &value) override
	{
		m_doNotify = false;
		if (m_buttons.contains(value)) {
			m_buttons.value(value)->setChecked(true);
		}
		m_doNotify = true;
	}
	void setEnabled(const bool enabled) override
	{
		for (QRadioButton *btn : m_buttons) {
			btn->setEnabled(enabled);
		}
	}

private:
	QHash<QVariant, QRadioButton *> m_buttons;
};

SyncedWidgetsGroup::SyncedWidgetsGroup(SyncedList *list, QObject *parent)
	: QObject(parent), m_list(list), m_model(new SyncedListModel(list, this))
{
	connect(m_model, &SyncedListModel::added, this, &SyncedWidgetsGroup::recordAdded);
	connect(m_model, &SyncedListModel::removed, this, &SyncedWidgetsGroup::recordRemoved);
	connect(m_model, &SyncedListModel::changed, this, &SyncedWidgetsGroup::recordChanged);
	connect(m_model, &SyncedListModel::modifiedChanged, this, [this]() { emit modifiedChanged(isModified()); });
}
SyncedWidgetsGroup::~SyncedWidgetsGroup()
{
	qDeleteAll(m_entries);
}

void SyncedWidgetsGroup::registerButtons(QPushButton *saveBtn, QPushButton *resetBtn, QPushButton *discardBtn)
{
	if (saveBtn) {
		connect(saveBtn, &QPushButton::clicked, this, &SyncedWidgetsGroup::commit);
		connect(this, &SyncedWidgetsGroup::modifiedChanged, saveBtn, &QPushButton::setEnabled);
		saveBtn->setEnabled(isModified());
	}
	if (resetBtn) {
		connect(resetBtn, &QPushButton::clicked, this, &SyncedWidgetsGroup::discard);
		connect(this, &SyncedWidgetsGroup::modifiedChanged, resetBtn, &QPushButton::setEnabled);
		resetBtn->setEnabled(isModified());
	}
	if (discardBtn) {
		connect(discardBtn, &QPushButton::clicked, this, &SyncedWidgetsGroup::discard);
		connect(this, &SyncedWidgetsGroup::modifiedChanged, discardBtn, &QPushButton::setEnabled);
		discardBtn->setEnabled(isModified());
	}
}

void SyncedWidgetsGroup::setSelector(const QString &otherProperty, QComboBox *box)
{
	m_parentPropertySelector = otherProperty;
	m_selector = box;
	box->setModel(m_model);
	connect(box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this, box](const int index) { setId(m_model->idForIndex(m_model->index(index))); });
}
void SyncedWidgetsGroup::setSelector(const QString &otherProperty, QAbstractItemView *view)
{
	m_parentPropertySelector = otherProperty;
	m_selector = view;
	view->setModel(m_model);
	connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &index) { setId(m_model->idForIndex(index)); });
}
void SyncedWidgetsGroup::setSelector(const int id)
{
	m_model->setFilter(Filter(FilterPart("id", FilterPart::Equal, id)));
	setId(m_id = id);
}

void SyncedWidgetsGroup::add(const QString &property, QLabel *label)
{
	m_entries.append(new LabelEntry(label, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, QGroupBox *box)
{
	m_entries.append(new GroupBoxEntry(box, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, QLineEdit *edit)
{
	m_entries.append(new LineEditEntry(edit, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, QDateEdit *edit)
{
	m_entries.append(new DateEditEntry(edit, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, QDateTimeEdit *edit)
{
	m_entries.append(new DateTimeEditEntry(edit, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, QCheckBox *box)
{
	m_entries.append(new CheckBoxEntry(box, property, this));
}
void SyncedWidgetsGroup::add(const QString &property, const QHash<QVariant, QRadioButton *> &buttons)
{
	m_entries.append(new RadioButtonsEntry(buttons, property, this));
}

SyncedWidgetsGroup *SyncedWidgetsGroup::add(const QString &otherProperty, QComboBox *box, SyncedList *list)
{
	SyncedWidgetsGroup *group = new SyncedWidgetsGroup(list, this);
	group->setSelector(otherProperty, box);
	m_subgroups.append(group);
	connect(group, &SyncedWidgetsGroup::modifiedChanged, this, [this]() { emit modifiedChanged(isModified()); });
	return group;
}
SyncedWidgetsGroup *SyncedWidgetsGroup::add(const QString &otherProperty, QAbstractItemView *view, SyncedList *list)
{
	SyncedWidgetsGroup *group = new SyncedWidgetsGroup(list, this);
	group->setSelector(otherProperty, view);
	m_subgroups.append(group);
	connect(group, &SyncedWidgetsGroup::modifiedChanged, this, [this]() { emit modifiedChanged(isModified()); });
	return group;
}

void SyncedWidgetsGroup::add(QWidget *other)
{
	m_others.append(other);
}

void SyncedWidgetsGroup::setEnabled(const bool enabled)
{
	for (SyncedWidgetsGroupEntry *entry : m_entries) {
		entry->setEnabled(enabled);
	}
	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->setEnabled(enabled);
	}
	for (QWidget *other : m_others) {
		other->setEnabled(enabled);
	}
}

bool SyncedWidgetsGroup::isModified() const
{
	if (m_model->isModified()) {
		return true;
	}
	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		if (subgroup->isModified()) {
			return true;
		}
	}
	return false;
}

void SyncedWidgetsGroup::discard()
{
	m_model->discardPreliminaries();

	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->discard();
	}

	emit discarded();
}
void SyncedWidgetsGroup::commit()
{
	m_model->commitPreliminaries();

	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->commit();
	}

	emit committed();
}

void SyncedWidgetsGroup::setId(const int id)
{
	if (id == m_id) {
		return;
	}
	m_id = id;

	if (m_id != -1) {
		if (m_list->contains(id)) {
			for (SyncedWidgetsGroupEntry *entry : m_entries) {
				entry->setValue(m_list->get(id, entry->m_property));
			}
		} else {
			m_list->fetchOnce(Filter(FilterPart("id", FilterPart::Equal, id)));
			setEnabled(false);
		}
	} else {
		setEnabled(false);
	}

	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->setParentId(m_id);
	}
}
void SyncedWidgetsGroup::setParentId(const int id)
{
	m_model->setFilter(Filter(FilterPart(m_parentPropertySelector, FilterPart::Equal, id)));
}

void SyncedWidgetsGroup::recordAdded(const int id)
{
	if (id == m_id) {
		for (SyncedWidgetsGroupEntry *entry : m_entries) {
			entry->setValue(m_list->get(id, entry->m_property));
		}
	}
}
void SyncedWidgetsGroup::recordRemoved(const int id)
{
	if (id == m_id) {
		setId(-1);
	}
}
void SyncedWidgetsGroup::recordChanged(const int id, const QString &property)
{
	if (id == m_id) {
		for (SyncedWidgetsGroupEntry *entry : m_entries) {
			if (entry->m_property == property) {
				entry->setValue(m_model->data(m_id, property));
			}
		}
	}
}

void SyncedWidgetsGroup::edited(SyncedWidgetsGroupEntry *entry)
{
	m_model->setData(m_id, entry->m_property, entry->value());
}
