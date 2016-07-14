#pragma once

#include <QQueue>

#include "common/AbstractExternalActor.h"
#include "common/Message.h"

class QTcpSocket;

class TcpClientConnection : public AbstractExternalActor
{
	Q_OBJECT
public:
	explicit TcpClientConnection(MessageHub *hub, qintptr handle, const QString &auth);

	void run() override;

protected:
	void sendToExternal(const Message &msg) override;

private slots:
	void readyRead();
	void disconnected();

private:
	qintptr m_handle;
	QTcpSocket *m_socket;
	QString m_auth;

	QQueue<Message> m_inQueue, m_outQueue;
	void sendQueue(QQueue<Message> *queue);
};
