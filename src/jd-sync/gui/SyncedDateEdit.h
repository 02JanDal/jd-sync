#pragma once

#include <QDateEdit>

class MessageHub;
class SyncedWidgetActor;

class SyncedDateEdit : public QDateEdit
{
	Q_OBJECT
public:
	explicit SyncedDateEdit(QWidget *parent = nullptr);

	void setup(MessageHub *hub, const QString &channel, const QString &property);
	void setId(const int id);

private:
	SyncedWidgetActor *m_actor = nullptr;
};
