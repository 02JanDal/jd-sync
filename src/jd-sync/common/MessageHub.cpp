#include "MessageHub.h"

#include <jd-util/Exception.h>

#include "AbstractActor.h"
#include "Message.h"
#include <jd-util/Json.h>

Q_LOGGING_CATEGORY(Messages, "tablesync.messages")

MessageHub::MessageHub() {}
MessageHub::~MessageHub() {}

void MessageHub::registerActor(AbstractActor *actor)
{
	Q_ASSERT_X(!m_actors.contains(actor), "MessageHub::registerActor", "the given actor is already registered");
	Q_ASSERT_X(actor->m_hub == this, "MessageHub::registerActor", "attempting to register an actor belonging to a different hub");
	m_actors.insert(actor);
	// re-subscribe to channels
	for (const QString &channel : actor->m_channels) {
		subscribeActorTo(actor, channel);
	}
}
void MessageHub::unregisterActor(AbstractActor *actor)
{
	Q_ASSERT_X(m_actors.contains(actor), "MessageHub::unregisterActor", "the given actor is not yet registered");
	m_actors.remove(actor);
	for (const QString &channel : actor->m_channels) {
		unsubscribeActorFrom(actor, channel);
	}
}

void MessageHub::subscribeActorTo(AbstractActor *actor, const QString &channel)
{
	Q_ASSERT(actor);
	if (!m_subscriptions.contains(channel)) {
		sendToAllActors(Message("client", "subscribe", QJsonObject({{"channel", channel}})));
	}
	m_subscriptions[channel].insert(actor);
}
void MessageHub::unsubscribeActorFrom(AbstractActor *actor, const QString &channel)
{
	Q_ASSERT(actor);
	m_subscriptions[channel].remove(actor);
	if (m_subscriptions[channel].isEmpty()) {
		sendToAllActors(Message("client", "unsubscribe", QJsonObject({{"channel", channel}})));
		m_subscriptions.remove(channel);
	}
}

void MessageHub::messageFromActor(AbstractActor *actor, const Message &msg)
{
	qCDebug(Messages) << "routing" << msg;

	if (msg.to()) {
		msg.to()->receive(msg);
	} else if (msg.channel() == "client") {
		if (msg.command() == "reset") {
			for (AbstractActor *a : m_actors) {
				a->reset();
			}
		}
	} else {
		sendToAllActors(msg);
	}
}

void MessageHub::sendToAllActors(const Message &msg)
{
	auto actors = [this, msg]() { return (m_subscriptions.value(msg.channel()) + m_subscriptions.value("*")).toList().toSet(); };

	for (AbstractActor *a : actors()) {
		// actors might unsubscribe, or even get deleted, when handling a message, thus we double-check to make sure it's still there
		if (actors().contains(a) && a != msg.from()) {
			try {
				a->receive(msg);
			} catch (Exception &e) {
				messageFromActor(a, msg.createErrorReply(e.cause()));
			}
		}
	}
}
