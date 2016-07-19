#pragma once

#include "AbstractRecordList.h"

#include <QUuid>

#include "jd-sync/common/Filter.h"

class FilteredList : public AbstractRecordList
{
	Q_OBJECT
public:
	explicit FilteredList(AbstractRecordList *parentList, QObject *parent = nullptr);

	Filter filter() const { return m_filter; }
	void setFilter(const Filter &filter);

	QVariantHash get(const QUuid &id) const override { return m_parent->get(id); }
	void set(const QVector<QVariantHash> &properties) override { m_parent->set(properties); }
	void add(const QVector<QVariantHash> &properties) override { m_parent->add(properties); }
	void remove(const QSet<QUuid> &ids) override { m_parent->remove(ids); }
	ABSTRACTRECORDLIST_USING_COMFORT_OVERLOADS

	bool contains(const QUuid &id) const override { return m_ids.contains(id); }
	QList<QUuid> ids() const override { return m_ids; }

private slots:
	void addedInParent(const QUuid &id);
	void removedInParent(const QUuid &id);
	void changedInParent(const QUuid &id, const QString &property);

private:
	Filter m_filter;
	QList<QUuid> m_ids;

	void refilter();
};
