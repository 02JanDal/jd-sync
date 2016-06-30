#pragma once

class QTcpSocket;
class QByteArray;

namespace TcpUtils
{
void writePacket(QTcpSocket *socket, const QByteArray &data);
QByteArray readPacket(QTcpSocket *socket);
}
