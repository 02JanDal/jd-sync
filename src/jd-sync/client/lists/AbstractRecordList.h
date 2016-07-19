#pragma once

#include <QObject>

class Filter;
class Table;

class AbstractRecordList : public QObject
{
	Q_OBJECT
public:
	explicit AbstractRecordList(AbstractRecordList *parentList, QObject *parent = nullptr);
	virtual ~AbstractRecordList() {}

	QVariant get(const QUuid &id, const QString &property) const;
	QVector<QVariantHash> get(const Filter &filter) const;
	virtual QVariantHash get(const QUuid &id) const = 0;

	void set(const QUuid &id, const QString &property, const QVariant &value);
	void set(const QUuid &id, const QVariantHash &properties);
	virtual void set(const QVector<QVariantHash> &properties) = 0;

	void add(const QVariantHash &values);
	virtual void add(const QVector<QVariantHash> &values) = 0;
	void remove(const QUuid &id);
	virtual void remove(const QSet<QUuid> &ids) = 0;

	virtual bool contains(const QUuid &id) const = 0;
	virtual QList<QUuid> ids() const = 0;

signals:
	void added(const QUuid &id);
	void removed(const QUuid &id);
	void changed(const QUuid &id, const QString &property);

protected:
	AbstractRecordList *m_parent = nullptr;
};

#define ABSTRACTRECORDLIST_USING_COMFORT_OVERLOADS \
	using AbstractRecordList::get; \
	using AbstractRecordList::set; \
	using AbstractRecordList::add; \
	using AbstractRecordList::remove;
