//#pragma once

//#include <QObject>
//#include <QJsonValue>
//#include "client/SyncedList.h"

//class SyncedWidgetActor : public QObject
//{
//	Q_OBJECT
//	Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
//	Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
//public:
//	explicit SyncedWidgetActor(SyncedList *list, const QString &property, QObject *parent = nullptr);

//	QVariant value() const { return m_value; }
//	bool isModified() const { return !m_stagedValue.isValid(); }

//public slots:
//	void setValue(const QVariant &value);
//	void setId(const int id);
//	void commit();

//signals:
//	void valueChanged(const QVariant &value);
//	void modifiedChanged(const bool modified);

//private:
//	SyncedList *m_list;

//	QString m_channel;
//	QString m_property;
//	int m_id = -1;
//	QVariant m_value;

//	QVariant m_stagedValue;
//};
