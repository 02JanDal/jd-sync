#pragma once

#include <QDateTimeEdit>

class MessageHub;
class SyncedWidgetActor;

class SyncedDateTimeEdit : public QDateTimeEdit
{
	Q_OBJECT
public:
	explicit SyncedDateTimeEdit(QWidget *parent = nullptr);

	void setup(MessageHub *hub, const QString &channel, const QString &property);
	void setId(const int id);

private:
	SyncedWidgetActor *m_actor = nullptr;
};
