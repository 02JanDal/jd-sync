#pragma once

#include <QWidget>
#include <QUuid>

class SyncedList;
class FilteredList;
class ChangeTrackingList;
class RecordListModel;
class SyncedWidgetsGroupEntry;
class Filter;

class QLineEdit;
class QDateEdit;
class QDateTimeEdit;
class QCheckBox;
class QRadioButton;
class QComboBox;
class QAbstractItemView;
class QPushButton;
class QLabel;
class QGroupBox;
class QAbstractProxyModel;

class SyncedWidgetsGroup : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
	Q_PROPERTY(QUuid id READ id WRITE setId NOTIFY idChanged)
public:
	explicit SyncedWidgetsGroup(SyncedList *list, QObject *parent = nullptr);
	~SyncedWidgetsGroup();

	void registerButtons(QPushButton *saveBtn, QPushButton *resetBtn, QPushButton *discardBtn);
	void registerAddButton(QPushButton *btn);
	void registerRemoveButton(QPushButton *btn);
	void registerCopyButton(QPushButton *btn);
	void registerMoveButtons(QPushButton *up, QPushButton *down, const QString &property);

	void setSelector(const QString &otherProperty, QComboBox *box);
	void setSelector(const QString &otherProperty, QAbstractItemView *view);
	void setSelector(const QUuid &id);

	void add(const QString &property, QLabel *label);
	void add(const QString &property, QGroupBox *box);
	void add(const QString &property, QLineEdit *edit);
	void add(const QString &property, QDateEdit *edit);
	void add(const QString &property, QDateTimeEdit *edit);
	void add(const QString &property, QCheckBox *box);
	void add(const QString &property, const QHash<QVariant, QRadioButton *> &buttons);
	SyncedWidgetsGroup *add(const QString &otherProperty, QComboBox *box, SyncedList *list);
	SyncedWidgetsGroup *add(const QString &otherProperty, QAbstractItemView *view, SyncedList *list);
	void add(QWidget *other);

	void setEnabled(const bool enabled);

	bool isModified() const;
	QUuid id() const { return m_id; }

	RecordListModel *model() const { return m_model; }
	void sortSelector(const QString &property);
	void injectProxyModel(QAbstractProxyModel *proxy);

signals:
	void modifiedChanged(const bool modified);
	void idChanged(const QUuid &id);

public slots:
	void discard();
	void commit();

private slots:
	void setId(const QUuid &id);
	void setParentId(const QUuid &id);

	void recordAdded(const QUuid &id);
	void recordRemoved(const QUuid &id);
	void recordChanged(const QUuid &id, const QString &property);

private:
	SyncedList *m_list;
	FilteredList *m_filteredList;
	ChangeTrackingList *m_changeTrackingList;
	RecordListModel *m_model;
	QUuid m_id;

	// either m_selector or m_id should be used
	QWidget *m_selector = nullptr;
	QString m_parentPropertySelector;
	QUuid m_parentId;

	QString m_orderProperty;

	friend class SyncedWidgetsGroupEntry;
	void edited(SyncedWidgetsGroupEntry *entry);

	QVector<SyncedWidgetsGroupEntry *> m_entries;
	QVector<SyncedWidgetsGroup *> m_subgroups;
	QVector<QWidget *> m_others;

	void commitInternal();

	// for new records
	QVariantHash defaultValues() const;
};
