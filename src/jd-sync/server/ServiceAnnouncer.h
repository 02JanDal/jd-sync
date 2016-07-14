#pragma once

#include <QObject>
#include <QHash>

class ServiceAnnouncer : public QObject
{
	Q_OBJECT
public:
	explicit ServiceAnnouncer(QObject *parent = nullptr);
	~ServiceAnnouncer();

	void registerService(const QString &name, const quint16 port, const QString &type, const QHash<QString, QString> &txt = {});
	void unregisterService(const QString &name);

signals:
	void error(const QString &error);

private:
	class ServiceAnnouncerPrivate *d;
};
