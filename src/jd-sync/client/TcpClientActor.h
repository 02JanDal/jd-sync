#pragma once

#include <QObject>
#include <QQueue>

#include "jd-sync/common/AbstractExternalActor.h"

class QTcpSocket;

class TcpClientActor final : public AbstractExternalActor
{
	Q_OBJECT
	INTROSPECTION
public:
	explicit TcpClientActor(MessageHub *hub, const QString &host, const quint16 port, QObject *parent = nullptr);


	enum State
	{
		Waiting,
		Connecting,
		Connected,
		Reconnecting
	};
	State state() const { return m_state; }

signals:
	void message(const QString &message);
	void authenticationRequired();
	void connected();
	void error(const QString &error);

public slots:
	void setAuthentication(const QString &data);
	void connectToHost();
	void disconnectFromHost();

private:
	void sendToExternal(const Message &message) override;
	void run() override;

private slots:
	void socketChangedState();
	void socketError();
	void socketDataReady();

private:
	QTcpSocket *m_socket = nullptr;
	QString m_host;
	quint16 m_port;

	State m_state = Waiting;

	QQueue<QByteArray> m_messagesQueue, m_noAuthMessageQueue;

	QString m_authentication;
	bool m_needAuthentication;

	void connectSocket();
	void sendQueue(QQueue<QByteArray> *queue);
};
