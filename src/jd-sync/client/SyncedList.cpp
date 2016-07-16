#include "SyncedList.h"

#include <jd-util/Json.h>
#include <jd-util/Functional.h>

#include "common/Message.h"
#include "common/Request.h"
#include "common/CRUDMessages.h"

using namespace JD::Util;

struct ToJsonObjectRecord
{
	Table m_table;
	bool m_isCreate;

	explicit ToJsonObjectRecord(const Table &table, const bool isCreate) : m_table(table), m_isCreate(isCreate) {}

	QJsonObject operator()(const QVariantHash &properties) const
	{
		QJsonObject out;
		for (const QString &property : properties.keys()) {
			if (m_table.contains(property)) {
				out.insert(property, toJson(m_table.column(property).type(), properties.value(property)));
			}
		}
		if (m_isCreate) {
			for (const QString &column : m_table.columns().keys()) {
				if (!out.contains(column)) {
					const Column &col = m_table.column(column);
					if (col.defaultValue().isValid()) {
						out.insert(column, toJson(col.type(), col.defaultValue()));
					} else if (col.isNullable()) {
						out.insert(column, QJsonValue::Null);
					} else {
						throw Exception("No value given for non-defaulted non-nullable field");
					}
				}
			}
		}
		return out;
	}
};

SyncedList::SyncedList(MessageHub *hub, const QString &channel, const Table &table, QObject *parent)
	: QObject(parent), AbstractActor(hub), m_channel(channel), m_table(table)
{
	subscribeTo(channel);
}

QVariant SyncedList::get(const QUuid &id, const QString &property) const
{
	return m_rows[id].value(property);
}
QVariantHash SyncedList::getAll(const QUuid &id) const
{
	return m_rows[id];
}

void SyncedList::set(const QUuid &id, const QString &property, const QVariant &value)
{
	set(id, QVariantHash({{property, Json::toJson(value)}}));
}
void SyncedList::set(const QUuid &id, const QVariantHash &properties)
{
	QVariantHash prop = properties;
	prop.insert("id", id);
	set(QVector<QVariantHash>() << prop);
}
void SyncedList::set(const QVector<QVariantHash> &properties)
{
	const QVector<QJsonObject> array = Functional::map(properties, ToJsonObjectRecord(m_table, false));
	request(UpdateMessage(m_channel, m_channel, array)).send();
	for (const QJsonObject &obj : array) {
		addOrUpdate(obj);
	}
}

void SyncedList::add(const QVariantHash &values)
{
	add(QVector<QVariantHash>() << values);
}
void SyncedList::add(const QVector<QVariantHash> &values)
{
	const QVector<QJsonObject> array = Functional::map(values, ToJsonObjectRecord(m_table, true));
	request(CreateMessage(m_channel, m_channel, array)).send();
	for (const QJsonObject &obj : array) {
		addOrUpdate(obj);
	}
}
void SyncedList::remove(const QUuid &id)
{
	remove(QSet<QUuid>() << id);
}
void SyncedList::remove(const QSet<QUuid> &ids)
{
	const QVector<QVariant> idsVector = Functional::map2<QVector<QVariant>>(ids, &QVariant::fromValue<QUuid>);
	request(DeleteMessage(m_channel, m_channel, idsVector))
			.error([this, idsVector](const ErrorMessage &) { fetchOnce(Filter(FilterPart("id", FilterPart::InSet, idsVector.toList()))); })
			.send();
	for (const QUuid &id : ids) {
		m_rows.remove(id);
		emit removed(id);
	}
}

bool SyncedList::contains(const QUuid &id) const
{
	return m_rows.contains(id);
}
QList<QUuid> SyncedList::indexes() const
{
	return m_rows.keys();
}

void SyncedList::refetch()
{
	if (m_lastUpdated != -1) {
		request(IndexMessage(m_channel, m_channel).setFilter(m_focusFilter).setLimit(250).setSince(m_lastUpdated)).send();
	}
}
void SyncedList::fetchMore()
{
	throw NotImplementedException();
}
void SyncedList::refetch(const QUuid &id)
{
	request(ReadMessage(m_channel, m_channel, id)).send();
}
void SyncedList::fetchOnce(const Filter &filter)
{
	request(IndexMessage(m_channel, m_channel).setFilter(filter).setLimit(250)).send();
}

