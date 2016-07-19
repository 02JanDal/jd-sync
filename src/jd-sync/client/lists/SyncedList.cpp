#include "SyncedList.h"

#include <jd-util/Json.h>
#include <jd-util/Functional.h>
#include <jd-util/Util.h>

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
					} else if (column != "updated_at") {
						throw Exception("No value given for non-defaulted non-nullable field '%1'" % column);
					}
				}
			}
		}
		return out;
	}
};

SyncedList::SyncedList(MessageHub *hub, const QString &channel, const Table &table, QObject *parent)
	: AbstractRecordList(nullptr, parent), AbstractActor(hub), m_channel(channel), m_table(table)
{
	subscribeTo(channel);
}

QVariantHash SyncedList::get(const QUuid &id) const
{
	return m_rows[id];
}
void SyncedList::set(const QVector<QVariantHash> &properties)
{
	if (properties.isEmpty()) {
		return;
	}

	const QVector<QJsonObject> array = Functional::map(properties, ToJsonObjectRecord(m_table, false));
	request(UpdateMessage(m_channel, m_channel, array)).send();
	for (const QJsonObject &obj : array) {
		addOrUpdate(obj);
	}
}
void SyncedList::add(const QVector<QVariantHash> &values)
{
	if (values.isEmpty()) {
		return;
	}

	const QVector<QJsonObject> array = Functional::map(values, ToJsonObjectRecord(m_table, true));
	request(CreateMessage(m_channel, m_channel, array)).send();
	for (const QJsonObject &obj : array) {
		addOrUpdate(obj);
	}
}
void SyncedList::remove(const QSet<QUuid> &ids)
{
	if (ids.isEmpty()) {
		return;
	}

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
QList<QUuid> SyncedList::ids() const
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
			if (!record.contains(expected)
					&& !(m_table.column(expected).isNullable() || m_table.column(expected).defaultValue().isValid())
					&& expected != "updated_at") {
				request(ReadMessage(m_channel, m_channel, id)).send();
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
