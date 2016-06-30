#include "ServerConnection.h"

#include <QTcpSocket>

#include "jd-util/Json.h"
#include "jd-tablesync/common/TcpUtils.h"
#include "AbstractConsumer.h"

ServerConnection::ServerConnection(const QString &host, const quint16 port, QObject *parent)
	: QObject(parent), m_host(host), m_port(port)
{
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::stateChanged, this, &ServerConnection::socketChangedState);
	connect(m_socket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &ServerConnection::socketError);
	connect(m_socket, &QTcpSocket::readyRead, this, &ServerConnection::socketDataReady);
}

ServerConnection::~ServerConnection()
{
	QList<AbstractConsumer *> tmp = m_consumers;
	for (AbstractConsumer *consumer : tmp)
	{
		delete consumer;
	}
}

void ServerConnection::registerConsumer(AbstractConsumer *consumer)
{
	m_consumers.append(consumer);
	for (const QString &channel : consumer->channels())
	{
		subscribeConsumerTo(consumer, channel);
	}
}
void ServerConnection::unregisterConsumer(AbstractConsumer *consumer)
{
	m_consumers.removeAll(consumer);
	for (const QString &channel : m_subscriptions.keys())
	{
		unsubscribeConsumerFrom(consumer, channel);
	}
}

void ServerConnection::connectToHost()
{
	m_socket->connectToHost(m_host, m_port);
}
void ServerConnection::disconnectFromHost()
{
	m_socket->disconnectFromHost();
}

void ServerConnection::subscribeConsumerTo(AbstractConsumer *consumer, const QString &channel)
{
	if (!m_subscriptions.contains(channel))
	{
		if (channel == "*")
		{
			sendFromConsumer("general", "monitor", {{"value", true}}, QUuid());
		}
		else
		{
			sendFromConsumer(channel, "subscribe", {}, QUuid());
		}
	}
	m_subscriptions[channel].append(consumer);
}
void ServerConnection::unsubscribeConsumerFrom(AbstractConsumer *consumer, const QString &channel)
{
	m_subscriptions[channel].removeAll(consumer);
	if (m_subscriptions[channel].isEmpty())
	{
		if (channel == "*")
		{
			sendFromConsumer("general", "monitor", {{"value", false}}, QUuid());
		}
		else
		{
			sendFromConsumer(channel, "unsubscribe", {}, QUuid());
		}
		m_subscriptions.remove(channel);
	}
}
void ServerConnection::sendFromConsumer(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo)
{
	QJsonObject obj = data;
	obj["channel"] = channel;
	obj["cmd"] = cmd;
	obj["msgId"] = Json::toJson(QUuid::createUuid());
	if (!replyTo.isNull())
	{
		obj["replyTo"] = Json::toJson(replyTo);
	}

	qDebug() << "sending" << obj;
	if (m_socket->state() == QTcpSocket::ConnectedState)
	{
		TcpUtils::writePacket(m_socket, Json::toBinary(obj));
	}
	else
	{
		m_messageQueue.append(Json::toBinary(obj));
	}
}

void ServerConnection::socketChangedState()
{
	switch (m_socket->state())
	{
	case QAbstractSocket::UnconnectedState:
		emit message(tr("Lost connection to host"));
		emit disconnected();
		break;
	case QAbstractSocket::HostLookupState:
		emit message(tr("Looking up host..."));
		break;
	case QAbstractSocket::ConnectingState:
		emit message(tr("Connecting to host..."));
		break;
	case QAbstractSocket::ConnectedState:
		emit message(tr("Connected!"));
		emit connected();
		for (const QByteArray &msg : m_messageQueue)
		{
			TcpUtils::writePacket(m_socket, msg);
		}
		break;
	case QAbstractSocket::BoundState:
		break;
	case QAbstractSocket::ListeningState:
		break;
	case QAbstractSocket::ClosingState:
		break;
	}
}
void ServerConnection::socketError()
{
	emit message(tr("Socket error: %1").arg(m_socket->errorString()));
}
void ServerConnection::socketDataReady()
{
	using namespace Json;

	while (m_socket->bytesAvailable() > 0)
	{
		QString channel;
		QUuid messageId;
		try
		{
			const QJsonObject obj = ensureObject(ensureDocument(TcpUtils::readPacket(m_socket)));
			channel = ensureString(obj, "channel");
			messageId = ensureUuid(obj, "msgId");
			const QString cmd = ensureString(obj, "cmd");
			qDebug() << "Got" << cmd << "on" << channel << ":" << obj;

			QList<AbstractConsumer *> notifiedConsumers;
			for (AbstractConsumer *consumer : m_subscriptions[channel])
			{
				consumer->consume(channel, cmd, obj);
				notifiedConsumers += consumer;
			}
			for (AbstractConsumer *consumer : m_subscriptions["*"])
			{
				if (!notifiedConsumers.contains(consumer))
				{
					consumer->consume(channel, cmd, obj);
				}
			}
		}
		catch (Exception &e)
		{
			sendFromConsumer(channel, "error", {{"error", e.cause()}}, messageId);
		}
	}
}
