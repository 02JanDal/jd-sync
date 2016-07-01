#include "WebSocketServer.h"

#include <QThread>
#include <QWebSocketServer>
#include <QUrl>

#include "WebSocketClientConnection.h"

Q_LOGGING_CATEGORY(WebSocket, "core.websocket")

class WebSocketServerImpl : public QWebSocketServer
{
	Q_OBJECT
public:
	explicit WebSocketServerImpl(WebSocketServer *server, QObject *parent = nullptr)
		: QWebSocketServer("", NonSecureMode, parent), m_server(server)
	{
		connect(this, &QWebSocketServer::newConnection, this, &WebSocketServerImpl::newConnectionReceived);
	}

private slots:
	void newConnectionReceived()
	{
		while (hasPendingConnections())
		{
			emit m_server->newConnection(new WebSocketClientConnection(nextPendingConnection()));
		}
	}

private:
	WebSocketServer *m_server;
};

WebSocketServer::WebSocketServer(const QHostAddress &address, const quint16 port, QObject *parent)
	: AbstractClientConnection(parent), m_address(address), m_port(port)
{
}

void WebSocketServer::ready()
{
	m_server = new WebSocketServerImpl(this, this);
	if (!m_server->listen(m_address, m_port))
	{
		qWarning(WebSocket) << "Unable to start WebSocket server:" << m_server->errorString();
		thread()->exit(1);
	}
	else
	{
		qCDebug(WebSocket) << "WebSocket server started on" << formatAddress(m_server->serverAddress(), m_server->serverPort());
	}
}

const char *WebSocketServer::formatAddress(const QHostAddress &address, const quint16 port)
{
	return QString("%1:%2").arg(address.toString()).arg(port).toUtf8().constData();
}


#include "WebSocketServer.moc"
