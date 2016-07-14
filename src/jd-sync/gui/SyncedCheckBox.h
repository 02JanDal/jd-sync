#pragma once

#include <QCheckBox>

class MessageHub;
class SyncedWidgetActor;

class SyncedCheckBox : public QCheckBox
{
	Q_OBJECT
public:
	explicit SyncedCheckBox(QWidget *parent = nullptr);

	void setup(MessageHub *hub, const QString &channel, const QString &property);
	void setId(const int id);

private:
	SyncedWidgetActor *m_actor = nullptr;
};
