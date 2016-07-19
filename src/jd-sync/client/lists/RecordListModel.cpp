#include "RecordListModel.h"

#include <QUuid>

#include "AbstractRecordList.h"

RecordListModel::RecordListModel(AbstractRecordList *list, QObject *parent)
	: QAbstractListModel(parent), m_list(list)
{
	connect(m_list, &AbstractRecordList::added, this, &RecordListModel::addedToList);
	connect(m_list, &AbstractRecordList::removed, this, &RecordListModel::removedFromList);
	connect(m_list, &AbstractRecordList::changed, this, &RecordListModel::changedInList);

	m_ids = m_list->ids().toVector();
}

int RecordListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	return m_ids.size();
}
int RecordListModel::columnCount(const QModelIndex &parent) const
{
	return m_cols;
}
QVariant RecordListModel::data(const QModelIndex &index, int role) const
{
	if (role == IdRole) {
		return idForIndex(index);
	} else if (role == AllDataRole) {
		return m_list->get(idForIndex(index));
	} else if (m_mapping.contains(qMakePair(index.column(), Qt::ItemDataRole(role)))) {
		const QString property = m_mapping[qMakePair(index.column(), Qt::ItemDataRole(role))].property;
		const QUuid id = idForIndex(index);
		return m_list->get(id, property);
	} else {
		return QVariant();
	}
}
QVariant RecordListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < m_headings.size()) {
		return m_headings.at(section);
	} else {
		return QVariant();
	}
}
bool RecordListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!m_mapping.contains(qMakePair(index.column(), Qt::ItemDataRole(role)))) {
		return false;
	}
	const Mapping mapping = m_mapping[qMakePair(index.column(), Qt::ItemDataRole(role))];
	if (!mapping.editable) {
		return false;
	}

	const QString property = mapping.property;
	const QUuid id = idForIndex(index);
	if (id.isNull()) {
		return false;
	}

	m_list->set(id, property, value);
	return true;
}
Qt::ItemFlags RecordListModel::flags(const QModelIndex &index) const
{
	const QPair<int, Qt::ItemDataRole> mappingKey = qMakePair(index.column(), Qt::ItemDataRole(Qt::EditRole));
	if (m_mapping.contains(mappingKey) && m_mapping[mappingKey].editable) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	} else {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

void RecordListModel::addMapping(const int column, const QString &property, const Qt::ItemDataRole role, const bool editable)
{
	beginResetModel();
	m_cols = qMax(m_cols, column + 1);
	m_mapping.insert(qMakePair(column, Qt::ItemDataRole(role)), Mapping{property, editable});
	endResetModel();
}
void RecordListModel::addEditable(const int column, const QString &property)
{
	addMapping(column, property, Qt::DisplayRole);
	addMapping(column, property, Qt::EditRole, true);
}
void RecordListModel::setHeadings(const QVector<QString> &headings)
{
	m_headings = headings;
}

void RecordListModel::addedToList(const QUuid &id)
{
	Q_ASSERT(!m_ids.contains(id));
	beginInsertRows(QModelIndex(), m_ids.size(), m_ids.size());
	m_ids.append(id);
	endInsertRows();
}
void RecordListModel::removedFromList(const QUuid &id)
{
	const int row = m_ids.indexOf(id);
	if (row != -1) {
		beginRemoveRows(QModelIndex(), row, row);
		m_ids.removeAt(row);
		endRemoveRows();
	}
}
void RecordListModel::changedInList(const QUuid &id, const QString &property)
{
	const int row = m_ids.indexOf(id);

	for (auto it = m_mapping.constBegin(); it != m_mapping.constEnd(); ++it) {
		if (it.value().property == property) {
			emit dataChanged(index(row, it.key().first), index(row, it.key().first), QVector<int>() << it.key().second);
			break;
		}
	}
}

QPair<int, Qt::ItemDataRole> RecordListModel::reverseMapping(const QString &property, const bool editable) const
{
	for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
		if (it.value().property == property && (it.value().editable == editable || !editable)) {
			return it.key();
		}
	}
	return {};
}

QUuid RecordListModel::idForIndex(const QModelIndex &index) const
{
	if (index.row() < 0 || index.row() >= m_ids.size()) {
		return QUuid();
	}
	return m_ids.at(index.row());
}
QModelIndex RecordListModel::indexForId(const QUuid &id) const
{
	const int row = m_ids.indexOf(id);
	if (row == -1) {
		return QModelIndex();
	} else {
		return index(m_ids.indexOf(id));
	}
}
