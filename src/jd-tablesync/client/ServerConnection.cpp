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
	for (AbstractConsumer *consumer : tmp) {
		delete consumer;
	}
}

void ServerConnection::setAuthentication(const QString &data)
{
	m_authentication = data;
	m_needAuthentication = true;

	if (m_socket->isOpen()) {
		sendFromConsumer("general", "auth.attempt", {{"data", m_authentication}}, QUuid(), true);
	}
}

void ServerConnection::registerConsumer(AbstractConsumer *consumer)
{
	Q_ASSERT(!m_consumers.contains(consumer));
	Q_ASSERT(consumer->m_serverConnection == this);
	m_consumers.append(consumer);
	for (const QString &channel : consumer->m_channels)
	{
		subscribeConsumerTo(consumer, channel);
	}
}
void ServerConnection::unregisterConsumer(AbstractConsumer *consumer)
{
	Q_ASSERT(m_consumers.contains(consumer));
	Q_ASSERT(consumer->m_serverConnection == this);
	m_consumers.removeAll(consumer);
	for (const QString &channel : consumer->m_channels)
	{
		unsubscribeConsumerFrom(consumer, channel);
	}
}

void ServerConnection::connectToHost()
{
	Q_ASSERT(!m_socket->isOpen() && m_socket->state() != QTcpSocket::ConnectingState);
	m_socket->connectToHost(m_host, m_port);
}
void ServerConnection::disconnectFromHost()
{
	Q_ASSERT(m_socket->isOpen());
	m_socket->disconnectFromHost();
}

void ServerConnection::subscribeConsumerTo(AbstractConsumer *consumer, const QString &channel)
{
	if (!m_subscriptions.contains(channel)) {
		if (channel == "*") {
			sendFromConsumer("general", "monitor", {{"value", true}}, QUuid());
		} else {
			sendFromConsumer(channel, "subscribe", {}, QUuid());
		}
	}
	m_subscriptions[channel].append(consumer);
}
void ServerConnection::unsubscribeConsumerFrom(AbstractConsumer *consumer, const QString &channel)
{
	m_subscriptions[channel].removeAll(consumer);
	if (m_subscriptions[channel].isEmpty()) {
		if (channel == "*") {
			sendFromConsumer("general", "monitor", {{"value", false}}, QUuid());
		} else {
			sendFromConsumer(channel, "unsubscribe", {}, QUuid());
		}
		m_subscriptions.remove(channel);
	}
}
void ServerConnection::sendFromConsumer(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo, const bool bypassAuth)
{
	QJsonObject obj = data;
	obj["channel"] = channel;
	obj["cmd"] = cmd;
	obj["msgId"] = Json::toJson(QUuid::createUuid());
	if (!replyTo.isNull()) {
		obj["replyTo"] = Json::toJson(replyTo);
	}

	qDebug() << "sending" << obj;
	if (m_socket->state() == QTcpSocket::ConnectedState && (!m_needAuthentication || bypassAuth)) {
		TcpUtils::writePacket(m_socket, Json::toBinary(obj));
	} else if (bypassAuth) {
		m_noAuthMessageQueue.append(Json::toBinary(obj));
	} else {
		m_messageQueue.append(Json::toBinary(obj));
	}
}

void ServerConnection::socketChangedState()
{
	switch (m_socket->state()) {
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
		for (const QByteArray &msg : m_noAuthMessageQueue) {
			TcpUtils::writePacket(m_socket, msg);
		}
		if (m_needAuthentication) {
			emit message(tr("Authenticating..."));
			sendFromConsumer("general", "auth.attempt", {{"data", m_authentication}}, QUuid(), true);
		} else {
			emit message(tr("Connected!"));
			emit connected();
			for (const QByteArray &msg : m_messageQueue) {
				TcpUtils::writePacket(m_socket, msg);
			}
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
	emit error(tr("Socker error: %1").arg(m_socket->errorString()));
}
void ServerConnection::socketDataReady()
{
	using namespace Json;

	while (m_socket->bytesAvailable() > 0) {
		QString channel;
		QUuid messageId;
		try {
			const QJsonObject obj = ensureObject(ensureDocument(TcpUtils::readPacket(m_socket)));
			qDebug() << "received" << obj;
			channel = ensureString(obj, "channel");
			messageId = ensureUuid(obj, "msgId");
			const QString cmd = ensureString(obj, "cmd");

			if (channel == "general") {
				if (cmd == "auth.challenge") {
					emit authenticationRequired();
				} else if (cmd == "auth.response") {
					if (ensureBoolean(obj, QStringLiteral("success"))) {
						m_needAuthentication = false;
						emit message(tr("Connected"));
						emit connected();
						for (const QByteArray &msg : m_messageQueue) {
							TcpUtils::writePacket(m_socket, msg);
						}
					} else {
						emit authenticationRequired();
					}
				}
			}

			QList<AbstractConsumer *> notifiedConsumers;
			// begin by handling explicit subscriptions...
			for (AbstractConsumer *consumer : m_subscriptions[channel]) {
				consumer->consume(channel, cmd, obj);
				notifiedConsumers += consumer;
			}
			// ...and follow up with monitoring consumers
			for (AbstractConsumer *consumer : m_subscriptions["*"]) {
				if (!notifiedConsumers.contains(consumer))
				{
					consumer->consume(channel, cmd, obj);
				}
			}
		} catch (Exception &e) {
			sendFromConsumer(channel, "error", {{"error", e.cause()}}, messageId);
		}
	}
}
