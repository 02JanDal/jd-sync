#pragma once

#include <QString>
#include <QSet>
#include <QLoggingCategory>

#include <jd-util/Introspection.h>

class MessageHub;
class Message;
class Request;

class AbstractActor
{
public:
	INTROSPECTION

	AbstractActor(MessageHub *hub);
	virtual ~AbstractActor();

	MessageHub *hub() const { return m_hub; }

protected:
	virtual void receive(const Message &msg) = 0;
	virtual void reset() {}

	void subscribeTo(const QString &channel);
	void unsubscribeFrom(const QString &channel);
	QSet<QString> channels() const;
	QUuid send(const Message &message);

	/// convenience function that creates a request with the same hub as this actor
	Request &request(const Message &msg);

private:
	Q_DISABLE_COPY(AbstractActor)

	friend class MessageHub;
	MessageHub *m_hub;
	QSet<QString> m_channels;
};

Q_DECLARE_LOGGING_CATEGORY(Actor)
