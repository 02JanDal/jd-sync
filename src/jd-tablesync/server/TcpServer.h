#pragma once

#include <QHostAddress>
#include "core/AbstractClientConnection.h"

class TcpServer : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit TcpServer(const QHostAddress &address, const quint16 port, QObject *parent = nullptr);

	void ready() override;

	static const char *formatAddress(const QHostAddress &address, const quint16 port);

protected:
	void toClient(const QJsonObject &obj) override {}

private:
	QHostAddress m_address;
	quint16 m_port;

	class TcpServerImpl *m_server;
};

Q_DECLARE_LOGGING_CATEGORY(Tcp)
