#include "TcpClientConnection.h"

#include <QTcpSocket>

#include "common/Json.h"
#include "common/TcpUtils.h"
#include "TcpServer.h"

TcpClientConnection::TcpClientConnection(qintptr handle)
	: AbstractClientConnection(nullptr), m_handle(handle)
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

void TcpClientConnection::toClient(const QJsonObject &obj)
{
	TcpUtils::writePacket(m_socket, Json::toBinary(obj));
}

void TcpClientConnection::readyRead()
{
	while (m_socket->bytesAvailable() > 0)
	{
		QString channel;
		QUuid messageId;
		try
		{
			const QJsonObject obj = Json::ensureObject(Json::ensureDocument(TcpUtils::readPacket(m_socket)));
			channel = Json::ensureString(obj, "channel");
			messageId = Json::ensureUuid(obj, "msgId");
			fromClient(obj);
		}
		catch (Exception &e)
		{
			receive(channel, "error", {{"error", e.message()}}, messageId);
		}
	}
}

void TcpClientConnection::disconnected()
{
	qCDebug(Tcp) << TcpServer::formatAddress(m_socket->peerAddress(), m_socket->peerPort()) << "disconnected";
	m_socket = nullptr;
	deleteLater();
}
