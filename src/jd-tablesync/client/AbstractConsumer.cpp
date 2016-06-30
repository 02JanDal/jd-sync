#include "AbstractConsumer.h"

#include "ServerConnection.h"

AbstractConsumer::AbstractConsumer(ServerConnection *server)
	: m_serverConnection(server)
{
	m_serverConnection->registerConsumer(this);
}

AbstractConsumer::~AbstractConsumer()
{
	m_serverConnection->unregisterConsumer(this);
}

void AbstractConsumer::subscribeTo(const QString &channel)
{
	m_serverConnection->subscribeConsumerTo(this, channel);
	m_channels.append(channel);
}
void AbstractConsumer::unsubscribeFrom(const QString &channel)
{
	m_serverConnection->unsubscribeConsumerFrom(this, channel);
	m_channels.removeAll(channel);
}
void AbstractConsumer::emitMsg(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo)
{
	m_serverConnection->sendFromConsumer(channel, cmd, data, replyTo);
}
