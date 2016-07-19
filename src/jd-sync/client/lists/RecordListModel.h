#pragma once

#include <QAbstractListModel>

class AbstractRecordList;

class RecordListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit RecordListModel(AbstractRecordList *list, QObject *parent = nullptr);

	enum
	{
		IdRole = Qt::UserRole,
		AllDataRole
	};

	AbstractRecordList *list() const { return m_list; }

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	void addMapping(const int column, const QString &property, const Qt::ItemDataRole role = Qt::DisplayRole, const bool editable = false);
	void addEditable(const int column, const QString &property);
	void setHeadings(const QVector<QString> &headings);

	QUuid idForIndex(const QModelIndex &index) const;
	QModelIndex indexForId(const QUuid &id) const;

private slots:
	void addedToList(const QUuid &id);
	void removedFromList(const QUuid &id);
	void changedInList(const QUuid &id, const QString &property);

private:
	AbstractRecordList *m_list;

	struct Mapping
	{
		QString property;
		bool editable;
	};
	QHash<QPair<int, Qt::ItemDataRole>, Mapping> m_mapping;
	QVector<QString> m_headings;
	QPair<int, Qt::ItemDataRole> reverseMapping(const QString &property, const bool editable) const;

	QVector<QUuid> m_ids;
	int m_cols = 0;
};
