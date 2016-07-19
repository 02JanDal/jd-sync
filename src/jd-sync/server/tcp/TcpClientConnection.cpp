#include "TcpClientConnection.h"

#include <QTcpSocket>

#include <jd-util/Json.h>

#include "common/TcpUtils.h"
#include "common/Message.h"
#include "TcpServer.h"

TcpClientConnection::TcpClientConnection(MessageHub *hub, qintptr handle, const QString &auth)
	: AbstractExternalActor(hub), m_handle(handle), m_auth(auth)
{
}

void TcpClientConnection::run()
{
	Q_ASSERT_X(!m_socket, "TcpClientConnection::run", "attempt to re-run");
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientConnection::readyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &TcpClientConnection::disconnected);
	m_socket->setSocketDescriptor(m_handle);
	qCInfo(Tcp) << "New TCP connection from" << TcpServer::formatAddress(m_socket->peerAddress(), m_socket->peerPort());

	AbstractThreadedActor::run();
}

static bool isAuthMessage(const Message &msg)
{
	using namespace Json;
	return msg.channel() == "client" && msg.command().startsWith("auth.");
}
void TcpClientConnection::sendToExternal(const Message &msg)
{
	if (m_auth.isNull() || isAuthMessage(msg) || msg.command() == "error") {
		TcpUtils::writePacket(m_socket, Json::toBinary(msg.toJson()));
		qCDebug(Tcp) << "sending" << msg.toJson();
	} else {
		m_outQueue.enqueue(msg);
	}
}

void TcpClientConnection::readyRead()
{
	while (m_socket->bytesAvailable() > 0)
	{
		Message msg;
		try {
			const QJsonDocument doc = Json::ensureDocument(TcpUtils::readPacket(m_socket));
			msg = Message::fromJson(Json::ensureObject(doc));
			qCDebug(Tcp) << "received" << msg.toJson();

			if (msg.channel() == "client.ping" && msg.command() == "request") {
				sendToExternal(msg.createTargetedReply("reply", msg.data()));
			} else if (m_auth.isNull()) {
				receivedFromExternal(msg);
			} else if (msg.channel() == "client.auth" && msg.command() == "attempt") {
				if (Json::ensureString(msg.dataObject(), "token") == m_auth) {
					m_auth = QString();
					sendToExternal(msg.createReply("response", QJsonObject({{"success", true}})));
					sendQueue(&m_outQueue);
					for (const Message &message : m_inQueue) {
						receivedFromExternal(message);
					}
				} else {
					sendToExternal(msg.createReply("response", QJsonObject({{"success", false}})));
				}
			} else {
				sendToExternal(msg.createReply("challenge", QJsonValue()));
				m_inQueue.enqueue(msg);
			}
		} catch (Exception &e) {
			if (msg.isNull()) {
				qCWarning(Tcp) << e.cause();
			} else {
				sendToExternal(msg.createErrorReply(e.cause()));
			}
		}
	}
}

void TcpClientConnection::disconnected()
{
	qCInfo(Tcp) << TcpServer::formatAddress(m_socket->peerAddress(), m_socket->peerPort()) << "disconnected";
	m_socket = nullptr;
	deleteLater();
}

void TcpClientConnection::sendQueue(QQueue<Message> *queue)
{
	while (m_socket->isOpen() && m_socket->isWritable() && !queue->isEmpty()) {
		TcpUtils::writePacket(m_socket, Json::toBinary(queue->dequeue().toJson()));
	}
}
