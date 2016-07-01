#pragma once

#include <QJsonObject>
#include <QUuid>
#include <QStringList>

class ServerConnection;

class AbstractConsumer
{
protected:
	explicit AbstractConsumer(ServerConnection *server);

public:
	virtual ~AbstractConsumer();

	virtual void consume(const QString &channel, const QString &cmd, const QJsonObject &data) = 0;
	virtual void connectionEstablished() {}

protected:
	friend class ServerConnection;
	void subscribeTo(const QString &channel);
	void unsubscribeFrom(const QString &channel);
	void emitMsg(const QString &channel, const QString &cmd, const QJsonObject &data = QJsonObject(), const QUuid &replyTo = QUuid());

private:
	ServerConnection *m_serverConnection;
	QStringList m_channels;
};
