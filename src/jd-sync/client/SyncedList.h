#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QPair>
#include <QVariant>
#include <QDateTime>

#include "jd-sync/common/AbstractActor.h"
#include "jd-sync/common/Filter.h"

class SyncedList : public QObject, public AbstractActor
{
	Q_OBJECT
public:
	INTROSPECTION

	explicit SyncedList(MessageHub *hub, const QString &channel, const QSet<QString> &properties, QObject *parent = nullptr);

	QVariant get(const int id, const QString &property) const;
	QVariantHash getAll(const int id) const;
	void set(const int id, const QString &property, const QVariant &value);
	void set(const int id, const QVariantHash &properties);
	void set(const QVector<QVariantHash> &properties);

	void add(const QVariantHash &values);
	void add(const QVector<QVariantHash> &values);
	void remove(const int id);
	void remove(const QSet<int> &ids);

	bool contains(const int id) const;
	QList<int> indexes() const;

	void refetch();
	void fetchMore();
	void refetch(const int id);
	void fetchOnce(const Filter &filter);

	void setFocus(const Filter &filter);

	QSet<QString> properties() const { return m_properties; }

signals:
	void added(const int index);
	void removed(const int index);
	void changed(const int index, const QString &property);

private:
	void receive(const Message &msg) override;
	void reset() override;

	QString m_channel;
	QHash<int, QVariantHash> m_rows;
	QSet<QString> m_properties;

	Filter m_focusFilter;
	int m_lastUpdated = -1;

	void addOrUpdate(const QJsonObject &record);
};

class SyncedListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit SyncedListModel(MessageHub *hub, const QString &channel, const QSet<QString> &properties, QObject *parent = nullptr);
	explicit SyncedListModel(SyncedList *list, QObject *parent = nullptr);

	void focus();
	void setFilter(const Filter &filter);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant data(const int id, const QString &property) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	bool setData(const int id, const QString &property, const QVariant &value);
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	void addMapping(const int column, const QString &property, const Qt::ItemDataRole role = Qt::DisplayRole, const bool editable = false);
	void addEditable(const int column, const QString &property);

	int idForIndex(const QModelIndex &index) const;

	int addPreliminary(const QVariantHash &properties);
	void removePreliminary(const int id);
	void commitPreliminaries();
	void discardPreliminaries();
	bool isModified() const;

private slots:
	void addedToList(const int id);
	void removedFromList(const int id);
	void changedInList(const int id, const QString &property);

signals:
	void modifiedChanged(const bool modified);
	void added(const int id);
	void removed(const int id);
	void changed(const int id, const QString &property);

private:
	SyncedList *m_list;
	Filter m_filter;

	struct Mapping
	{
		QString property;
		bool editable;
	};
	QHash<QPair<int, Qt::ItemDataRole>, Mapping> m_mapping;
	QPair<int, Qt::ItemDataRole> reverseMapping(const QString &property, const bool editable) const;

	QSet<int> m_preliminaryRemovals;
	QHash<int, QVariantHash> m_preliminaryAdditions;
	QHash<int, QVariantHash> m_preliminaryChanges;

	QVector<int> m_indices;
	int m_cols = 0;
};
