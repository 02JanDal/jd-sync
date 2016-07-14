#pragma once

#include <QObject>
#include "AbstractActor.h"
#include "Message.h"

#include <functional>
#include <memory>

class TimeoutTimer;

class Request : public AbstractActor
{
public:
	INTROSPECTION

	explicit Request(MessageHub *hub, const Message &msg);
	~Request();

	template <typename... Args>
	using Callback = std::function<void(Args...)>;

	Request &then(const Callback<Message> &func);
	Request &error(const Callback<ErrorMessage> &func);
	Request &timeout(const Callback<> &func);

	Request &setTimeout(const int secs, const int retries = 0);
	Request &deleteOnFinished();

	Request &send();

	static Request &create(MessageHub *hub, const Message &msg);

private:
	Q_DISABLE_COPY(Request)
	friend class TimeoutTimer;

	Message m_message;
	Callback<Message> m_then;
	Callback<ErrorMessage> m_error;
	Callback<> m_timeout;
	int m_timeoutSecs = -1;
	int m_retriesOnTimeout = 0;
	bool m_deleteOnFinish = false;
	TimeoutTimer *m_timer = nullptr;

	void receive(const Message &message) override;
	void reset() override;
};

class RequestObject : public QObject
{
	Q_OBJECT
public:
	explicit RequestObject(MessageHub *hub, const Message &message, QObject *parent = nullptr);

public slots:
	void send();

signals:
	void reply(const Message &msg);
	void error(const Message &msg);
	void timeout();
	void finished();

private:
	std::unique_ptr<Request> m_request;
};
