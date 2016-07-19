#pragma once

#include <QObject>
#include "jd-sync/common/AbstractActor.h"

class Ping : public QObject, public AbstractActor
{
	Q_OBJECT
	Q_PROPERTY(int ping READ ping NOTIFY pingUpdated)
public:
	explicit Ping(MessageHub *hub, QObject *parent = 0);

	int ping() const { return m_ping; }

signals:
	void pingUpdated(const int ping);

private:
	void timerEvent(QTimerEvent *event) override;
	void receive(const Message &msg) override;
	void reset() override;

	int m_ping = -1;
	qint64 m_lastSentAt;
	bool m_pingPending = false;

	int m_sendPingTimer, m_updatePingTimer;
};
