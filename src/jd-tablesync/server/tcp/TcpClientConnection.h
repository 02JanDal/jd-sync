#pragma once

#include "../AbstractClientConnection.h"

class QTcpSocket;

class TcpClientConnection : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit TcpClientConnection(qintptr handle, const QString &auth);

	Q_INVOKABLE void setup();

protected:
	void toClient(const QJsonObject &obj) override;

private slots:
	void readyRead();
	void disconnected();

private:
	qintptr m_handle;
	QTcpSocket *m_socket;
	QString m_auth;

	QList<QJsonObject> m_inQueue, m_outQueue;
};
