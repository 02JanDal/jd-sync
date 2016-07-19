#include "FilteredList.h"

#include <jd-util/functional/Filter.h>

FilteredList::FilteredList(AbstractRecordList *parentList, QObject *parent)
	: AbstractRecordList(parentList, parent)
{
	connect(m_parent, &AbstractRecordList::added, this, &FilteredList::addedInParent);
	connect(m_parent, &AbstractRecordList::removed, this, &FilteredList::removedInParent);
	connect(m_parent, &AbstractRecordList::changed, this, &FilteredList::changedInParent);
}

void FilteredList::setFilter(const Filter &filter)
{
	if (m_filter != filter) {
		m_filter = filter;
		refilter();
	}
}

void FilteredList::addedInParent(const QUuid &id)
{
	if (m_filter.matches(m_parent->get(id))) {
		m_ids.append(id);
		emit added(id);
	}
}
void FilteredList::removedInParent(const QUuid &id)
{
	if (m_ids.contains(id)) {
		m_ids.removeAll(id);
		emit removed(id);
	}
}
void FilteredList::changedInParent(const QUuid &id, const QString &property)
{
	const bool isInModel = m_ids.contains(id);
	const bool shouldBeInModel = m_filter.matches(m_parent->get(id));

	if (isInModel && !shouldBeInModel) {
		m_ids.removeAll(id);
		emit removed(id);
	} else if (!isInModel && shouldBeInModel) {
		m_ids.append(id);
		emit added(id);
	} else if (isInModel && shouldBeInModel) {
		emit changed(id, property);
	}
}

void FilteredList::refilter()
{
	const QList<QUuid> oldIds = m_ids;
	m_ids = JD::Util::Functional::filter(m_parent->ids(), [this](const QUuid &id) { return m_filter.matches(m_parent->get(id)); });

	for (const QUuid &id : oldIds) {
		if (!m_ids.contains(id)) {
			emit removed(id);
		}
	}
	for (const QUuid &id : m_ids) {
		if (!oldIds.contains(id)) {
			emit added(id);
		}
	}
}
