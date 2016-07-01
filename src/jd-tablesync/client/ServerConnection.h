#pragma once

#include <QObject>
#include <QHash>
#include <QVector>

class QTcpSocket;
class QHostAddress;
class AbstractConsumer;

/**
 * The ServerConnection class maintains the connection to the server, parses incomming messages and distributes them
 * amongst the consumers that have been registered.
 *
 * Each consumer can be subscribed to one or more channels.
 */
class ServerConnection : public QObject
{
	Q_OBJECT
public:
	explicit ServerConnection(const QString &host, const quint16 port, QObject *parent = nullptr);
	~ServerConnection();

	void setAuthentication(const QString &data);

	/**
	 * @note You only need to call this if you previously called @see unregisterConsumer, the AbstractConsumer
	 * constructor calls this for you.
	 *
	 * Registers the consumer with this connection, taking ownership of it. Channels previously subscribed to will be
	 * re-subscribed to.
	 */
	void registerConsumer(AbstractConsumer *consumer);
	/**
	 * @note The AbstractConsumer destructor calls this for you, so you usually won't need to use this.
	 */
	void unregisterConsumer(AbstractConsumer *consumer);

public slots:
	/// Attempts to connect to the host, as specified in the constructor.
	void connectToHost();
	void disconnectFromHost();

signals:
	void message(const QString &msg);
	void disconnected();
	void connected();
	void error(const QString &error);
	void authenticationRequired();

private:
	friend class AbstractConsumer;
	void subscribeConsumerTo(AbstractConsumer *consumer, const QString &channel);
	void unsubscribeConsumerFrom(AbstractConsumer *consumer, const QString &channel);
	void sendFromConsumer(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo, const bool bypassAuth = false);

private slots:
	void socketChangedState();
	void socketError();
	void socketDataReady();

private: // low-level stuff
	QString m_host;
	quint16 m_port;
	QString m_authentication;
	bool m_needAuthentication;
	QTcpSocket *m_socket;

private: // higher-level stuff
	QList<QByteArray> m_messageQueue;
	QList<QByteArray> m_noAuthMessageQueue;
	QHash<QString, QVector<AbstractConsumer *>> m_subscriptions;
	QList<AbstractConsumer *> m_consumers;
};
