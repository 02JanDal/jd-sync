#include "ConnectionManager.h"

#include "AbstractClientConnection.h"

ConnectionManager::ConnectionManager(QObject *parent)
	: QObject(parent)
{
}

void ConnectionManager::newConnection(AbstractClientConnection *connection)
{
	connect(connection, &QObject::destroyed, this, &ConnectionManager::connectionDestroyed);
	connect(connection, &AbstractClientConnection::newConnection, this, &ConnectionManager::newConnection);
	for (auto other : m_connections)
	{
		connect(connection, &AbstractClientConnection::broadcast, other, &AbstractClientConnection::receive);
		connect(other, &AbstractClientConnection::broadcast, connection, &AbstractClientConnection::receive);
	}
	m_connections.append(connection);
	QMetaObject::invokeMethod(connection, "ready", Qt::QueuedConnection);
}

void ConnectionManager::connectionDestroyed(QObject *connection)
{
	m_connections.removeAll(static_cast<AbstractClientConnection *>(connection));
}
