#include "AbstractActor.h"

#include <jd-util/Json.h>

#include "MessageHub.h"
#include "Message.h"
#include "Request.h"

Q_LOGGING_CATEGORY(Actor, "actor")

AbstractActor::AbstractActor(MessageHub *hub)
	: m_hub(hub)
{
	m_hub->registerActor(this);
}

AbstractActor::~AbstractActor()
{
	if (m_hub->actors().contains(this)) {
		m_hub->unregisterActor(this);
	}
}

void AbstractActor::subscribeTo(const QString &channel)
{
	m_hub->subscribeActorTo(this, channel);
	m_channels.insert(channel);
}
void AbstractActor::unsubscribeFrom(const QString &channel)
{
	m_hub->unsubscribeActorFrom(this, channel);
	m_channels.remove(channel);
}
QSet<QString> AbstractActor::channels() const
{
	return m_channels;
}

QUuid AbstractActor::send(const Message &msg)
{
	// AbstractThreadedActor subscribes by sending messages, intercept those here
	if (msg.channel() == "client") {
		if (msg.command() == "subscribe") {
			subscribeTo(Json::ensureString(msg.dataObject(), "channel"));
			return msg.id();
		} else if (msg.command() == "unsubscribe") {
			unsubscribeFrom(Json::ensureString(msg.dataObject(), "channel"));
			return msg.id();
		}
	}

	if (!m_channels.contains(msg.channel()) && !m_channels.contains("*") && msg.channel() != "client") {
		qCWarning(Messages) << "Sending a message on a channel not subscribed to. You probably don't mean to do this.";
	}

	Message message = msg;
	message.m_from = this;
	m_hub->messageFromActor(this, message);
	return message.id();
}

Request &AbstractActor::request(const Message &msg)
{
	return Request::create(m_hub, msg);
}
