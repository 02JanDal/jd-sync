#include "Request.h"

#include <QTimer>

#include <jd-util/Exception.h>

#include "RequestWaiter.h"

class TimeoutTimer : public QTimer
{
public:
	explicit TimeoutTimer(Request *request)
		: QTimer(nullptr)
	{
		connect(this, &TimeoutTimer::timeout, this, [request]()
		{
			if (request->m_retriesOnTimeout > 0) {
				--request->m_retriesOnTimeout;
				request->m_message = request->m_message.createCopy();
				if (request->m_timeout) {
					request->m_timeout();
				}
				request->send();
			} else {
				if (request->m_error) {
					request->m_error(ErrorMessage(request->m_message.channel(), "Timeout"));
				} else if (request->m_then) {
					request->m_error(ErrorMessage(request->m_message.channel(), "Timeout"));
				}
				if (request->m_waiter) {
					request->m_waiter->notifyDone(request);
				}
				if (request->m_deleteOnFinish) {
					QTimer::singleShot(0, Qt::CoarseTimer, [request]() { delete request; });
					delete request;
				}
				request->m_done = true;
			}
		});
		setInterval(request->m_timeoutSecs * 1000);
		setSingleShot(true);
		start();
	}
};

Request::Request(MessageHub *hub, const Message &msg)
	: AbstractActor(hub), m_message(msg) {}

Request::~Request()
{
	if (m_timer) {
		delete m_timer;
	}
	if (m_waiter) {
		m_waiter->notifyDone(this);
	}
}

Request &Request::then(const Callback<Message> &func)
{
	m_then = func;
	return *this;
}
Request &Request::error(const Callback<ErrorMessage> &func)
{
	m_error = func;
	return *this;
}
Request &Request::timeout(const Callback<> &func)
{
	m_timeout = func;
	return *this;
}
Request &Request::throwOnError()
{
	return error([](const ErrorMessage &msg)
	{
		throw Exception(msg.errorString());
	});
}
Request &Request::setTimeout(const int secs, const int retries)
{
	m_timeoutSecs = secs;
	m_retriesOnTimeout = retries;
	return *this;
}
Request &Request::deleteOnFinished()
{
	m_deleteOnFinish = true;
	return *this;
}

Request &Request::send()
{
	subscribeTo(m_message.channel());

	if (m_timeoutSecs != -1) {
		if (m_timer) {
			m_timer->start();
		} else {
			m_timer = new TimeoutTimer(this);
		}
	}

	AbstractActor::send(m_message);

	return *this;
}

void Request::sendAndWait()
{
	send();
	if (!m_done) {
		RequestWaiter::wait(this);
	}
}

Request &Request::create(MessageHub *hub, const Message &msg)
{
	return (new Request(hub, msg))->deleteOnFinished();
}

void Request::receive(const Message &message)
{
	if (!message.isReply() || message.channel() != m_message.channel() || message.replyTo() != m_message.id()) {
		return;
	}

	if (m_timer) {
		delete m_timer;
		m_timer = nullptr;
	}

	std::exception_ptr exception;
	try {
		if (message.command() == "error" && m_error) {
			m_error(message.toError());
		} else if (m_then) {
			m_then(message);
		}
	} catch (...) {
		exception = std::current_exception();
	}

	unsubscribeFrom(m_message.channel());

	if (m_waiter && !exception) {
		m_waiter->notifyDone(this);
	}
	if (m_deleteOnFinish) {
		QTimer::singleShot(0, Qt::CoarseTimer, [this]() { delete this; });
	}

	m_done = true;

	if (m_waiter && exception) {
		m_waiter->notifyException(exception);
	}
}

void Request::reset()
{
	// if the message was already sent we resend it upon a reset
	if (channels().contains(m_message.channel())) {
		m_message = m_message.createCopy();
		send();
	}
}

RequestObject::RequestObject(MessageHub *hub, const Message &message, QObject *parent)
	: QObject(parent), m_request(std::make_unique<Request>(hub, message))
{
	m_request->then([this](const Message &msg) { emit reply(msg); deleteLater(); });
	m_request->error([this](const Message &msg) { emit error(msg); deleteLater(); });
	m_request->timeout([this]() { emit timeout(); deleteLater(); });

	connect(this, &RequestObject::reply, &RequestObject::finished);
	connect(this, &RequestObject::error, &RequestObject::finished);
	connect(this, &RequestObject::timeout, &RequestObject::finished);
}
void RequestObject::send()
{
	m_request->send();
}
