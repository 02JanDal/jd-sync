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
#include <QSortFilterProxyModel>
#include <QMessageBox>

#include <jd-util/Functional.h>
#include <jd-util/Util.h>
#include <jd-util/Exception.h>

#include "client/lists/SyncedList.h"
#include "client/lists/FilteredList.h"
#include "client/lists/ChangeTrackingList.h"
#include "client/lists/RecordListModel.h"

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
	: QObject(parent),
	  m_list(list),
	  m_filteredList(new FilteredList(m_list, this)),
	  m_changeTrackingList(new ChangeTrackingList(m_filteredList, this)),
	  m_model(new RecordListModel(m_changeTrackingList, this))
{
	connect(m_changeTrackingList, &AbstractRecordList::added, this, &SyncedWidgetsGroup::recordAdded);
	connect(m_changeTrackingList, &AbstractRecordList::removed, this, &SyncedWidgetsGroup::recordRemoved);
	connect(m_changeTrackingList, &AbstractRecordList::changed, this, &SyncedWidgetsGroup::recordChanged);
	connect(m_changeTrackingList, &ChangeTrackingList::modifiedChanged, this, [this]() { emit modifiedChanged(isModified()); });
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

void SyncedWidgetsGroup::registerAddButton(QPushButton *btn)
{
	connect(btn, &QPushButton::clicked, this, [this]()
	{
		const QVariantHash values = defaultValues();
		const QUuid id = values.value("id").toUuid();
		m_changeTrackingList->add(values);

		const QModelIndex index = m_model->indexForId(id);
		if (m_selector->inherits("QComboBox")) {
			qobject_cast<QComboBox *>(m_selector)->setCurrentIndex(index.row());
		} else if (m_selector->inherits("QAbstractItemView")) {
			qobject_cast<QAbstractItemView *>(m_selector)->setCurrentIndex(index);
		}
	});
}
void SyncedWidgetsGroup::registerRemoveButton(QPushButton *btn)
{
	connect(btn, &QPushButton::clicked, this, [this]()
	{
		if (!m_id.isNull()) {
			m_changeTrackingList->remove(m_id);
		}
	});
	JD::Util::applyProperty(this, &SyncedWidgetsGroup::id, &SyncedWidgetsGroup::idChanged, btn, [this, btn](const QUuid &id) {
		btn->setEnabled(!id.isNull());
	});
}
void SyncedWidgetsGroup::registerCopyButton(QPushButton *btn)
{
	connect(btn, &QPushButton::clicked, this, [this]()
	{
		if (!m_id.isNull()) {
			const QVariantHash values = JD::Util::mergeAssociative(m_changeTrackingList->get(m_id), defaultValues());
			const QUuid id = values.value("id").toUuid();
			m_changeTrackingList->add(values);

			const QModelIndex index = m_model->indexForId(id);
			if (m_selector->inherits("QComboBox")) {
				qobject_cast<QComboBox *>(m_selector)->setCurrentIndex(index.row());
			} else if (m_selector->inherits("QAbstractItemView")) {
				qobject_cast<QAbstractItemView *>(m_selector)->setCurrentIndex(index);
			}
		}
	});
	connect(this, &SyncedWidgetsGroup::idChanged, btn, [btn](const QUuid &id) { btn->setEnabled(!id.isNull()); });
	btn->setEnabled(!m_id.isNull());
}
void SyncedWidgetsGroup::registerMoveButtons(QPushButton *up, QPushButton *down, const QString &property)
{
	m_orderProperty = property;
	connect(up, &QPushButton::clicked, this, [this]()
	{
		if (m_id.isNull()) {
			return;
		}
		const QVariantHash current = m_changeTrackingList->get(m_id);

		// get all others in same context and sort them
		QVector<QVariantHash> others = m_changeTrackingList->get(Filter(QVector<FilterPart>()
																		<< FilterPart(m_parentPropertySelector, FilterPart::Equal, current.value(m_parentPropertySelector)) // same context
																		<< FilterPart("id", FilterPart::Equal, current.value("id"), true), // only others
																		QVector<FilterGroup>()));
		std::sort(others.begin(), others.end(), [this](const QVariantHash &a, const QVariantHash &b)
		{
			return a.value(m_orderProperty) < b.value(m_orderProperty);
		});

		// find our the higher element, and swap with it
		for (int i = 0; i < others.size(); ++i) {
			if (others.at(i).value(m_orderProperty) > current.value(m_orderProperty)) {
				m_changeTrackingList->set(m_id, m_orderProperty, others.at(i).value(m_orderProperty));
				m_changeTrackingList->set(others.at(i).value("id").toUuid(), m_orderProperty, current.value(m_orderProperty));
				break;
			}
		}
	});
	connect(down, &QPushButton::clicked, this, [this]()
	{
		if (m_id.isNull()) {
			return;
		}
		const QVariantHash current = m_changeTrackingList->get(m_id);

		// get all others in same context and sort them
		QVector<QVariantHash> others = m_changeTrackingList->get(Filter(QVector<FilterPart>()
																		<< FilterPart(m_parentPropertySelector, FilterPart::Equal, current.value(m_parentPropertySelector)) // same context
																		<< FilterPart("id", FilterPart::Equal, current.value("id"), true), // only others
																		QVector<FilterGroup>()));
		std::sort(others.begin(), others.end(), [this](const QVariantHash &a, const QVariantHash &b)
		{
			return a.value(m_orderProperty) < b.value(m_orderProperty);
		});

		// find our the lower element, and swap with it
		for (int i = others.size()-1; i >= 0; --i) {
			if (others.at(i).value(m_orderProperty) < current.value(m_orderProperty)) {
				m_changeTrackingList->set(m_id, m_orderProperty, others.at(i).value(m_orderProperty));
				m_changeTrackingList->set(others.at(i).value("id").toUuid(), m_orderProperty, current.value(m_orderProperty));
				break;
			}
		}
	});

	// TODO: disable button if at top or at bottom
	connect(this, &SyncedWidgetsGroup::idChanged, up, [up](const QUuid &id) { up->setEnabled(!id.isNull()); });
	connect(this, &SyncedWidgetsGroup::idChanged, down, [down](const QUuid &id) { down->setEnabled(!id.isNull()); });
	up->setEnabled(!m_id.isNull());
	down->setEnabled(!m_id.isNull());
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
void SyncedWidgetsGroup::setSelector(const QUuid &id)
{
	m_filteredList->setFilter(Filter(FilterPart("id", FilterPart::Equal, id)));
	m_list->setFocus(m_filteredList->filter());
	setId(id);
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
	box->setModel(group->m_model);
	group->setSelector(otherProperty, box);
	m_subgroups.append(group);
	connect(group, &SyncedWidgetsGroup::modifiedChanged, this, [this]() { emit modifiedChanged(isModified()); });
	return group;
}
SyncedWidgetsGroup *SyncedWidgetsGroup::add(const QString &otherProperty, QAbstractItemView *view, SyncedList *list)
{
	SyncedWidgetsGroup *group = new SyncedWidgetsGroup(list, this);
	view->setModel(group->m_model);
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
	if (m_changeTrackingList->isModified()) {
		return true;
	}
	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		if (subgroup->isModified()) {
			return true;
		}
	}
	return false;
}

class SortedRecordModel : public QSortFilterProxyModel
{
	QString m_property;
public:
	explicit SortedRecordModel(const QString &property, QObject *parent = nullptr)
		: QSortFilterProxyModel(parent), m_property(property) {}

private:
	bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
	{
		const QVariantHash dataLeft = sourceLeft.data(RecordListModel::AllDataRole).toHash();
		const QVariantHash dataRight = sourceRight.data(RecordListModel::AllDataRole).toHash();
		return dataLeft.value(m_property) < dataRight.value(m_property);
	}
};
void SyncedWidgetsGroup::sortSelector(const QString &property)
{
	injectProxyModel(new SortedRecordModel(property, this));
}

void SyncedWidgetsGroup::injectProxyModel(QAbstractProxyModel *proxy)
{
	if (!m_selector) {
		return;
	}

	proxy->setParent(this);
	if (QComboBox *box = qobject_cast<QComboBox *>(m_selector)) {
		proxy->setSourceModel(box->model());
		box->setModel(proxy);
		connect(box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this, box](const int index) { setId(m_model->idForIndex(m_model->index(index))); });
	} else if (QAbstractItemView *view = qobject_cast<QAbstractItemView *>(m_selector)) {
		proxy->setSourceModel(view->model());
		view->setModel(proxy);
		connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &index) { setId(m_model->idForIndex(index)); });
	}
}

