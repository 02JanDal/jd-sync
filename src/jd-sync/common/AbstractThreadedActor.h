#pragma once

#include <QThread>
#include "AbstractActor.h"

class AbstractThreadedActor : public QObject, public AbstractActor
{
	Q_OBJECT
public:
	INTROSPECTION

	explicit AbstractThreadedActor(MessageHub *hub, QObject *parent = nullptr);
	virtual ~AbstractThreadedActor();

private:
	friend class MessagePasser;
	class MessagePasser *m_passer;
	void receive(const Message &message) override final;

private slots:
	void receivedFromPasser(const Message &message);
signals:
	void sendToPasser(const Message &message);

protected:
	virtual void received(const Message &message) = 0;
	void send(const Message &message);
	void subscribeTo(const QString &channel);
	void unsubscribeFrom(const QString &channel);

	virtual void run();

private:
	QThread *m_thread;
};
