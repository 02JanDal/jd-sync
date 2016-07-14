#include "AbstractThreadedActor.h"

#include <QEventLoop>
#include <jd-util/Exception.h>

#include "MessageHub.h"
#include "Message.h"

class MessagePasser : public QObject
{
	Q_OBJECT
public:
	explicit MessagePasser(AbstractThreadedActor *actor, QObject *parent = nullptr)
		: QObject(parent), m_actor(actor)
	{
	}

signals:
	void toActor(const Message &msg);

public slots:
	void fromActor(const Message &msg)
	{
		m_actor->AbstractActor::send(msg);
	}

private:
	AbstractThreadedActor *m_actor;
};

AbstractThreadedActor::AbstractThreadedActor(MessageHub *hub, QObject *parent)
	: QObject(), AbstractActor(hub), m_passer(new MessagePasser(this, parent)), m_thread(new QThread(parent))
{
	connect(m_passer, &MessagePasser::toActor, this, &AbstractThreadedActor::receivedFromPasser);
	connect(this, &AbstractThreadedActor::sendToPasser, m_passer, &MessagePasser::fromActor);

	moveToThread(m_thread);
	connect(m_thread, &QThread::started, this, [this]()
	{
		run();
		m_thread->quit();
	});
	connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
	connect(m_thread, &QThread::destroyed, this, [this]() { m_thread = nullptr; });
	m_thread->start();
}

AbstractThreadedActor::~AbstractThreadedActor()
{
	if (m_thread) {
		m_thread->quit();
	}
}

void AbstractThreadedActor::receive(const Message &message)
{
	// this will be called from the MessageHub thread, which is the same as the MessagePasser thread
	emit m_passer->toActor(message);
}

void AbstractThreadedActor::receivedFromPasser(const Message &message)
{
	try {
		received(message);
	} catch (Exception &e) {
		send(message.createErrorReply(e.cause()));
	}
}

void AbstractThreadedActor::send(const Message &message)
{
	emit sendToPasser(message);
}
void AbstractThreadedActor::subscribeTo(const QString &channel)
{
	send(Message("client", "subscribe", QJsonObject({{"channel", channel}})).setFlags(Message::Internal));
}
void AbstractThreadedActor::unsubscribeFrom(const QString &channel)
{
	send(Message("client", "unsubscribe", QJsonObject({{"channel", channel}})).setFlags(Message::Internal));
}

void AbstractThreadedActor::run()
{
	QEventLoop loop;
	loop.exec();
}

#include "AbstractThreadedActor.moc"
