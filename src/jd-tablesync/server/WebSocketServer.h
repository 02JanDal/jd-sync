#pragma once

#include <QHostAddress>
#include "core/AbstractClientConnection.h"

class WebSocketServer : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit WebSocketServer(const QHostAddress &address, const quint16 port, QObject *parent = nullptr);

	void ready() override;

	static const char *formatAddress(const QHostAddress &address, const quint16 port);

private:
	void toClient(const QJsonObject &obj) override {}
	QHostAddress m_address;
	quint16 m_port;

	class WebSocketServerImpl *m_server;
};

Q_DECLARE_LOGGING_CATEGORY(WebSocket)
