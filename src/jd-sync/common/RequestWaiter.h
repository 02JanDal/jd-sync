#pragma once

#include <QSet>
#include <exception>

class Request;
class QEventLoop;

class RequestWaiter
{
public:
	explicit RequestWaiter();
	~RequestWaiter();

	void add(Request &request);
	void add(Request *request);

	void wait();
	void reset();

	static void wait(Request *request);

private:
	QSet<Request *> m_pendingRequests;
	QEventLoop *m_loop = nullptr;
	std::exception_ptr m_exception;

	friend class Request;
	friend class TimeoutTimer;
	void notifyDone(Request *request);
	void notifyException(std::exception_ptr exception);
};

inline RequestWaiter &operator<<(RequestWaiter &waiter, Request &request) { waiter.add(request); return waiter; }
inline RequestWaiter &operator<<(RequestWaiter &waiter, Request *request) { waiter.add(request); return waiter; }
