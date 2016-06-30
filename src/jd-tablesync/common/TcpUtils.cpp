#include "TcpUtils.h"

#include <QTcpSocket>
#include <QDataStream>

void TcpUtils::writePacket(QTcpSocket *socket, const QByteArray &data)
{
	QDataStream str(socket);
	str.setByteOrder(QDataStream::LittleEndian);
	str << (quint32) data.size();
	socket->write(data);
}
QByteArray TcpUtils::readPacket(QTcpSocket *socket)
{
	while (socket->bytesAvailable() < sizeof(quint32))
	{
		socket->waitForReadyRead(500);
	}

	QDataStream str(socket);
	str.setByteOrder(QDataStream::LittleEndian);
	quint32 size;
	str >> size;

	while (socket->bytesAvailable() < size)
	{
		socket->waitForReadyRead(500);
	}

	return socket->read(size);
}
