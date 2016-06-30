#include "SyncedList.h"

#include "jd-util/Json.h"

static inline QJsonValue toJson(const QVariant &value)
{
	return QJsonValue::fromVariant(value);
}

SyncedList::SyncedList(ServerConnection *server, const QString &channel, const QString &cmdPrefix, const QString &indexProperty, QObject *parent)
	: QObject(parent), AbstractConsumer(server), m_channel(channel), m_cmdPrefix(cmdPrefix), m_indexProperty(indexProperty)
{
	subscribeTo(channel);
	refetch();
}

QVariant SyncedList::get(const QVariant &index, const QString &property) const
{
	return m_rows[index].value(property);
}
QVariantMap SyncedList::getAll(const QVariant &index) const
{
	return m_rows[index];
}

void SyncedList::set(const QVariant &index, const QString &property, const QVariant &value)
{
	if (get(index, property) != value)
	{
		emitMsg(m_channel, command("set"), {{m_indexProperty, toJson(index)}, {property, toJson(value)}});
	}
}

void SyncedList::add(const QVariant &index, const QVariantMap &values)
{
	QVariantMap v = values;
	v.insert(m_indexProperty, index);
	emitMsg(m_channel, command("add"), QJsonObject::fromVariantMap(v));
}
void SyncedList::remove(const QVariant &index)
{
	emitMsg(m_channel, command("remove"), {{m_indexProperty, toJson(m_rows[index].value(m_indexProperty))}});
}

bool SyncedList::contains(const QVariant &index) const
{
	return m_rows.contains(index);
}
QList<QVariant> SyncedList::indices() const
{
	return m_rows.keys();
}

void SyncedList::refetch()
{
	for (const QVariant &index : m_rows.keys())
	{
		emit removed(index);
	}
	m_rows.clear();
	emitMsg(m_channel, command("list"));
}
void SyncedList::refetch(const QVariant &index)
{
	emitMsg(m_channel, "get", {{m_indexProperty, toJson(index)}});
}

void SyncedList::consume(const QString &channel, const QString &cmd, const QJsonObject &data)
{
	if (channel == m_channel)
	{
		if (cmd == command("added"))
		{
			QVariantMap values = cleanMap(data.toVariantMap());
			const QVariant index = values[m_indexProperty];
			if (m_rows.contains(index))
			{
				emit removed(index);
			}
			m_rows.insert(index, values);
			emit added(index);
		}
		else if (cmd == command("removed"))
		{
			const QVariant index = Json::ensureVariant(data, m_indexProperty);
			if (m_rows.contains(index))
			{
				emit removed(index);
				m_rows.remove(index);
			}
		}
		else if (cmd == command("changed"))
		{
			const QVariant index = Json::ensureVariant(data, m_indexProperty);
			for (const QString &property : data.keys())
			{
				if (property == "channel" || property == "cmd" || property == "msgId" || property == "replyTo" || property == m_indexProperty)
				{
					continue;
				}
				m_rows[index].insert(property, data.value(property).toVariant());
				emit changed(index, property);
			}
		}
		else if (cmd == command("item"))
		{
			addOrUpdate(data.toVariantMap());
		}
		else if (cmd == command("items"))
		{
			const QJsonArray array = Json::ensureArray(data, "items");
			for (const QJsonValue &val : array)
			{
				if (val.isObject() && val.toObject().contains(m_indexProperty))
				{
					addOrUpdate(Json::ensureObject(val).toVariantMap());
				}
				else
				{
					emit refetch(val.toVariant());
				}
			}
		}
	}
}

void SyncedList::addOrUpdate(const QVariantMap &map)
{
	const QVariant index = map.value(m_indexProperty);
	if (!m_rows.contains(index))
	{
		m_rows.insert(index, cleanMap(map));
		emit added(index);
	}
	else
	{
		const QVariantMap old = m_rows[index];
		m_rows[index] = cleanMap(map);
		QStringList keys = old.keys() + m_rows[index].keys();
		keys.removeDuplicates();
		for (const QString &key : keys)
		{
			if (m_rows[index].value(key) != old.value(key))
			{
				emit changed(index, key);
			}
		}
	}
}

QString SyncedList::command(const QString &cmd) const
{
	if (m_cmdPrefix.isEmpty())
	{
		return cmd;
	}
	else
	{
		return m_cmdPrefix + ':' + cmd;
	}
}

QVariantMap SyncedList::cleanMap(const QVariantMap &map)
{
	QVariantMap out = map;
	out.remove("channel");
	out.remove("cmd");
	out.remove("msgId");
	out.remove("replyTo");
	return out;
}


SyncedListModel::SyncedListModel(ServerConnection *server, const QString &channel, const QString &cmdPrefix, const QString &indexProperty, QObject *parent)
	: QAbstractListModel(parent), AbstractConsumer(server), m_list(new SyncedList(server, channel, cmdPrefix, indexProperty, this))
{
	connect(m_list, &SyncedList::added, this, &SyncedListModel::added);
	connect(m_list, &SyncedList::removed, this, &SyncedListModel::removed);
	connect(m_list, &SyncedList::changed, this, &SyncedListModel::changed);
}

int SyncedListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
	{
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
	if (!m_mapping.contains(qMakePair(index.column(), role)))
	{
		return QVariant();
	}
	return m_list->get(m_indices.at(index.row()), m_mapping[qMakePair(index.column(), role)].first);
}
bool SyncedListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!m_mapping.contains(qMakePair(index.column(), role)))
	{
		return false;
	}
	const QPair<QString, bool> pair = m_mapping[qMakePair(index.column(), role)];
	if (!pair.second)
	{
		return false;
	}
	m_list->set(m_indices.at(index.row()), pair.first, value);
	return true;
}
Qt::ItemFlags SyncedListModel::flags(const QModelIndex &index) const
{
	if (m_mapping.contains(qMakePair(index.column(), Qt::EditRole)) && m_mapping[qMakePair(index.column(), Qt::EditRole)].second)
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}
	else
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void SyncedListModel::addMapping(int column, const QString &property, int role, const bool editable)
{
	m_cols = qMax(m_cols, column +1);
	m_mapping.insert(qMakePair(column, role), qMakePair(property, editable));
	emit dataChanged(index(0, column), index(rowCount(QModelIndex()), column), QVector<int>() << role);
}
void SyncedListModel::addEditable(int column, const QString &property)
{
	addMapping(column, property, Qt::DisplayRole);
	addMapping(column, property, Qt::EditRole, true);
}

void SyncedListModel::added(const QVariant &id)
{
	Q_ASSERT(!m_indices.contains(id));
	beginInsertRows(QModelIndex(), m_indices.size(), m_indices.size());
	m_indices.append(id);
	endInsertRows();
}
void SyncedListModel::removed(const QVariant &id)
{
	const int row = m_indices.indexOf(id);
	Q_ASSERT(row != -1);
	beginRemoveRows(QModelIndex(), row, row);
	m_indices.removeAt(row);
	endRemoveRows();
}
void SyncedListModel::changed(const QVariant &id, const QString &property)
{
	const int row = m_indices.indexOf(id);
	for (auto it = m_mapping.constBegin(); it != m_mapping.constEnd(); ++it)
	{
		if (it.value().first == property)
		{
			emit dataChanged(index(row, it.key().first), index(row, it.key().first), QVector<int>() << it.key().second);
		}
	}
}
