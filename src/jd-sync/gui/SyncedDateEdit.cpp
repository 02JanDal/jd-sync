#include "SyncedDateEdit.h"

#include "SyncedWidgetActor.h"
#include <jd-util/Json.h>

SyncedDateEdit::SyncedDateEdit(QWidget *parent)
	: QDateEdit(parent) {}

void SyncedDateEdit::setup(MessageHub *hub, const QString &channel, const QString &property)
{
//	m_actor = new SyncedWidgetActor(hub, channel, property, this);
//	connect(m_actor, &SyncedWidgetActor::valueChanged, this, [this](const QJsonValue &val) { setDate(Json::ensureDate(val)); });
//	connect(this, &SyncedDateEdit::editingFinished, m_actor, [this]() { m_actor->setValue(Json::toJson(date())); });

}
void SyncedDateEdit::setId(const int id)
{
//	m_actor->setId(id);
}

