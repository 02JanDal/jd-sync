#pragma once

#include <QHash>
#include <QSet>
#include <QLoggingCategory>

class AbstractActor;
class AbstractExternalActor;
class Message;

class MessageHub
{
public:
	explicit MessageHub();
	~MessageHub();

	void registerActor(AbstractActor *actor);
	void unregisterActor(AbstractActor *actor);

	QSet<AbstractActor *> actors() const { return m_actors; }

private:
	friend class AbstractActor;
	friend class AbstractExternalActor;
	/// @see AbstractActor::subscribeTo
	void subscribeActorTo(AbstractActor *actor, const QString &channel);
	/// @see AbstractActor::unsubscribeFrom
	void unsubscribeActorFrom(AbstractActor *actor, const QString &channel);
	/// @see AbstractActor::send
	void messageFromActor(AbstractActor *actor, const Message &message);

private:
	QHash<QString, QSet<AbstractActor *>> m_subscriptions;
	QSet<AbstractActor *> m_actors;

	void sendToAllActors(const Message &msg);
};

Q_DECLARE_LOGGING_CATEGORY(Messages)
