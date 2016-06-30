#include "AbstractClientConnection.h"

#include "jd-util/Json.h"

AbstractClientConnection::AbstractClientConnection(QObject *parent)
	: QObject(parent)
{
}

void AbstractClientConnection::receive(const QString &channel, const QString &cmd, const QJsonObject &data, const QUuid &replyTo)
{
	if (!m_channels.contains(channel) && !m_monitor)
	{
		return;
	}
	QJsonObject obj = data;
	obj["channel"] = channel;
	obj["cmd"] = cmd;
	obj["msgId"] = Json::toJson(QUuid::createUuid());
	if (!replyTo.isNull())
	{
		obj["replyTo"] = Json::toJson(replyTo);
	}
	toClient(obj);
}

void AbstractClientConnection::fromClient(const QJsonObject &obj)
{
	using namespace Json;

	const QString channel = ensureString(obj, "channel", "");
	const QString cmd = ensureString(obj, "cmd");

	if (cmd == "ping")
	{
		toClient({{"cmd", "pong"}, {"channel", ""}, {"timestamp", ensureInteger(obj, "timestamp")}});
	}
	else if (cmd == "subscribe")
	{
		subscribeTo(channel);
	}
	else if (cmd == "unsubscribe")
	{
		unsubscribeFrom(channel);
	}
	else if (cmd == "monitor")
	{
		setMonitor(ensureBoolean(obj, QStringLiteral("value")));
	}
	else
	{
		emit broadcast(channel, cmd, obj);
	}
}

void AbstractClientConnection::subscribeTo(const QString &channel)
{
	m_channels.append(channel);
}
void AbstractClientConnection::unsubscribeFrom(const QString &channel)
{
	m_channels.removeAll(channel);
}
void AbstractClientConnection::setMonitor(const bool monitor)
{
	m_monitor = monitor;
}
