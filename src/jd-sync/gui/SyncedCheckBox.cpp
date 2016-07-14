#include "SyncedCheckBox.h"

#include "SyncedWidgetActor.h"
#include <jd-util/Json.h>

SyncedCheckBox::SyncedCheckBox(QWidget *parent)
	: QCheckBox(parent) {}

void SyncedCheckBox::setup(MessageHub *hub, const QString &channel, const QString &property)
{
	//m_actor = new SyncedWidgetActor(hub, channel, property, this);
//	connect(m_actor, &SyncedWidgetActor::valueChanged, this, [this](const QJsonValue &val) { setChecked(Json::ensureBoolean(val)); });
//	connect(this, &SyncedCheckBox::clicked, m_actor, [this](const bool checked) { m_actor->setValue(checked); });
}
void SyncedCheckBox::setId(const int id)
{
//	m_actor->setId(id);
}

