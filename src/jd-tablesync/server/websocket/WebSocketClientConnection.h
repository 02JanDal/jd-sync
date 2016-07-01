#pragma once

#include <QObject>

#include "core/AbstractClientConnection.h"

class QWebSocket;

class WebSocketClientConnection : public AbstractClientConnection
{
	Q_OBJECT
public:
	explicit WebSocketClientConnection(QWebSocket *socket, QObject *parent = nullptr);

private slots:
	void binaryReceived(const QByteArray &msg);
	void textReceived(const QString &msg);
	void disconnected();

protected:
	void toClient(const QJsonObject &obj) override;

private:
	QWebSocket *m_socket = nullptr;
};
