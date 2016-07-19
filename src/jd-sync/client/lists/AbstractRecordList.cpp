#include "AbstractRecordList.h"

#include <QVariant>
#include <QVector>
#include <QSet>
#include <QUuid>

#include "common/Filter.h"

AbstractRecordList::AbstractRecordList(AbstractRecordList *parentList, QObject *parent)
	: QObject(parent), m_parent(parentList) {}

QVariant AbstractRecordList::get(const QUuid &id, const QString &property) const
{
	return get(id).value(property);
}
QVector<QVariantHash> AbstractRecordList::get(const Filter &filter) const
{
	QVector<QVariantHash> result;
	for (const QUuid &id : ids()) {
		const QVariantHash hash = get(id);
		if (filter.matches(hash)) {
			result.append(hash);
		}
	}
	return result;
}
void AbstractRecordList::set(const QUuid &id, const QString &property, const QVariant &value)
{
	set(id, QVariantHash({{property, value}}));
}
void AbstractRecordList::set(const QUuid &id, const QVariantHash &properties)
{
	QVariantHash prop = properties;
	prop.insert("id", id);
	set(QVector<QVariantHash>() << prop);
}
void AbstractRecordList::add(const QVariantHash &values)
{
	add(QVector<QVariantHash>() << values);
}
void AbstractRecordList::remove(const QUuid &id)
{
	remove(QSet<QUuid>() << id);
}
