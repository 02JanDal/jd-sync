#include "AbstractActor.h"
#include "Message.h"

#include <jd-util/Json.h>

class DummyActor : public AbstractActor
{
public:
	explicit DummyActor(MessageHub *hub) : AbstractActor(hub) {}

	using AbstractActor::subscribeTo;
	using AbstractActor::unsubscribeFrom;
	using AbstractActor::send;
	using AbstractActor::channels;
	using AbstractActor::request;

	QVector<Message> messages() const { return m_messages; }
	int resets() const { return m_resetCounter; }

	static void setMessageTo(Message &msg, AbstractActor *to) { msg.m_to = to; }
	static void setMessageFrom(Message &msg, AbstractActor *from) { msg.m_from = from; }

private:
	void receive(const Message &msg) override { m_messages.append(msg); }
	void reset() override { m_resetCounter++; }
	QVector<Message> m_messages;
	int m_resetCounter = 0;
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-function")
static std::ostream &operator<<(std::ostream &str, const QString &string) { return str << '"' << string.toStdString() << '"'; }
static std::ostream &operator<<(std::ostream &str, const QJsonObject &obj) { return str << Json::toText(obj).constData(); }
QT_WARNING_POP
