#include "TcpClientActor.h"

#include <QTcpSocket>

#include <jd-util/Json.h>
#include <jd-util/Logging.h>

#include "Message.h"
#include "common/TcpUtils.h"

static Q_LOGGING_CATEGORY(Tcp, "tablesync.tcp.client")

TcpClientActor::TcpClientActor(MessageHub *hub, const QString &host, const quint16 port, QObject *parent)
	: AbstractExternalActor(hub, parent), m_host(host), m_port(port)
{
	subscribeTo("*");
}

void TcpClientActor::setAuthentication(const QString &data)
{
	Q_ASSERT_X(QThread::currentThread() == thread(), "TcpClientActor::setAuthentication", "You need to call this from the right thread (use queued signals/slots)");
	m_authentication = data;
	m_needAuthentication = true;

	if (m_socket && m_socket->isOpen()) {
		sendToExternal(Message("client.auth", "attempt", QJsonObject({{"token", m_authentication}})).setFlags(Message::BypassAuth));
	}
}

void TcpClientActor::connectToHost()
{
	Q_ASSERT_X(QThread::currentThread() == thread(), "TcpClientActor::setAuthentication", "You need to call this from the right thread (use queued signals/slots)");
	Q_ASSERT(!m_socket->isOpen() && m_socket->state() != QTcpSocket::ConnectingState);
	m_socket->connectToHost(m_host, m_port);
	m_state = Connecting;
}
void TcpClientActor::disconnectFromHost()
{
	Q_ASSERT_X(QThread::currentThread() == thread(), "TcpClientActor::setAuthentication", "You need to call this from the right thread (use queued signals/slots)");
	Q_ASSERT(m_socket->isOpen());
	m_socket->disconnectFromHost();
}

void TcpClientActor::sendToExternal(const Message &message)
{
	// we don't send errors to the server
	if (message.command() == "error") {
		return;
	}

	//qCDebug(Tcp) << "sending" << message.toJson();
	const QByteArray raw = Json::toBinary(message.toJson());
	if (m_needAuthentication && !message.isBypassingAuth()) {
		m_messagesQueue.enqueue(raw);
	} else if (m_socket->state() == QTcpSocket::ConnectedState) {
		TcpUtils::writePacket(m_socket, raw);
	} else if (message.isBypassingAuth()) {
		m_noAuthMessageQueue.enqueue(raw);
	} else {
		m_messagesQueue.enqueue(raw);
	}
}

void TcpClientActor::run()
{
	m_socket = new QTcpSocket(this);
	connectSocket();

	AbstractExternalActor::run();
}

void TcpClientActor::socketChangedState()
{
	switch (m_socket->state()) {
	case QAbstractSocket::UnconnectedState:
		if (m_state == Connected) {
			m_state = Reconnecting;
			emit message(tr("Lost connection to host"));
			connectToHost();
		}
		break;
	case QAbstractSocket::HostLookupState:
		emit message(tr("Looking up host..."));
		break;
	case QAbstractSocket::ConnectingState:
		emit message(tr("Connecting to host..."));
		break;
	case QAbstractSocket::ConnectedState: {
		sendQueue(&m_noAuthMessageQueue);
		if (m_needAuthentication) {
			emit message(tr("Authenticating..."));
			sendToExternal(Message("client.auth", "attempt", QJsonObject({{"token", m_authentication}})).setFlags(Message::BypassAuth));
		} else {
			emit message(tr("Connected!"));
			qCDebug(Tcp) << "Connection and authentication successfull";
			if (m_state == Connecting) {
				m_state = Connected;
				emit connected();
			}
			sendQueue(&m_messagesQueue);
		}
		break;
	}
	case QAbstractSocket::BoundState:
		break;
	case QAbstractSocket::ListeningState:
		break;
	case QAbstractSocket::ClosingState:
		break;
	}
}
void TcpClientActor::socketError()
{
	emit message(tr("Socket error: %1").arg(m_socket->errorString()));

	// initial connection
	if (m_state == Connecting) {
		emit error(tr("Socket error: %1").arg(m_socket->errorString()));
	}
}
void TcpClientActor::socketDataReady()
{
	using namespace Json;

	while (m_socket->bytesAvailable() > 0) {
		Message msg;
		try {
			const QJsonObject obj = ensureObject(ensureDocument(TcpUtils::readPacket(m_socket)));
			msg = Message::fromJson(obj);

			if (msg.channel() == "client.auth") {
				if (msg.command() == "challenge") {
					emit authenticationRequired();
				} else if (msg.command() == "response") {
					if (ensureBoolean(ensureObject(msg.data()), QStringLiteral("success"))) {
						m_needAuthentication = false;

						emit message(tr("Connected!"));
						qCDebug(Tcp) << "Connection and authentication successfull";
						m_state = Connected;
						emit connected();
						sendQueue(&m_messagesQueue);
						send(Message("client", "reset"));
					} else {
						emit authenticationRequired();
					}
				}
			} else {
				receivedFromExternal(msg);
			}
		} catch (Exception &e) {
			qCWarning(Tcp) << e.cause();
		}
	}
}

void TcpClientActor::connectSocket()
{
	connect(m_socket, &QTcpSocket::stateChanged, this, &TcpClientActor::socketChangedState);
	connect(m_socket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &TcpClientActor::socketError);
	connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientActor::socketDataReady);
}

void TcpClientActor::sendQueue(QQueue<QByteArray> *queue)
{
	while (m_socket->isOpen() && m_socket->isWritable() && !queue->isEmpty()) {
		TcpUtils::writePacket(m_socket, queue->dequeue());
	}
}
