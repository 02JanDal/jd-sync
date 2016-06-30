#pragma once

#include "AbstractClientConnection.h"

#include <QMetaMethod>

class BaseSyncableList : public AbstractClientConnection
{
	Q_OBJECT
	Q_FLAGS(Flag Flags)
public:
	enum Flag
	{
		NoFlags = 0x00,
		AllowAdd = 0x01,
		AllowRemove = 0x02,
		AllowSet = 0x04,
		AllowExternalAdd = 0x10,
		AllowExternalRemove = 0x20,
		AllowExternalSet = 0x40,

		AllowOverwrite = 0x100,
		ListOnlyIndex = 0x200,
		AllFlags = AllowAdd | AllowRemove | AllowSet | AllowExternalAdd | AllowExternalRemove | AllowExternalSet | AllowOverwrite | ListOnlyIndex
	};
	Q_DECLARE_FLAGS(Flags, Flag)
	static constexpr Flags flags_noExternal() { return Flags({AllowAdd, AllowRemove, AllowSet}); }

	explicit BaseSyncableList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags = AllFlags, QObject *parent = nullptr);

	virtual void set(const int index, const QString &property, const QVariant &value, const QUuid &origin = QUuid()) = 0;
	void set(const QVariant &index, const QString &property, const QVariant &value, const QUuid &origin = QUuid());
	virtual QVariant get(const int index, const QString &property) const = 0;
	QVariant get(const QVariant &index, const QString &property) const;
	virtual QVariantMap getAll(const int index) const = 0;
	QVariantMap getAll(const QVariant &index) const;

	virtual int size() const = 0;
	bool contains(const QVariant &index) const { return findIndex(index) != -1; }
	int find(const QString &property, const QVariant &value);
	virtual QStringList keys() const = 0;

	virtual void add(const QMap<QString, QVariant> &values, const QUuid &origin = QUuid()) = 0;
	void add(const QVariant &index, const QMap<QString, QVariant> &values, const QUuid &origin = QUuid());
	virtual void remove(const int index, const QUuid &origin = QUuid()) = 0;
	void remove(const QVariant &index, const QUuid &origin = QUuid());

	virtual QVariant transformToList(const QString &property, const QVariant &value) const { return value; }
	virtual QVariant transformFromList(const QString &property, const QVariant &value) const { return value; }

	virtual int findIndex(const QVariant &index) const = 0;

signals:
	void changed(const int index, const QString &property);

protected:
	QString m_channel;
	QString m_cmdPrefix;
	QString m_indexProperty;
	Flags m_flags;

	virtual void toClient(const QJsonObject &obj) override;
	void ready() override;

	QString command(const QString &command) const;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(BaseSyncableList::Flags)

class SyncableList : public BaseSyncableList
{
	Q_OBJECT
public:
	explicit SyncableList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags = AllFlags, QObject *parent = nullptr);

	void set(const int index, const QString &property, const QVariant &value, const QUuid &origin) override;
	QVariant get(const int index, const QString &property) const override;
	QVariantMap getAll(const int index) const override;
	void add(const QMap<QString, QVariant> &values, const QUuid &origin = QUuid()) override;
	void remove(const int index, const QUuid &origin = QUuid()) override;
	int size() const override { return m_rows.size(); }
	int findIndex(const QVariant &index) const override;
	QStringList keys() const override;

private:
	QList<QMap<QString, QVariant>> m_rows;
};

class SyncableQObjectList : public BaseSyncableList
{
	Q_OBJECT
public:
	explicit SyncableQObjectList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags = AllFlags, QObject *parent = nullptr);

	void add(QObject *obj);
	void remove(QObject *obj);
	void set(const int index, const QString &property, const QVariant &value, const QUuid &origin) override;
	QVariant get(const int index, const QString &property) const override;
	QVariantMap getAll(const int index) const override;
	void remove(const int index, const QUuid &origin = QUuid()) override;
	int size() const override { return m_objects.size(); }
	template<typename T>
	T *get(const int index) const { return qobject_cast<T *>(m_objects.at(index)); }
	int findIndex(const QVariant &index) const override;
	QStringList keys() const override;

	void addPropertyMapping(const QString &objectProperty, const QString &externalProperty);
	void addPropertyMapping(const QString &wrappedObjectProperty, const QString &objectProperty, const QString &externalProperty);
	void addCommandMapping(const QString &cmd, const char *method, const QStringList &arguments = QStringList());

private slots:
	void propertyChanged();
	void wrappedPropertyChanged();

private:
	void toClient(const QJsonObject &obj) override;
	void add(const QMap<QString, QVariant> &values, const QUuid &origin) override;

	QVariantMap objToExt(QObject *obj) const;
	void extToObj(QObject *obj, const QVariantMap &values);
	QVariant indexValue(QObject *obj) const;
	QObject *wrappedObject(QObject *obj, const QString &extProp) const;
	QMetaMethod findOwnSlot(const char *slot) const;

	QList<QObject *> m_objects;
	QHash<QMetaMethod, QList<QString>> m_signalToProperty;
	QMap<QVariant, QObject *> m_mapping;
	QHash<QString, QString> m_objPropToExtProp;
	QHash<QString, QString> m_extPropToObjProp;
	QHash<QString, QPair<QString, QString>> m_wrappedPropMapping;
	QHash<QObject *, QObject *> m_wrappedToWrapper;
	QHash<QString, QPair<QString, QString>> m_extToWrappedMapping;
	QHash<QString, QPair<QByteArray, QStringList>> m_commandMapping;
};
