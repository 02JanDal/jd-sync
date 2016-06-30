#pragma once

#include <QAbstractListModel>
#include "AbstractConsumer.h"

class MonitorModel : public QAbstractListModel, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit MonitorModel(ServerConnection *server, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override;

	QList<QString> m_messages;
};
