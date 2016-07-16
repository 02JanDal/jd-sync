#include "Message.h"

#include <QDebug>

#include <jd-util/Json.h>

#include "CRUDMessages.h"
#include "AbstractActor.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wglobal-constructors")
static int metaType = qRegisterMetaType<Message>();
QT_WARNING_POP

Message::Message(const QString &channel, const QString &cmd, const QJsonValue &data)
	: m_channel(channel), m_command(cmd), m_data(data), m_id(QUuid::createUuid()) {}
Message::Message(const QString &channel, const QString &cmd, const QJsonValue &data, const QUuid &id, const QUuid &replyTo, const int timestamp)
	: m_channel(channel), m_command(cmd), m_data(data), m_id(id), m_replyTo(replyTo), m_timestamp(timestamp) {}
Message::Message() {}

QJsonObject Message::dataObject() const
{
	return Json::ensureObject(m_data);
}
QJsonArray Message::dataArray() const
{
	return Json::ensureArray(m_data);
}

Message Message::createReply(const QString &command, const QJsonValue &data) const
{
	Q_ASSERT_X(!isNull(), "Message::createReply", "cannot reply to null message without an explicit channel");
	Message reply{m_channel, command, data};
	reply.m_replyTo = m_id;
	return reply;
}
Message Message::createReply(const QString &channel, const QString &command, const QJsonValue &data) const
{
	Message reply{channel, command, data};
	reply.m_replyTo = m_id;
	return reply;
}
Message Message::createTargetedReply(const QString &command, const QJsonValue &data) const
{
	Q_ASSERT_X(!isNull(), "Message::createReply", "cannot reply to null message without an explicit channel");
	Message reply{m_channel, command, data};
	reply.m_replyTo = m_id;
	reply.m_to = m_from;
	return reply;
}
ErrorMessage Message::createErrorReply(const QString &msg) const
{
	return ErrorMessage(msg,
						createTargetedReply("error", QJsonObject({{"msg", msg}})));
}

Message Message::createCopy() const
{
	Message copy = *this;
	copy.m_id = QUuid::createUuid();
	return copy;
}

QJsonObject Message::toJson() const
{
	QJsonObject obj = QJsonObject({
						   {"ch", m_channel},
						   {"cmd", m_command},
						   {"id", Json::toJson(m_id)}
					   });
	if (!m_replyTo.isNull()) {
		obj.insert("reply", Json::toJson(m_replyTo));
	}
	if (!m_data.isNull()) {
		obj.insert("data", m_data);
	}
	if (m_timestamp != -1) {
		obj.insert("timestamp", m_timestamp);
	}
	return obj;
}

Message Message::fromJson(const QJsonObject &obj)
{
	using namespace Json;
	return Message(
				ensureString(obj, "ch"),
				ensureString(obj, "cmd"),
				obj.value("data"),
				ensureUuid(obj, "id"),
				ensureUuid(obj, "reply", QUuid()),
				ensureInteger(obj, "timestamp", -1)
				);
}

bool Message::operator==(const Message &other) const
{
	return m_command == other.m_command && m_channel == other.m_channel && m_data == other.m_data;
}
bool Message::operator!=(const Message &other) const
{
	return !operator==(other);
}

bool Message::isError() const
{
	return m_command == "error";
}
bool Message::isCreate() const
{
	return m_command == "create";
}
bool Message::isRead() const
{
	return m_command == "read";
}
bool Message::isUpdate() const
{
	return m_command == "update";
}
bool Message::isDelete() const
{
	return m_command == "delete";
}
bool Message::isIndex() const
{
	return m_command == "index";
}
bool Message::isCreateReply() const
{
	return m_command == "create:result";
}
bool Message::isReadReply() const
{
	return m_command == "read:result";
}
bool Message::isUpdateReply() const
{
	return m_command == "update:result";
}
bool Message::isDeleteReply() const
{
	return m_command == "delete:result";
}
bool Message::isIndexReply() const
{
	return m_command == "index:result";
}

ErrorMessage Message::toError() const
{
	Q_ASSERT_X(isError(), "Message::toError", "invalid message conversion");
	return ErrorMessage(Json::ensureString(dataObject(), "msg"), *this);
}
CreateMessage Message::toCreate() const
{
	Q_ASSERT_X(isCreate(), "Message::toCreate", "invalid message conversion");
	return CreateMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}
ReadMessage Message::toRead() const
{
	Q_ASSERT_X(isRead(), "Message::toRead", "invalid message conversion");
	return ReadMessage(*this,
					   Json::ensureString(dataObject(), "table"),
					   Json::ensureIsArrayOf<QVariant>(dataObject(), "ids"),
					   Json::ensureIsArrayOf<QString>(dataObject(), "properties", QVector<QString>()));
}
UpdateMessage Message::toUpdate() const
{
	Q_ASSERT_X(isUpdate(), "Message::toUpdate", "invalid message conversion");
	return UpdateMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}
DeleteMessage Message::toDelete() const
{
	Q_ASSERT_X(isDelete(), "Message::toDelete", "invalid message conversion");
	return DeleteMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QVariant>(dataObject(), "ids"));
}
IndexMessage Message::toIndex() const
{
	Q_ASSERT_X(isIndex(), "Message::toIndex", "invalid message conversion");
	const QJsonObject obj = dataObject();
	return IndexMessage(*this,
						Json::ensureString(obj, "table"),
						obj.contains("filter") ? Filter::fromJson(Json::ensureObject(obj, "filter")) : Filter(),
						Json::ensureInteger(obj, "limit", -1),
						Json::ensureInteger(obj, "offset", -1),
						qMakePair(Json::ensureString(obj, "order", QString()), Json::ensureBoolean(obj, "orderAsc", true) ? Qt::AscendingOrder : Qt::DescendingOrder),
						Json::ensureInteger(obj, "since", -1)
						);
}

