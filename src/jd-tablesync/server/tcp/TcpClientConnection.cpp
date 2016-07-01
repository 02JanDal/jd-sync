#include "TcpClientConnection.h"

#include <QTcpSocket>

#include <jd-util/Json.h>

#include "../../common/TcpUtils.h"
#include "TcpServer.h"

TcpClientConnection::TcpClientConnection(qintptr handle, const QString &auth)
	: AbstractClientConnection(nullptr), m_handle(handle), m_auth(auth)
{
}

void TcpClientConnection::setup()
{
	m_socket = new QTcpSocket(this);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientConnection::readyRead);
	connect(m_socket, &QTcpSocket::disconnected, this, &TcpClientConnection::disconnected);
	m_socket->setSocketDescriptor(m_handle);
	qCDebug(Tcp) << "New TCP connection from" << TcpServer::formatAddress(m_socket->peerAddress(), m_socket->peerPort());
}

static bool isAuthMessage(const QJsonObject &obj)
{
	using namespace Json;
	return ensureString(obj, "channel") == "general" && ensureString(obj, "cmd").startsWith("auth.");
}
void TcpClientConnection::toClient(const QJsonObject &obj)
{
	if (m_auth.isNull() || isAuthMessage(obj)) {
		TcpUtils::writePacket(m_socket, Json::toBinary(obj));
		qCDebug(Tcp) << "sending" << obj;
	} else {
		m_outQueue.append(obj);
	}
}

void TcpClientConnection::readyRead()
{
	while (m_socket->bytesAvailable() > 0)
	{
		QString channel;
		QUuid messageId;
		try {
			const QJsonObject obj = Json::ensureObject(Json::ensureDocument(TcpUtils::readPacket(m_socket)));
			qCDebug(Tcp) << "received" << obj;
			channel = Json::ensureString(obj, "channel");
			messageId = Json::ensureUuid(obj, "msgId");
			const QString cmd = Json::ensureString(obj, "cmd");

			if (m_auth.isNull()) {
				fromClient(obj);
			} else if (channel == "general" && cmd == "auth.attempt") {
				if (Json::ensureString(obj, "data") == m_auth) {
					m_auth = QString();
					receive("general", "auth.response", {{"success", true}}, messageId);
					for (const QJsonObject &msg : m_outQueue) {
						TcpUtils::writePacket(m_socket, Json::toBinary(msg));
					}
					for (const QJsonObject &msg : m_inQueue) {
						fromClient(msg);
					}
				} else {
					receive("general", "auth.response", {{"success", false}}, messageId);
				}
			} else {
				receive("general", "auth.challenge", QJsonObject(), messageId);
				m_inQueue.append(obj);
			}
		} catch (Exception &e) {
			receive(channel, "error", {{"error", e.cause()}}, messageId);
		}
	}
}

void TcpClientConnection::disconnected()
{
	qCDebug(Tcp) << TcpServer::formatAddress(m_socket->peerAddress(), m_socket->peerPort()) << "disconnected";
	m_socket = nullptr;
	deleteLater();
}
