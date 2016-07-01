#include "TcpServer.h"

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>

#include "TcpClientConnection.h"

Q_LOGGING_CATEGORY(Tcp, "core.tcp")

class TcpServerImpl : public QTcpServer
{
public:
	explicit TcpServerImpl(TcpServer *server, QObject *parent = nullptr)
		: QTcpServer(parent), m_server(server) {}

protected:
	void incomingConnection(qintptr handle)
	{
		TcpClientConnection *connection = new TcpClientConnection(handle, m_server->m_authentication);
		QThread *thread = new QThread;
		connection->moveToThread(thread);
		connect(connection, &TcpClientConnection::destroyed, thread, [thread](){thread->exit();});
		QMetaObject::invokeMethod(connection, "setup", Qt::QueuedConnection);
		thread->start();

		emit m_server->newConnection(connection);
	}

private:
	TcpServer *m_server;
};

TcpServer::TcpServer(const QHostAddress &address, const quint16 port, QObject *parent)
	: AbstractClientConnection(parent), m_address(address), m_port(port), m_server(new TcpServerImpl(this, this))
{
}

void TcpServer::setAuthentication(const QString &data)
{
	m_authentication = data;
}

void TcpServer::ready()
{
	if (!m_server->listen(m_address, m_port))
	{
		qWarning(Tcp) << "Unable to start TCP server:" << m_server->errorString();
		thread()->exit(1);
	}
	else
	{
		qCDebug(Tcp) << "TCP server started on" << formatAddress(m_server->serverAddress(), m_server->serverPort());
	}
}

const char *TcpServer::formatAddress(const QHostAddress &address, const quint16 port)
{
	return QString("%1:%2").arg(address.toString()).arg(port).toUtf8().constData();
}
