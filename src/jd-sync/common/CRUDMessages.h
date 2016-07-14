#pragma once

#include <QVector>

#include "Message.h"
#include "Filter.h"

class BaseCRUDMessage : public Message
{
public:
	explicit BaseCRUDMessage(const QString &channel, const QString &command, const QString &table, const QJsonValue &data);
	explicit BaseCRUDMessage(const Message &origin, const QString &table);

	QString table() const { return m_table; }

private:
	QString m_table;
};

class CreateMessage : public BaseCRUDMessage
{
public:
	explicit CreateMessage(const QString &channel, const QString &table, const QVector<QJsonObject> &items);
	explicit CreateMessage(const QString &channel, const QString &table, const QJsonObject &item);
	explicit CreateMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items);

	QVector<QJsonObject> items() const { return m_items; }

	CreateReplyMessage createSuccessReply(const QVector<QJsonObject> &items) const;

private:
	QVector<QJsonObject> m_items;
};
class CreateReplyMessage : public CreateMessage
{
public:
	using CreateMessage::CreateMessage;
};

class ReadMessage : public BaseCRUDMessage
{
public:
	explicit ReadMessage(const QString &channel, const QString &table, const QVector<QVariant> &recordIds, const QVector<QString> &properties = QVector<QString>());
	explicit ReadMessage(const QString &channel, const QString &table, const QVariant &recordId, const QVector<QString> &properties = QVector<QString>());
	explicit ReadMessage(const Message &origin, const QString &table, const QVector<QVariant> &recordIds, const QVector<QString> &properties);

	QVector<QVariant> recordIds() const { return m_recordIds; }
	QVector<QString> properties() const { return m_properties; }

	ReadReplyMessage createSuccessReply(const QVector<QJsonObject> &items) const;

private:
	QVector<QVariant> m_recordIds;
	QVector<QString> m_properties;
};
class ReadReplyMessage : public BaseCRUDMessage
{
public:
	explicit ReadReplyMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items);

	QVector<QJsonObject> items() const { return m_items; }

private:
	QVector<QJsonObject> m_items;
};

class UpdateMessage : public BaseCRUDMessage
{
public:
	explicit UpdateMessage(const QString &channel, const QString &table, const QVector<QJsonObject> &items);
	explicit UpdateMessage(const QString &channel, const QString &table, const QJsonObject &item);
	explicit UpdateMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items);

	QVector<QJsonObject> items() const { return m_items; }

	UpdateReplyMessage createSuccessReply() const;

private:
	QVector<QJsonObject> m_items;
};
class UpdateReplyMessage : public UpdateMessage
{
public:
	using UpdateMessage::UpdateMessage;
};

class DeleteMessage : public BaseCRUDMessage
{
public:
	explicit DeleteMessage(const QString &channel, const QString &table, const QVector<QVariant> &recordIds);
	explicit DeleteMessage(const QString &channel, const QString &table, const QVariant &recordId);
	explicit DeleteMessage(const Message &origin, const QString &table, const QVector<QVariant> &recordIds);

	QVector<QVariant> recordIds() const { return m_recordIds; }

	DeleteReplyMessage createSuccessReply() const;

private:
	QVector<QVariant> m_recordIds;
};
class DeleteReplyMessage : public DeleteMessage
{
public:
	using DeleteMessage::DeleteMessage;
};

class IndexMessage : public BaseCRUDMessage
{
public:
	explicit IndexMessage(const QString &channel, const QString &table);
	explicit IndexMessage(const Message &origin, const QString &table, const Filter &filter, const int limit, const int offset, const QPair<QString, Qt::SortOrder> &order, const int since);

	Filter filter() const { return m_filter; }
	int limit() const { return m_limit; }
	int offset() const { return m_offset; }
	QPair<QString, Qt::SortOrder> order() const { return m_order; }
	int since() const { return m_since; }

	bool hasOrder() const { return !m_order.first.isEmpty(); }

	IndexMessage &setFilter(const Filter &filter);
	IndexMessage &setLimit(const int limit);
	IndexMessage &setOffset(const int offset);
	IndexMessage &setOrder(const QPair<QString, Qt::SortOrder> &order);
	IndexMessage &setSince(const int since);

	IndexReplyMessage createSuccessReply(const QVector<QJsonObject> &items) const;

private:
	Filter m_filter;
	int m_limit = -1;
	int m_offset = -1;
	QPair<QString, Qt::SortOrder> m_order;
	int m_since = -1;
};
class IndexReplyMessage : public BaseCRUDMessage
{
public:
	explicit IndexReplyMessage(const Message &origin, const QString &table, const QVector<QJsonObject> &items);

	QVector<QJsonObject> items() const { return m_items; }

private:
	QVector<QJsonObject> m_items;
};
