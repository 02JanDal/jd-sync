#include "SyncedLineEdit.h"

#include "SyncedWidgetActor.h"
#include <jd-util/Json.h>

SyncedLineEdit::SyncedLineEdit(QWidget *parent)
	: QLineEdit(parent) {}

void SyncedLineEdit::setup(MessageHub *hub, const QString &channel, const QString &property)
{
//	m_actor = new SyncedWidgetActor(hub, channel, property, this);
//	connect(m_actor, &SyncedWidgetActor::valueChanged, this, [this](const QJsonValue &val) { setText(Json::ensureString(val)); });
//	connect(this, &SyncedLineEdit::editingFinished, m_actor, [this]() { m_actor->setValue(text()); });

}
void SyncedLineEdit::setId(const int id)
{
//	m_actor->setId(id);
}

