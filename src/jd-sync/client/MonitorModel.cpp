#include "MonitorModel.h"

#include <jd-util/Json.h>

#include "common/Message.h"

MonitorModel::MonitorModel(MessageHub *hub, QObject *parent)
	: QAbstractListModel(parent), AbstractActor(hub)
{
	subscribeTo("*");
}

int MonitorModel::rowCount(const QModelIndex &parent) const
{
	return m_messages.size();
}
QVariant MonitorModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0) {
		return m_messages.at(index.row());
	} else {
		return QVariant();
	}
}

void MonitorModel::receive(const Message &msg)
{
	beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
	m_messages.append(Json::toText(msg.toJson()));
	endInsertRows();
}
