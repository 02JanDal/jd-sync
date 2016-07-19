#include "RequestWaiter.h"

#include <QEventLoop>

#include "Request.h"

RequestWaiter::RequestWaiter() {}

RequestWaiter::~RequestWaiter()
{
	reset();
}

void RequestWaiter::add(Request &request)
{
	m_pendingRequests.insert(&request);
	request.m_waiter = this;
}
void RequestWaiter::add(Request *request)
{
	m_pendingRequests.insert(request);
	request->m_waiter = this;
}

void RequestWaiter::wait()
{
	if (m_pendingRequests.size() > 0) {
		QEventLoop loop;
		m_loop = &loop;
		loop.exec();
		m_loop = nullptr;

		if (m_exception) {
			std::rethrow_exception(m_exception);
		}
	}
}

void RequestWaiter::reset()
{
	for (Request *request : m_pendingRequests) {
		request->m_waiter = nullptr;
	}
	m_pendingRequests.clear();
	if (m_loop) {
		m_loop->exit();
	}
}

void RequestWaiter::wait(Request *request)
{
	RequestWaiter waiter;
	waiter.add(request);
	waiter.wait();
}

void RequestWaiter::notifyDone(Request *request)
{
	m_pendingRequests.remove(request);
	request->m_waiter = nullptr;
	if (m_loop && m_pendingRequests.size() == 0) {
		m_loop->exit();
	}
}
void RequestWaiter::notifyException(std::exception_ptr exception)
{
	m_exception = exception;
	reset();
}