void SyncedWidgetsGroup::discard()
{
	m_changeTrackingList->discard();

	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->discard();
	}
}
void SyncedWidgetsGroup::commit()
{
	try {
		commitInternal();
	} catch (Exception &e) {
		QMessageBox::critical(qobject_cast<QWidget *>(parent()), tr("Commit failed"), tr("Committing changes failed: %1").arg(e.cause()));
	}
}

void SyncedWidgetsGroup::setId(const QUuid &id)
{
	if (id == m_id) {
		return;
	}
	m_id = id;

	if (!m_id.isNull()) {
		if (m_changeTrackingList->contains(id) || m_changeTrackingList->isAddition(id)) {
			for (SyncedWidgetsGroupEntry *entry : m_entries) {
				entry->setValue(m_changeTrackingList->get(id, entry->m_property));
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

	emit idChanged(m_id);
}
void SyncedWidgetsGroup::setParentId(const QUuid &id)
{
	m_parentId = id;
	m_filteredList->setFilter(Filter(FilterPart(m_parentPropertySelector, FilterPart::Equal, id)));
	m_list->setFocus(m_filteredList->filter());
}

void SyncedWidgetsGroup::recordAdded(const QUuid &id)
{
	if (id == m_id) {
		for (SyncedWidgetsGroupEntry *entry : m_entries) {
			entry->setValue(m_list->get(id, entry->m_property));
		}
		setEnabled(true);
	}
}
void SyncedWidgetsGroup::recordRemoved(const QUuid &id)
{
	if (id == m_id) {
		setId(QUuid());
	}
}
void SyncedWidgetsGroup::recordChanged(const QUuid &id, const QString &property)
{
	if (id == m_id) {
		for (SyncedWidgetsGroupEntry *entry : m_entries) {
			if (entry->m_property == property) {
				entry->setValue(m_changeTrackingList->get(m_id, property));
			}
		}
	}
}

void SyncedWidgetsGroup::edited(SyncedWidgetsGroupEntry *entry)
{
	m_changeTrackingList->set(m_id, entry->m_property, entry->value());
}

void SyncedWidgetsGroup::commitInternal()
{
	m_changeTrackingList->commit();

	for (SyncedWidgetsGroup *subgroup : m_subgroups) {
		subgroup->commitInternal();
	}
}

QVariantHash SyncedWidgetsGroup::defaultValues() const
{
	QVariantHash values;
	values.insert("id", QUuid::createUuid());
	if (!m_parentId.isNull()) {
		values.insert(m_parentPropertySelector, m_parentId);
	}
	if (!m_orderProperty.isNull()) {
		const QVector<QVariantHash> existing = m_changeTrackingList->get(Filter(FilterPart(m_parentPropertySelector, FilterPart::Equal, m_parentId)));
		const QVariant max = JD::Util::Functional::collection(existing).map([this](const QVariantHash &hash) { return hash.value(m_orderProperty); }).max();
		values.insert(m_orderProperty, max.toInt()+1);
	}
	return values;
}
