#pragma once

#include "AbstractRecordList.h"

#include <QSet>

class ChangeTrackingList : public AbstractRecordList
{
	Q_OBJECT
	Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
public:
	explicit ChangeTrackingList(AbstractRecordList *parentList, QObject *parent = nullptr);

	bool isModified() const { return m_modified; }

	QVariantHash get(const QUuid &id) const override;
	void set(const QVector<QVariantHash> &properties) override;
	void add(const QVector<QVariantHash> &properties) override;
	void remove(const QSet<QUuid> &ids) override;
	ABSTRACTRECORDLIST_USING_COMFORT_OVERLOADS

	bool contains(const QUuid &id) const override;
	QList<QUuid> ids() const override;

	bool isAddition(const QUuid &id) const;

signals:
	void modifiedChanged(const bool modified);

public slots:
	void commit();
	void discard();

private slots:
	void addedInParent(const QUuid &id);
	void removedInParent(const QUuid &id);
	void changedInParent(const QUuid &id, const QString &property);

private:
	bool m_modified = false;
	void updateModified();

	bool m_committing = false;

	QSet<QUuid> m_removals;
	QHash<QUuid, QVariantHash> m_additions;
	QHash<QUuid, QVariantHash> m_changes;
};