void SyncedList::setFocus(const Filter &filter)
{
	m_focusFilter = filter;
	m_lastUpdated = 0;
	refetch();
}

void SyncedList::receive(const Message &msg)
{
	if (msg.channel() == m_channel) {
		if (msg.isCreateReply()) {
			for (const QJsonObject &row : msg.toCreateReply().items()) {
				addOrUpdate(row);
			}
		} else if (msg.isReadReply()) {
			for (const QJsonObject &row : msg.toReadReply().items()) {
				addOrUpdate(row);
			}
		} else if (msg.isUpdateReply()) {
			for (const QJsonObject &row : msg.toUpdateReply().items()) {
				addOrUpdate(row);
			}
		} else if (msg.isDeleteReply()) {
			for (const QVariant &var : msg.toDeleteReply().recordIds()) {
				const QUuid id = var.toUuid();
				if (m_rows.contains(id)) {
					m_rows.remove(id);
					emit removed(id);
				}
			}
		} else if (msg.isIndexReply()) {
			for (const QJsonObject &row : msg.toIndexReply().items()) {
				addOrUpdate(row);
			}
		}
	}
}
void SyncedList::reset()
{
	refetch();
}

void SyncedList::addOrUpdate(const QJsonObject &record)
{
	const QUuid id = Json::ensureUuid(record, "id");

	// if it's a new record we need all fields, otherwise we need to re-fetch the complete thing
	if (!m_rows.contains(id)) {
		for (const QString &expected : m_table.columns().keys()) {
			if (!record.contains(expected)) {
				request(Message(m_channel, "read", Json::toJson(id))).send();
				return;
			}
		}
		QVariantHash values;
		for (const QString &property : record.keys()) {
			if (m_table.contains(property)) {
				values.insert(property, fromJson(m_table.column(property).type(), record.value(property)));
			}
		}
		m_rows.insert(id, values);
		emit added(id);
	} else {
		// updated
		for (auto it = record.begin(); it != record.end(); ++it) {
			const QVariant value = fromJson(m_table.column(it.key()).type(), it.value());
			if (m_rows[id][it.key()] != value) {
				m_rows[id][it.key()] = value;
				emit changed(id, it.key());
			}
		}
	}
}

SyncedListModel::SyncedListModel(MessageHub *hub, const QString &channel, const Table &table, QObject *parent)
	: SyncedListModel(new SyncedList(hub, channel, table, this), parent) {}
SyncedListModel::SyncedListModel(SyncedList *list, QObject *parent)
	: QAbstractListModel(parent), m_list(list)
{
	connect(m_list, &SyncedList::added, this, &SyncedListModel::addedToList);
	connect(m_list, &SyncedList::removed, this, &SyncedListModel::removedFromList);
	connect(m_list, &SyncedList::changed, this, &SyncedListModel::changedInList);

	m_indices = m_list->indexes().toVector();
}

void SyncedListModel::focus()
{
	m_list->setFocus(m_filter);
}
void SyncedListModel::setFilter(const Filter &filter)
{
	beginResetModel();
	m_filter = filter;
	m_indices = JD::Util::Functional::filter2<QVector<QUuid>>(m_list->indexes(), [this](const QUuid &id) { return m_filter.matches(m_list->getAll(id)); });
	endResetModel();
	focus();
}

int SyncedListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	return m_indices.size();
}
int SyncedListModel::columnCount(const QModelIndex &parent) const
{
	return m_cols;
}
QVariant SyncedListModel::data(const QModelIndex &index, int role) const
{
	if (!m_mapping.contains(qMakePair(index.column(), Qt::ItemDataRole(role)))) {
		return QVariant();
	}
	const QString property = m_mapping[qMakePair(index.column(), Qt::ItemDataRole(role))].property;
	const QUuid id = idForIndex(index);
	if (m_preliminaryAdditions.contains(id)) {
		return m_preliminaryAdditions[id][property];
	} else if (m_preliminaryChanges.contains(id) && m_preliminaryChanges[id].contains(property)) {
		return m_preliminaryChanges[id][property];
	} else {
		return m_list->get(id, property);
	}
}
QVariant SyncedListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < m_headings.size()) {
		return m_headings.at(section);
	} else {
		return QVariant();
	}
}
QVariant SyncedListModel::data(const QUuid &id, const QString &property) const
{
	if (m_preliminaryAdditions.contains(id)) {
		return m_preliminaryAdditions[id][property];
	} else if (m_preliminaryChanges.contains(id) && m_preliminaryChanges[id].contains(property)) {
		return m_preliminaryChanges[id][property];
	} else {
		return m_list->get(id, property);
	}
}
bool SyncedListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!m_mapping.contains(qMakePair(index.column(), Qt::ItemDataRole(role)))) {
		return false;
	}
	const Mapping mapping = m_mapping[qMakePair(index.column(), Qt::ItemDataRole(role))];
	if (!mapping.editable) {
		return false;
	}

	const QString property = mapping.property;
	const QUuid id = idForIndex(index);

	if (m_preliminaryAdditions.contains(id)) {
		m_preliminaryAdditions[id][property] = value;
	} else {
		if (value == m_list->get(id, property)) {
			m_preliminaryChanges[id].remove(property);
			if (m_preliminaryChanges[id].isEmpty()) {
				m_preliminaryChanges.remove(id);
			}
		} else {
			m_preliminaryChanges[id][property] = value;
		}
	}
	emit dataChanged(index, index, QVector<int>() << role);
	return true;
}
bool SyncedListModel::setData(const QUuid &id, const QString &property, const QVariant &value)
{
	if (m_preliminaryAdditions.contains(id)) {
		m_preliminaryAdditions[id][property] = value;
	} else {
		if (value == m_list->get(id, property)) {
			m_preliminaryChanges[id].remove(property);
			if (m_preliminaryChanges[id].isEmpty()) {
				m_preliminaryChanges.remove(id);
			}
		} else {
			m_preliminaryChanges[id][property] = value;
		}
	}

	QSet<int> roles;
	int leftCol= 255, rightCol = 0;
	for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		if (it.value().property == property) {
			roles.insert(it.key().second);
			leftCol = std::min(leftCol, it.key().first);
			rightCol = std::max(rightCol, it.key().first);
		}
	}
	const int row = m_indices.indexOf(id);
	emit dataChanged(index(row, leftCol), index(row, rightCol), roles.toList().toVector());
	emit modifiedChanged(isModified());
	return true;
}
Qt::ItemFlags SyncedListModel::flags(const QModelIndex &index) const
{
	const QPair<int, Qt::ItemDataRole> mappingKey = qMakePair(index.column(), Qt::ItemDataRole(Qt::EditRole));
	if (m_preliminaryRemovals.contains(idForIndex(index))) {
		return Qt::NoItemFlags;
	} else if (m_mapping.contains(mappingKey) && m_mapping[mappingKey].editable) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	} else {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void SyncedListModel::addMapping(const int column, const QString &property, const Qt::ItemDataRole role, const bool editable)
{
	beginResetModel();
	m_cols = qMax(m_cols, column + 1);
	m_mapping.insert(qMakePair(column, Qt::ItemDataRole(role)), Mapping{property, editable});
	endResetModel();
}
void SyncedListModel::addEditable(const int column, const QString &property)
{
	addMapping(column, property, Qt::DisplayRole);
	addMapping(column, property, Qt::EditRole, true);
}

void SyncedListModel::setHeadings(const QVector<QString> &headings)
{
	m_headings = headings;
}

QUuid SyncedListModel::addPreliminary(const QVariantHash &properties)
{
	QVariantHash props;
	for (const QString &prop : m_list->table().columns().keys()) {
		props.insert(prop, properties.value(prop));
	}
	props.insert("id", properties.value("id", QUuid::createUuid()));

	const QUuid id = props.value("id").toUuid();
	m_preliminaryAdditions.insert(id, props);

	beginInsertRows(QModelIndex(), m_indices.size(), m_indices.size());
	m_indices.append(id);
	endInsertRows();

	emit added(id);
	emit modifiedChanged(isModified());

	return id;
}
void SyncedListModel::removePreliminary(const QUuid &id)
{
	const int index = m_indices.indexOf(id);
	if (index == -1) {
		return;
	}

	m_preliminaryRemovals.insert(id);
	emit dataChanged(this->index(index), this->index(index));
}
void SyncedListModel::commitPreliminaries()
{
	auto additions = m_preliminaryAdditions;
	auto removals = m_preliminaryRemovals;
	auto changes = m_preliminaryChanges;

	discardPreliminaries();

	if (!additions.isEmpty()) {
		m_list->add(additions.values().toVector());
	}
	if (!changes.isEmpty()) {
		QVector<QVariantHash> properties;
		for (auto it = changes.begin(); it != changes.end(); ++it) {
			(*it)["id"] = it.key();
			properties.append(*it);
		}
		m_list->set(properties);
	}
	if (!removals.isEmpty()) {
		m_list->remove(removals);
	}
}
void SyncedListModel::discardPreliminaries()
{
	auto additions = m_preliminaryAdditions;
	auto removals = m_preliminaryRemovals;
	auto changes = m_preliminaryChanges;
	m_preliminaryAdditions.clear();
	m_preliminaryRemovals.clear();
	m_preliminaryChanges.clear();

	for (const QUuid &id : additions.keys()) {
		const int index = m_indices.indexOf(id);
		if (index == -1) {
			continue;
		}
		beginRemoveRows(QModelIndex(), index, index);
		m_indices.removeAt(index);
		endRemoveRows();
		emit removed(id);
	}
	for (const QUuid &id : removals) {
		const int ind = m_indices.indexOf(id);
		if (ind == -1) {
			continue;
		}
		emit dataChanged(index(ind, 0), index(ind, m_cols));
	}
	for (const QUuid &id : changes.keys()) {
		const int ind = m_indices.indexOf(id);
		if (ind == -1) {
			continue;
		}

		emit dataChanged(index(ind, 0), index(ind, m_cols));
		for (const QString &property : changes.value(id).keys()) {
			emit changed(id, property);
		}
	}

	emit modifiedChanged(false);
}

bool SyncedListModel::isModified() const
{
	return !m_preliminaryAdditions.isEmpty() || !m_preliminaryChanges.isEmpty() || !m_preliminaryRemovals.isEmpty();
}

bool SyncedListModel::isPreliminaryAddition(const QUuid &uuid) const
{
	return m_preliminaryAdditions.contains(uuid);
}

void SyncedListModel::addedToList(const QUuid &id)
{
	if (m_filter.matches(m_list->getAll(id))) {
		Q_ASSERT(!m_indices.contains(id));
		beginInsertRows(QModelIndex(), m_indices.size(), m_indices.size());
		m_indices.append(id);
		endInsertRows();
		emit added(id);
	}
}
void SyncedListModel::removedFromList(const QUuid &id)
{
	const int row = m_indices.indexOf(id);
	if (row != -1) {
		beginRemoveRows(QModelIndex(), row, row);
		m_indices.removeAt(row);
		endRemoveRows();
		emit removed(id);

		if (m_preliminaryChanges.contains(id)) {
			m_preliminaryChanges.remove(id);
			emit modifiedChanged(isModified());
		}
	}
}
void SyncedListModel::changedInList(const QUuid &id, const QString &property)
{
	const int row = m_indices.indexOf(id);

	const bool isInModel = row != -1;
	const bool shouldBeInModel = m_filter.matches(m_list->getAll(id));

	if (isInModel && !shouldBeInModel) {
		removedFromList(id);
	} else if (!isInModel && shouldBeInModel) {
		addedToList(id);
	} else if (isInModel && shouldBeInModel) {
		for (auto it = m_mapping.constBegin(); it != m_mapping.constEnd(); ++it) {
			if (it.value().property == property) {
				emit dataChanged(index(row, it.key().first), index(row, it.key().first), QVector<int>() << it.key().second);
				break;
			}
		}
		emit changed(id, property);
	}

}

QPair<int, Qt::ItemDataRole> SyncedListModel::reverseMapping(const QString &property, const bool editable) const
{
	for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		if (it.value().property == property && (it.value().editable == editable || !editable)) {
			return it.key();
		}
	}
	return {};
}

QUuid SyncedListModel::idForIndex(const QModelIndex &index) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		return QUuid();
	}
	return m_indices.at(index.row());
}
QModelIndex SyncedListModel::indexForId(const QUuid &id) const
{
	return index(m_indices.indexOf(id));
}

QVariantHash SyncedListModel::getAll(const QUuid &id) const
{
	QVariantHash out;
	if (m_preliminaryAdditions.contains(id)) {
		out = m_preliminaryAdditions.value(id);
	} else {
		out = m_list->getAll(id);
		for (const QString &property : m_preliminaryChanges.value(id).keys()) {
			out[property] = m_preliminaryChanges.value(id).value(property);
		}
	}
	return out;
}
