#pragma once

#include <QObject>
#include <QStringList>
#include <QUuid>
#include <QJsonObject>
#include <QLoggingCategory>

class AbstractClientConnection : public QObject
{
	Q_OBJECT
public:
	virtual ~AbstractClientConnection() {}

	Q_INVOKABLE virtual void ready() {}

public slots:
	/// This gets called by ConnectionManager, and formats the message before sending it out using toClient
	void receive(const QString &channel, const QString &cmd, const QJsonObject &data = QJsonObject(), const QUuid &replyTo = QUuid());

signals:
	/// This gets emitted by subclasses of AbstractClientConnection, and is routed to other's AbstractClientConnection::receive through ConnectionManager
	void broadcast(const QString &channel, const QString &cmd, const QJsonObject &data = QJsonObject(), const QUuid &replyTo = QUuid());

	/// Emit this if this AbstractClientConnection has produced a new AbstractClientConnection
	void newConnection(AbstractClientConnection *client);

protected:
	explicit AbstractClientConnection(QObject *parent);

	/// This should be called by the client when it receives data. Emits broadcast.
	void fromClient(const QJsonObject &obj);
	/// This should be reimplemented by the client to send data out. Called by receive.
	virtual void toClient(const QJsonObject &obj) = 0;

	void subscribeTo(const QString &channel);
	void unsubscribeFrom(const QString &channel);
	void setMonitor(const bool monitor);

private:
	QStringList m_channels;
	bool m_monitor = false; ///< If true, receives messages on all channels
};
