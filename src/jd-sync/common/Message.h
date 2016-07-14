#pragma once

#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QFlags>
#include <QMetaType>

class AbstractActor;
class CreateMessage;
class ReadMessage;
class UpdateMessage;
class DeleteMessage;
class IndexMessage;
class CreateReplyMessage;
class ReadReplyMessage;
class UpdateReplyMessage;
class DeleteReplyMessage;
class IndexReplyMessage;
class ErrorMessage;

class Message
{
public:
	explicit Message(const QString &channel, const QString &cmd, const QJsonValue &data = QJsonValue());
	explicit Message();

	enum Flag
	{
		NoFlags = 0,
		BypassAuth = 1,
		Internal = 2
	};
	Q_DECLARE_FLAGS(Flags, Flag)

	QString channel() const { return m_channel; }
	QString command() const { return m_command; }
	QUuid id() const { return m_id; }
	QUuid replyTo() const { return m_replyTo; }
	int timestamp() const { return m_timestamp; }
	Flags flags() const { return m_flags; }

	AbstractActor *from() const { return m_from; }
	AbstractActor *to() const { return m_to; }

	QJsonValue data() const { return m_data; }
	QJsonObject dataObject() const;
	QJsonArray dataArray() const;

	bool isReply() const { return !m_replyTo.isNull(); }
	bool isBypassingAuth() const { return m_flags & BypassAuth; }
	bool isInternal() const { return m_flags & Internal; }
	bool isNull() const { return m_id.isNull(); }

	void setChannel(const QString &channel) { m_channel = channel; }
	void setCommand(const QString &command) { m_command = command; }
	void setData(const QJsonValue &data) { m_data = data; }
	void setTimestamp(const int timestamp) { m_timestamp = timestamp; }
	Message &setFlags(const Flags &flags) { m_flags = flags; return *this; }

	Message createReply(const QString &command, const QJsonValue &data) const;
	Message createReply(const QString &channel, const QString &command, const QJsonValue &data) const;
	Message createTargetedReply(const QString &command, const QJsonValue &data = QJsonValue()) const;
	ErrorMessage createErrorReply(const QString &msg) const;
	Message createCopy() const;

	QJsonObject toJson() const;
	static Message fromJson(const QJsonObject &obj);

	bool operator==(const Message &other) const;
	bool operator!=(const Message &other) const;

public: // message types
	bool isError() const;
	bool isCreate() const;
	bool isRead() const;
	bool isUpdate() const;
	bool isDelete() const;
	bool isIndex() const;
	bool isCreateReply() const;
	bool isReadReply() const;
	bool isUpdateReply() const;
	bool isDeleteReply() const;
	bool isIndexReply() const;

	ErrorMessage toError() const;
	CreateMessage toCreate() const;
	ReadMessage toRead() const;
	UpdateMessage toUpdate() const;
	DeleteMessage toDelete() const;
	IndexMessage toIndex() const;
	CreateReplyMessage toCreateReply() const;
	ReadReplyMessage toReadReply() const;
	UpdateReplyMessage toUpdateReply() const;
	DeleteReplyMessage toDeleteReply() const;
	IndexReplyMessage toIndexReply() const;

private:
	QString m_channel;
	QString m_command;
	QJsonValue m_data;
	QUuid m_id;
	QUuid m_replyTo;
	Flags m_flags = NoFlags;
	int m_timestamp = -1;

	friend class AbstractActor;
	friend class DummyActor; // for tests
	AbstractActor *m_from = nullptr;
	AbstractActor *m_to = nullptr;

	friend class MessageHub;
	explicit Message(const QString &channel, const QString &cmd, const QJsonValue &data, const QUuid &id, const QUuid &replyTo, const int timestamp);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Message::Flags)
Q_DECLARE_METATYPE(Message)

QDebug &operator<<(QDebug &dbg, const Message &msg);

class ErrorMessage : public Message
{
public:
	explicit ErrorMessage(const QString &msg, const Message &origin);
	explicit ErrorMessage(const QString &channel, const QString &msg);

	QString errorString() const { return m_message; }

private:
	QString m_message;
};
