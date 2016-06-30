#pragma once

#include <QObject>

class AbstractClientConnection;

class ConnectionManager : public QObject
{
	Q_OBJECT
public:
	explicit ConnectionManager(QObject *parent = nullptr);

public slots:
	void newConnection(AbstractClientConnection *connection);
	void connectionDestroyed(QObject *connection);

private:
	QList<AbstractClientConnection *> m_connections;
};
