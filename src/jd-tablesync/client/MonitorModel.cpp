#include "MonitorModel.h"

#include "jd-util/Json.h"

MonitorModel::MonitorModel(ServerConnection *server, QObject *parent)
	: QAbstractListModel(parent), AbstractConsumer(server)
{
	subscribeTo("*");
}

int MonitorModel::rowCount(const QModelIndex &parent) const
{
	return m_messages.size();
}
QVariant MonitorModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0)
	{
		return m_messages.at(index.row());
	}
	else
	{
		return QVariant();
	}
}

void MonitorModel::consume(const QString &channel, const QString &cmd, const QJsonObject &data)
{
	beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
	m_messages.append(Json::toText(data));
	endInsertRows();
}
