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

class SyncedList : public QObject, public AbstractActor
{
	Q_OBJECT
	INTROSPECTION
public:
	explicit SyncedList(MessageHub *hub, const QString &channel, const Table &table, QObject *parent = nullptr);

	QVariant get(const QUuid &id, const QString &property) const;
	QVariantHash getAll(const QUuid &id) const;
	void set(const QUuid &id, const QString &property, const QVariant &value);
	void set(const QUuid &id, const QVariantHash &properties);
	void set(const QVector<QVariantHash> &properties);

	void add(const QVariantHash &values);
	void add(const QVector<QVariantHash> &values);
	void remove(const QUuid &id);
	void remove(const QSet<QUuid> &ids);

	bool contains(const QUuid &id) const;
	QList<QUuid> indexes() const;

	void refetch();
	void fetchMore();
	void refetch(const QUuid &id);
	void fetchOnce(const Filter &filter);

	void setFocus(const Filter &filter);

	Table table() const { return m_table; }

signals:
	void added(const QUuid &index);
	void removed(const QUuid &index);
	void changed(const QUuid &index, const QString &property);

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

class SyncedListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit SyncedListModel(MessageHub *hub, const QString &channel, const Table &table, QObject *parent = nullptr);
	explicit SyncedListModel(SyncedList *list, QObject *parent = nullptr);

	void focus();
	void setFilter(const Filter &filter);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QUuid &id, const QString &property) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	bool setData(const QUuid &id, const QString &property, const QVariant &value);
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	void addMapping(const int column, const QString &property, const Qt::ItemDataRole role = Qt::DisplayRole, const bool editable = false);
	void addEditable(const int column, const QString &property);
	void setHeadings(const QVector<QString> &headings);

	QUuid idForIndex(const QModelIndex &index) const;
	QModelIndex indexForId(const QUuid &id) const;

	QVariantHash getAll(const QUuid &id) const;

	QUuid addPreliminary(const QVariantHash &properties);
	void removePreliminary(const QUuid &id);
	void commitPreliminaries();
	void discardPreliminaries();
	bool isModified() const;
	bool isPreliminaryAddition(const QUuid &uuid) const;

private slots:
	void addedToList(const QUuid &id);
	void removedFromList(const QUuid &id);
	void changedInList(const QUuid &id, const QString &property);

signals:
	void modifiedChanged(const bool modified);
	void added(const QUuid &id);
	void removed(const QUuid &id);
	void changed(const QUuid &id, const QString &property);

private:
	SyncedList *m_list;
	Filter m_filter;

	struct Mapping
	{
		QString property;
		bool editable;
	};
	QHash<QPair<int, Qt::ItemDataRole>, Mapping> m_mapping;
	QVector<QString> m_headings;
	QPair<int, Qt::ItemDataRole> reverseMapping(const QString &property, const bool editable) const;

	QSet<QUuid> m_preliminaryRemovals;
	QHash<QUuid, QVariantHash> m_preliminaryAdditions;
	QHash<QUuid, QVariantHash> m_preliminaryChanges;

	QVector<QUuid> m_indices;
	int m_cols = 0;
};
