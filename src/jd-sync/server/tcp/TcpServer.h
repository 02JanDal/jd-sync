#pragma once

#include <QHostAddress>
#include <QLoggingCategory>
#include "jd-sync/common/AbstractActor.h"

class TcpServer : public AbstractActor
{
public:
	explicit TcpServer(MessageHub *hub, const QHostAddress &address, const quint16 port);

	void setAuthentication(const QString &data);

	void start();

	quint16 port() const;

	static const char *formatAddress(const QHostAddress &address, const quint16 port);

protected:
	void receive(const Message &/*message*/) override {}

private:
	QHostAddress m_address;
	quint16 m_port;
	QString m_authentication;

	friend class TcpServerImpl;
	class TcpServerImpl *m_server;
};

Q_DECLARE_LOGGING_CATEGORY(Tcp)
