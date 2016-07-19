#include "TcpServer.h"

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>

#include "TcpClientConnection.h"

Q_LOGGING_CATEGORY(Tcp, "tablesync.tcp.server")

class TcpServerImpl : public QTcpServer
{
public:
	explicit TcpServerImpl(TcpServer *server, QObject *parent = nullptr)
		: QTcpServer(parent), m_server(server) {}

protected:
	void incomingConnection(qintptr handle)
	{
		new TcpClientConnection(m_server->hub(), handle, m_server->m_authentication);
	}

private:
	TcpServer *m_server;
};

TcpServer::TcpServer(MessageHub *hub, const QHostAddress &address, const quint16 port)
	: AbstractActor(hub), m_address(address), m_port(port), m_server(new TcpServerImpl(this))
{
}

void TcpServer::setAuthentication(const QString &data)
{
	m_authentication = data;
}

void TcpServer::start()
{
	if (!m_server->listen(m_address, m_port)) {
		qCWarning(Tcp) << "Unable to start TCP server:" << m_server->errorString();
		std::exit(1);
	} else {
		qCInfo(Tcp) << "TCP server started on" << formatAddress(m_server->serverAddress(), m_server->serverPort());
	}
}

quint16 TcpServer::port() const
{
	return m_server->serverPort();
}

const char *TcpServer::formatAddress(const QHostAddress &address, const quint16 port)
{
	return QString("%1:%2").arg(address.toString()).arg(port).toUtf8().constData();
}
