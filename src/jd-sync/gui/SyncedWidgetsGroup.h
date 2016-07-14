#pragma once

#include <QWidget>

class SyncedList;
class SyncedListModel;
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

class SyncedWidgetsGroup : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
	Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
public:
	explicit SyncedWidgetsGroup(SyncedList *list, QObject *parent = nullptr);
	~SyncedWidgetsGroup();

	void registerButtons(QPushButton *saveBtn, QPushButton *resetBtn, QPushButton *discardBtn);
	void setSelector(const QString &otherProperty, QComboBox *box);
	void setSelector(const QString &otherProperty, QAbstractItemView *view);
	void setSelector(const int id);

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
	int id() const { return m_id; }

signals:
	void modifiedChanged(const bool modified);
	void idChanged(const int id);

	void discarded();
	void committed();

public slots:
	void discard();
	void commit();

private slots:
	void setId(const int id);
	void setParentId(const int id);

	void recordAdded(const int id);
	void recordRemoved(const int id);
	void recordChanged(const int id, const QString &property);

private:
	SyncedList *m_list;
	SyncedListModel *m_model;
	int m_id = -1;

	// either m_selector or m_id should be used
	QWidget *m_selector = nullptr;
	QString m_parentPropertySelector;

	friend class SyncedWidgetsGroupEntry;
	void edited(SyncedWidgetsGroupEntry *entry);

	QVector<SyncedWidgetsGroupEntry *> m_entries;
	QVector<SyncedWidgetsGroup *> m_subgroups;
	QVector<QWidget *> m_others;
};