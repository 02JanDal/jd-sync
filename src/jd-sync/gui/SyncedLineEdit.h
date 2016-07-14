#pragma once

#include <QLineEdit>

class MessageHub;
class SyncedWidgetActor;

class SyncedLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit SyncedLineEdit(QWidget *parent = nullptr);

	void setup(MessageHub *hub, const QString &channel, const QString &property);
	void setId(const int id);

private:
	SyncedWidgetActor *m_actor = nullptr;
};
