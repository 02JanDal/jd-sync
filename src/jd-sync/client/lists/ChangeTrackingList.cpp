#include "ChangeTrackingList.h"

#include <QVector>
#include <QVariant>
#include <QUuid>

#include <jd-util/Util.h>

ChangeTrackingList::ChangeTrackingList(AbstractRecordList *parentList, QObject *parent)
	: AbstractRecordList(parentList, parent)
{
	connect(m_parent, &AbstractRecordList::added, this, &ChangeTrackingList::addedInParent);
	connect(m_parent, &AbstractRecordList::removed, this, &ChangeTrackingList::removedInParent);
	connect(m_parent, &AbstractRecordList::changed, this, &ChangeTrackingList::changedInParent);
}

QVariantHash ChangeTrackingList::get(const QUuid &id) const
{
	if (m_additions.contains(id)) {
		return m_additions.value(id);
	} else if (m_removals.contains(id)) {
		return QVariantHash();
	} else {
		return JD::Util::mergeAssociative(m_parent->get(id), m_changes.value(id));
	}
}
void ChangeTrackingList::set(const QVector<QVariantHash> &properties)
{
	for (const QVariantHash &props : properties) {
		const QUuid id = props.value("id").toUuid();
		if (m_additions.contains(id)) {
			JD::Util::mergeAssociative(m_additions[id], props);
		} else if (m_removals.contains(id)) {
			continue;
		} else {
			JD::Util::mergeAssociative(m_changes[id], props);
		}
		for (const QString &property : props.keys()) {
			emit changed(id, property);
		}
	}
	updateModified();
}
void ChangeTrackingList::add(const QVector<QVariantHash> &properties)
{
	for (const QVariantHash &props : properties) {
		const QUuid id = props.contains("id") ? props.value("id").toUuid() : QUuid::createUuid();
		Q_ASSERT_X(!m_additions.contains(id), "ChangeTrackingList::add", "Attempting to add already existing ID");
		m_additions.insert(id, JD::Util::mergeAssociative(props, QVariantHash({{"id", id}})));
		emit added(id);
	}
	updateModified();
}
void ChangeTrackingList::remove(const QSet<QUuid> &ids)
{
	for (const QUuid &id : ids) {
		if (m_additions.contains(id)) {
			m_additions.remove(id);
		} else {
			m_removals.insert(id);
		}
		emit removed(id);
	}
	updateModified();
}

bool ChangeTrackingList::contains(const QUuid &id) const
{
	return !m_removals.contains(id) && (m_parent->contains(id) || m_additions.contains(id));
}
QList<QUuid> ChangeTrackingList::ids() const
{
	QList<QUuid> ids = m_parent->ids() + m_additions.keys();
	for (const QUuid &removed : m_removals) {
		ids.removeAll(removed);
	}
	return ids;
}

bool ChangeTrackingList::isAddition(const QUuid &id) const
{
	return m_additions.contains(id);
}

void ChangeTrackingList::commit()
{
	m_committing = true;
	m_parent->set(m_changes.values().toVector());
	m_parent->add(m_additions.values().toVector());
	m_parent->remove(m_removals);
	m_committing = false;
}
void ChangeTrackingList::discard()
{
	auto additions = m_additions;
	auto removals = m_removals;
	auto changes = m_changes;
	m_additions.clear();
	m_removals.clear();
	m_changes.clear();

	for (const QUuid &id : additions.keys()) {
		emit removed(id);
	}
	for (const QUuid &id : removals) {
		emit added(id);
	}
	for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
		for (const QString &property : it.value().keys()) {
			emit changed(it.key(), property);
		}
	}

	updateModified();
}

void ChangeTrackingList::addedInParent(const QUuid &id)
{
	if (m_committing) {
		return;
	}
	emit added(id);
}
void ChangeTrackingList::removedInParent(const QUuid &id)
{
	if (m_committing) {
		return;
	}
	if (m_changes.contains(id)) {
		m_changes.remove(id);
		updateModified();
	}
	emit removed(id);
}
void ChangeTrackingList::changedInParent(const QUuid &id, const QString &property)
{
	if (m_committing) {
		return;
	}

	if (m_changes.contains(id) && m_changes[id].contains(property) && m_changes[id][property] == m_parent->get(id, property)) {
		m_changes[id].remove(property);
		if (m_changes[id].isEmpty()) {
			m_changes.remove(id);
		}
		emit changed(id, property);
		updateModified();
	} else if (!m_removals.contains(id)) {
		emit changed(id, property);
	}
}

void ChangeTrackingList::updateModified()
{
	const bool newStatus = !m_removals.isEmpty() || !m_additions.isEmpty() || !m_changes.isEmpty();
	if (newStatus != m_modified) {
		m_modified = newStatus;
		emit modifiedChanged(m_modified);
	}
}
