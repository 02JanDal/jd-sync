#include "CRUDMessages.h"

#include <jd-util/Json.h>

BaseCRUDMessage::BaseCRUDMessage(const QString &channel, const QString &command, const QString &table, const QJsonValue &data)
	: Message(channel, command, data), m_table(table) {}
BaseCRUDMessage::BaseCRUDMessage(const Message &origin, const QString &table)
	: Message(origin), m_table(table) {}

CreateMessage::CreateMessage(const QString &channel, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(channel, "create", table,
					  QJsonObject({{"table", table},
								   {"items", Json::toJsonArray(items)}})) {}
CreateMessage::CreateMessage(const QString &channel, const QString &table, const QJsonObject &item)
	: CreateMessage(channel, table, QVector<QJsonObject>() << item) {}
CreateMessage::CreateMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(origin, table), m_items(items) {}
CreateReplyMessage CreateMessage::createSuccessReply(const QVector<QJsonObject> &items) const
{
	return CreateReplyMessage(createReply("create:result", QJsonObject({{"table", table()},
																		{"items", Json::toJsonArray(items)}})),
							  table(), items);
}

ReadMessage::ReadMessage(const QString &channel, const QString &table, const QVector<QVariant> &recordIds, const QVector<QString> &properties)
	: BaseCRUDMessage(channel, "read", table,
					  QJsonObject({{"table", table},
								   {"ids", Json::toJsonArray(recordIds)},
								   {"properties", QJsonArray::fromStringList(properties.toList())}})
					  ),
	  m_recordIds(recordIds), m_properties(properties) {}
ReadMessage::ReadMessage(const QString &channel, const QString &table, const QVariant &recordId, const QVector<QString> &properties)
	: ReadMessage(channel, table, QVector<QVariant>() << recordId, properties) {}
ReadMessage::ReadMessage(const Message &origin, const QString &table, const QVector<QVariant> &recordIds, const QVector<QString> &properties)
	: BaseCRUDMessage(origin, table), m_recordIds(recordIds), m_properties(properties) {}
ReadReplyMessage ReadMessage::createSuccessReply(const QVector<QJsonObject> &items) const
{
	return ReadReplyMessage(createReply("read:result", QJsonObject({{"table", table()},
																	{"items", Json::toJsonArray(items)}})),
							table(), items);
}
ReadReplyMessage::ReadReplyMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(origin, table), m_items(items) {}

UpdateMessage::UpdateMessage(const QString &channel, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(channel, "update", table,
					  QJsonObject({{"table", table},
								   {"items", Json::toJsonArray(items)}})) {}
UpdateMessage::UpdateMessage(const QString &channel, const QString &table, const QJsonObject &item)
	: UpdateMessage(channel, table, QVector<QJsonObject>() << item) {}
UpdateMessage::UpdateMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(origin, table), m_items(items) {}
UpdateReplyMessage UpdateMessage::createSuccessReply() const
{
	return UpdateReplyMessage(createReply("update:result", data()), table(), m_items);
}

DeleteMessage::DeleteMessage(const QString &channel, const QString &table, const QVector<QVariant> &recordIds)
	: BaseCRUDMessage(channel, "delete", table,
					  QJsonObject({{"table", table},
								   {"ids", Json::toJsonArray(recordIds)}})),
	  m_recordIds(recordIds) {}
DeleteMessage::DeleteMessage(const QString &channel, const QString &table, const QVariant &recordId)
	: DeleteMessage(channel, table, QVector<QVariant>() << recordId) {}
DeleteMessage::DeleteMessage(const Message &origin, const QString &table, const QVector<QVariant> &recordIds)
	: BaseCRUDMessage(origin, table), m_recordIds(recordIds) {}
DeleteReplyMessage DeleteMessage::createSuccessReply() const
{
	return DeleteReplyMessage(createReply("delete:result", data()), table(), m_recordIds);
}

IndexMessage::IndexMessage(const QString &channel, const QString &table)
	: BaseCRUDMessage(channel, "index", table, QJsonObject({{"table", table}})) {}
IndexMessage::IndexMessage(const Message &origin, const QString &table, const Filter &filter, const int limit, const int offset, const QPair<QString, Qt::SortOrder> &order, const int since)
	: BaseCRUDMessage(origin, table), m_filter(filter), m_limit(limit), m_offset(offset), m_order(order), m_since(since) {}

IndexMessage &IndexMessage::setFilter(const Filter &filter)
{
	m_filter = filter;
	QJsonObject obj = dataObject();
	if (m_filter.isEmpty()) {
		obj.remove("filter");
	} else {
		obj.insert("filter", m_filter.toJson());
	}
	setData(obj);
	return *this;
}
IndexMessage &IndexMessage::setLimit(const int limit)
{
	m_limit = limit;
	QJsonObject obj = dataObject();
	if (m_limit == -1) {
		obj.remove("limit");
	}	else {
		obj.insert("limit", m_limit);
	}
	setData(obj);
	return *this;
}
IndexMessage &IndexMessage::setOffset(const int offset)
{
	m_offset = offset;
	QJsonObject obj = dataObject();
	if (m_offset == -1) {
		obj.remove("offset");
	} else {
		obj.insert("offset", m_offset);
	}
	setData(obj);
	return *this;
}
IndexMessage &IndexMessage::setOrder(const QPair<QString, Qt::SortOrder> &order)
{
	m_order = order;
	QJsonObject obj = dataObject();
	if (m_order.first.isEmpty()) {
		obj.remove("order");
		obj.remove("orderAsc");
	} else {
		obj.insert("order", m_order.first);
		obj.insert("orderAsc", m_order.second == Qt::AscendingOrder);
	}
	setData(obj);
	return *this;
}
IndexMessage &IndexMessage::setSince(const int since)
{
	m_since = since;
	QJsonObject obj = dataObject();
	if (since == -1) {
		obj.remove("since");
	} else {
		obj.insert("since", m_since);
	}
	setData(obj);
	return *this;
}

IndexReplyMessage IndexMessage::createSuccessReply(const QVector<QJsonObject> &items) const
{
	return IndexReplyMessage(createTargetedReply("index:result", QJsonObject({{"table", table()},
																			  {"items", Json::toJsonArray(items)}})),
							 table(), items);
}

IndexReplyMessage::IndexReplyMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items)
	: BaseCRUDMessage(origin, table), m_items(items) {}