CreateReplyMessage Message::toCreateReply() const
{
	Q_ASSERT_X(isCreateReply(), "Message::toCreateReply", "invalid message conversion");
	return CreateReplyMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}
ReadReplyMessage Message::toReadReply() const
{
	Q_ASSERT_X(isReadReply(), "Message::toReadReply", "invalid message conversion");
	return ReadReplyMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}
UpdateReplyMessage Message::toUpdateReply() const
{
	Q_ASSERT_X(isUpdateReply(), "Message::toUpdateReply", "invalid message conversion");
	return UpdateReplyMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}
DeleteReplyMessage Message::toDeleteReply() const
{
	Q_ASSERT_X(isDeleteReply(), "Message::toDeleteReply", "invalid message conversion");
	return DeleteReplyMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QVariant>(dataObject(), "ids"));
}
IndexReplyMessage Message::toIndexReply() const
{
	Q_ASSERT_X(isIndexReply(), "Message::toIndexReply", "invalid message conversion");
	return IndexReplyMessage(*this, Json::ensureString(dataObject(), "table"), Json::ensureIsArrayOf<QJsonObject>(dataObject(), "items"));
}

QDebug &operator<<(QDebug &dbg, const Message &msg)
{
	QDebugStateSaver saver(dbg);

	if (msg.isNull()) {
		dbg << "Message(Null)";
		return dbg;
	}

	const QString id = msg.id().toString();

	if (msg.isError()) {
		dbg.nospace().noquote() << "ErrorMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " message=" << msg.toError().errorString();
	} else if (msg.isCreate()) {
		dbg.nospace().noquote() << "CreateMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toCreate().table() << " items=" << msg.toCreate().items();
	} else if (msg.isRead()) {
		dbg.nospace().noquote() << "ReadMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toRead().table() << " ids=" << msg.toRead().recordIds();
	} else if (msg.isUpdate()) {
		dbg.nospace().noquote() << "UpdateMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toUpdate().table() << " items=" << msg.toUpdate().items();
	} else if (msg.isDelete()) {
		dbg.nospace().noquote() << "DeleteMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toDelete().table() << " ids=" << msg.toDelete().recordIds();
	} else if (msg.isIndex()) {
		const IndexMessage index = msg.toIndex();
		dbg.nospace().noquote() << "IndexMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << index.table();
		if (!index.filter().isEmpty()) {
			dbg.nospace().noquote() << " filter=" << Json::toText(index.filter().toJson());
			dbg.quote();
		}
		if (index.hasOrder()) {
			dbg.nospace() << " order={" << index.order().first << "," << (index.order().second == Qt::AscendingOrder ? "Ascending" : "Descending") << "}";
		}
		if (index.limit() != -1) {
			dbg.nospace() << " limit=" << index.limit();
		}
		if (index.offset() != -1) {
			dbg.nospace() << " offset=" << index.offset();
		}
		if (index.since() != -1) {
			dbg.nospace() << " since=" << index.since();
		}
	} else if (msg.isCreateReply()) {
		dbg.nospace().noquote() << "CreateReplyMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toCreateReply().table() << " items=" << msg.toCreateReply().items();
	} else if (msg.isReadReply()) {
		dbg.nospace().noquote() << "ReadReplyMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toReadReply().table() << " items=" << msg.toReadReply().items();
	} else if (msg.isUpdateReply()) {
		dbg.nospace().noquote() << "UpdateReplyMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toUpdateReply().table() << " items=" << msg.toUpdateReply().items();
	} else if (msg.isDeleteReply()) {
		dbg.nospace().noquote() << "DeleteReplyMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toDeleteReply().table() << " ids=" << msg.toDeleteReply().recordIds();
	} else if (msg.isIndexReply()) {
		dbg.nospace().noquote() << "IndexReplyMessage(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " table=" << msg.toIndexReply().table() << " items=" << msg.toIndexReply().items();
	} else {
		dbg.nospace().noquote() << "Message(id=" << id;
		dbg.nospace().quote() << " ch=" << msg.channel() << " cmd=" << msg.command();
		if (!msg.data().isNull()) {
			dbg.nospace() << " data=";
			if (msg.data().isObject()) {
				dbg.nospace().noquote() << Json::toText(msg.dataObject());
			} else if (msg.data().isArray()) {
				dbg.nospace().noquote() << Json::toText(msg.dataArray());
			} else if (msg.data().isBool()) {
				dbg.nospace() << msg.data().toBool();
			} else if (msg.data().isDouble()) {
				dbg.nospace() << msg.data().toDouble();
			} else if (msg.data().isString()) {
				dbg.nospace() << msg.data().toString();
			}
		}
	}

	if (msg.flags() != Message::NoFlags) {
		dbg.nospace() << " flags=" << msg.flags();
	}
	if (msg.isReply()) {
		dbg.nospace().noquote() << " replyTo=" << msg.replyTo().toString();
	}
	if (msg.to()) {
		dbg.nospace().noquote() << " to=" << msg.to()->className() << '(' << msg.to() << ')';
	}
	if (msg.from()) {
		dbg.nospace().noquote() << " from=" << msg.from()->className() << '(' << msg.from() << ')';
	}

	dbg.nospace().quote() << ")";
	return dbg.maybeSpace();
}

ErrorMessage::ErrorMessage(const QString &msg, const Message &origin)
	: Message(origin), m_message(msg) {}
ErrorMessage::ErrorMessage(const QString &channel, const QString &msg)
	: Message(channel, "error", QJsonObject({{"msg", msg}})), m_message(msg) {}
