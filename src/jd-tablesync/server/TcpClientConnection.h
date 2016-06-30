#pragma once

#include "core/AbstractClientConnection.h"

class QTcpSocket;

class TcpClientConnection : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit TcpClientConnection(qintptr handle);

	Q_INVOKABLE void setup();

protected:
	void toClient(const QJsonObject &obj) override;

private slots:
	void readyRead();
	void disconnected();

private:
	qintptr m_handle;
	QTcpSocket *m_socket;
};
