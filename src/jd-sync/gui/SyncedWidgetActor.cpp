//#include "SyncedWidgetActor.h"

//#include "Message.h"
//#include <jd-util/Json.h>

//SyncedWidgetActor::SyncedWidgetActor(SyncedList *list, const QString &property, QObject *parent)
//	: QObject(parent), m_list(list), m_property(property) {}

//void SyncedWidgetActor::setValue(const QVariant &value)
//{
//	m_stagedValue = value;
//}

//void SyncedWidgetActor::setId(const int id)
//{
//	if (m_id == id) {
//		return;
//	}
//	m_id = id;
//	m_stagedValue = QJsonValue::Undefined;
//	m_list->fetchOnce(Filter(QVector<FilterPart>() << FilterPart("id", FilterPart::Equal, m_id), QVector<FilterGroup>()));
//}

//void SyncedWidgetActor::commit()
//{
//	m_stagedValue = QJsonValue::Undefined;
//	send(Message(m_channel, "update", QJsonObject({
//													  {"id", m_id},
//													  {m_property, Json::toJson(m_stagedValue)}
//												  })));
//}

//void SyncedWidgetActor::receive(const Message &message)
//{
//	if (m_id == -1) {
//		return;
//	}

//	if (message.channel() == m_channel) {
//		if (message.command() == "read:result") {
//			if (Json::ensureInteger(message.dataObject(), "id") == m_id && message.dataObject().contains(m_property)) {
//				m_value = message.dataObject().value(m_property);
//				emit valueChanged(m_value);
//			}
//		} else if (message.command() == "update:result") {
//			for (const QJsonObject &obj : Json::ensureIsArrayOf<QJsonObject>(message.dataArray())) {
//				if (Json::ensureInteger(obj, "id") == m_id) {
//					m_value = obj.value(m_property);
//					emit valueChanged(m_value);
//					break;
//				}
//			}
//		}
//	}
//}

//void SyncedWidgetActor::reset()
//{
//	if (m_id != -1) {
//		send(Message(m_channel, "read", QJsonObject({
//														{"id", Json::toJson(m_id)},
//														{"properties", QJsonArray() << m_property}
//													})));
//		if (!m_pendingSet.isUndefined()) {
//			send(Message(m_channel, "update", QJsonObject({
//															  {"id", m_id},
//															  {m_property, m_pendingSet}
//														  })));
//		}
//	}
//}
