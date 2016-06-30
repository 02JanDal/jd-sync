#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QMap>
#include <QHash>
#include <QPair>
#include <QVariant>
#include "AbstractConsumer.h"

class SyncedList : public QObject, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit SyncedList(ServerConnection *server, const QString &channel, const QString &cmdPrefix, const QString &indexProperty, QObject *parent = nullptr);

	QVariant get(const QVariant &index, const QString &property) const;
	QVariantMap getAll(const QVariant &index) const;
	void set(const QVariant &index, const QString &property, const QVariant &value);

	void add(const QVariant &index, const QVariantMap &values);
	void remove(const QVariant &index);

	bool contains(const QVariant &index) const;
	QList<QVariant> indices() const;

	void refetch();
	void refetch(const QVariant &index);

signals:
	void added(const QVariant &index);
	void removed(const QVariant &index);
	void changed(const QVariant &index, const QString &property);

private:
	void consume(const QString &channel, const QString &cmd, const QJsonObject &data) override;
	QString m_channel;
	QString m_cmdPrefix;
	QString m_indexProperty;
	QMap<QVariant, QVariantMap> m_rows;

	void addOrUpdate(const QVariantMap &map);

	QString command(const QString &cmd) const;
	static QVariantMap cleanMap(const QVariantMap &map);
};

class SyncedListModel : public QAbstractListModel, public AbstractConsumer
{
	Q_OBJECT
public:
	explicit SyncedListModel(ServerConnection *server, const QString &channel, const QString &cmdPrefix, const QString &indexProperty, QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	void addMapping(int column, const QString &property, int role = Qt::DisplayRole, const bool editable = false);
	void addEditable(int column, const QString &property);

private slots:
	void added(const QVariant &id);
	void removed(const QVariant &id);
	void changed(const QVariant &id, const QString &property);

protected:
	using AbstractConsumer::emitMsg;
	SyncedList *m_list;
	QHash<QPair<int, int>, QPair<QString, bool>> m_mapping;
	QList<QVariant> m_indices;
	int m_cols = 0;
};
