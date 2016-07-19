#include "Ping.h"

#include <QTimerEvent>
#include <QDateTime>

#include "common/Message.h"

Ping::Ping(MessageHub *hub, QObject *parent)
	: QObject(parent), AbstractActor(hub)
{
	subscribeTo("client.ping");

	m_sendPingTimer = startTimer(30 * 1000, Qt::CoarseTimer);
	m_updatePingTimer = startTimer(1 * 1000, Qt::CoarseTimer);

	send(Message("client.ping", "request", m_lastSentAt = QDateTime::currentMSecsSinceEpoch()));
	m_pingPending = true;
}

void Ping::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_sendPingTimer) {
		if (!m_pingPending) {
			send(Message("client.ping", "request", m_lastSentAt = QDateTime::currentMSecsSinceEpoch()));
			m_pingPending = true;
		}
	} else if (event->timerId() == m_updatePingTimer) {
		if (m_pingPending) {
			m_ping = QDateTime::currentMSecsSinceEpoch() - m_lastSentAt;
			emit pingUpdated(m_ping);
		}
	}
}

void Ping::receive(const Message &msg)
{
	if (msg.channel() == "client.ping" && msg.command() == "reply") {
		m_pingPending = false;
		m_ping = QDateTime::currentMSecsSinceEpoch() - m_lastSentAt;
		emit pingUpdated(m_ping);
	}
}

void Ping::reset()
{
	m_ping = -1;
	emit pingUpdated(m_ping);
	send(Message("client.ping", "request", m_lastSentAt = QDateTime::currentMSecsSinceEpoch()));
	m_pingPending = true;
}
