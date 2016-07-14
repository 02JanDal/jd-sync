#include "SyncedDateTimeEdit.h"

#include "SyncedWidgetActor.h"
#include <jd-util/Json.h>

SyncedDateTimeEdit::SyncedDateTimeEdit(QWidget *parent)
	: QDateTimeEdit(parent) {}

void SyncedDateTimeEdit::setup(MessageHub *hub, const QString &channel, const QString &property)
{
//	m_actor = new SyncedWidgetActor(hub, channel, property, this);
//	connect(m_actor, &SyncedWidgetActor::valueChanged, this, [this](const QJsonValue &val) { setDateTime(Json::ensureDateTime(val)); });
//	connect(this, &SyncedDateTimeEdit::editingFinished, m_actor, [this]() { m_actor->setValue(Json::toJson(dateTime())); });

}
void SyncedDateTimeEdit::setId(const int id)
{
//	m_actor->setId(id);
}

