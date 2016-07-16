#pragma once

#include <QAbstractListModel>
#include "jd-sync/common/AbstractActor.h"

class MonitorModel : public QAbstractListModel, public AbstractActor
{
	Q_OBJECT
	INTROSPECTION
public:
	explicit MonitorModel(MessageHub *hub, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

private:
	void receive(const Message &msg) override;

	QList<QString> m_messages;
};
