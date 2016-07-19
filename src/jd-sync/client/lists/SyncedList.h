#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QPair>
#include <QVariant>
#include <QUuid>

#include <jd-db/schema/Table.h>

#include "jd-sync/common/AbstractActor.h"
#include "jd-sync/common/Filter.h"

#include "AbstractRecordList.h"

class SyncedList : public AbstractRecordList, public AbstractActor
{
	Q_OBJECT
	INTROSPECTION
public:
	explicit SyncedList(MessageHub *hub, const QString &channel, const Table &table, QObject *parent = nullptr);

	QVariantHash get(const QUuid &id) const override;
	void set(const QVector<QVariantHash> &properties) override;
	void add(const QVector<QVariantHash> &values) override;
	void remove(const QSet<QUuid> &ids) override;
	ABSTRACTRECORDLIST_USING_COMFORT_OVERLOADS

	bool contains(const QUuid &id) const override;
	QList<QUuid> ids() const override;

	void refetch();
	void fetchMore();
	void refetch(const QUuid &id);
	void fetchOnce(const Filter &filter);

	void setFocus(const Filter &filter);

	Table table() const { return m_table; }

private:
	void receive(const Message &msg) override;
	void reset() override;

	QString m_channel;
	QHash<QUuid, QVariantHash> m_rows;
	Table m_table;

	Filter m_focusFilter;
	int m_lastUpdated = -1;

	void addOrUpdate(const QJsonObject &record);
};
