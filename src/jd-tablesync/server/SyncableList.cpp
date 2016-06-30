#include "SyncableList.h"

#include <QMetaMethod>

#include "jd-util/Json.h"

inline static QJsonValue toJson(const QVariant &v)
{
	return QJsonValue::fromVariant(v);
}
inline static uint qHash(const QMetaMethod &method)
{
	return qHash(method.methodIndex());
}

BaseSyncableList::BaseSyncableList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags, QObject *parent)
	: AbstractClientConnection(parent), m_channel(channel), m_cmdPrefix(cmdPrefix), m_indexProperty(indexProperty), m_flags(flags)
{
	subscribeTo(channel);
}

void BaseSyncableList::set(const QVariant &index, const QString &property, const QVariant &value, const QUuid &origin)
{
	set(findIndex(index), property, value, origin);
}
QVariant BaseSyncableList::get(const QVariant &index, const QString &property) const
{
	return get(findIndex(index), property);
}
QVariantMap BaseSyncableList::getAll(const QVariant &index) const
{
	return getAll(findIndex(index));
}
int BaseSyncableList::find(const QString &property, const QVariant &value)
{
	for (int i = 0; i < size(); ++i)
	{
		if (get(i, property) == value)
		{
			return i;
		}
	}
	return -1;
}
void BaseSyncableList::add(const QVariant &index, const QMap<QString, QVariant> &values, const QUuid &origin)
{
	QMap<QString, QVariant> v;
	v.insert(m_indexProperty, index);
	add(v, origin);
}
void BaseSyncableList::remove(const QVariant &index, const QUuid &origin)
{
	remove(findIndex(index), origin);
}
void BaseSyncableList::toClient(const QJsonObject &obj)
{
	using namespace Json;
	const QString channel = ensureString(obj, "channel");
	const QString cmd = ensureString(obj, "cmd");
	const QUuid msgId = ensureUuid(obj, "msgId");

	if (channel == m_channel)
	{
		if (cmd == command("add") && m_flags.testFlag(AllowExternalAdd))
		{
			QVariantMap data = obj.toVariantMap();
			data.remove("channel");
			data.remove("cmd");
			data.remove("msgId");
			add(data, msgId);
		}
		else if (cmd == command("remove") && m_flags.testFlag(AllowExternalRemove))
		{
			remove(ensureVariant(obj, m_indexProperty), msgId);
		}
		else if (cmd == command("set") && m_flags.testFlag(AllowExternalSet))
		{
			for (const QString &key : obj.keys())
			{
				if (key == "channel" || key == "cmd" || key == "msgId")
				{
					continue;
				}
				set(ensureVariant(obj, m_indexProperty), key, obj.value(key), msgId);
			}
		}
		else if (cmd == command("get"))
		{
			const QVariant index = ensureVariant(obj, m_indexProperty);
			emit broadcast(channel, command("item"), QJsonObject::fromVariantMap(getAll(index)), msgId);
		}
		else if (cmd == command("list"))
		{
			QJsonArray array;
			for (int i = 0; i < size(); ++i)
			{
				if (m_flags.testFlag(ListOnlyIndex))
				{
					array.append(toJson(get(i, m_indexProperty)));
				}
				else
				{
					array.append(QJsonObject::fromVariantMap(getAll(i)));
				}
			}
			emit broadcast(channel, command("items"), {{"items", array}}, msgId);
		}
	}
}
void BaseSyncableList::ready()
{
	toClient({{"channel", m_channel},
			  {"cmd", command("list")},
			  {"msgId", toJson(QUuid())}});
}

QString BaseSyncableList::command(const QString &cmd) const
{
	if (m_cmdPrefix.isEmpty())
	{
		return cmd;
	}
	else
	{
		return m_cmdPrefix + ':' + cmd;
	}
}

SyncableList::SyncableList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags, QObject *parent)
	:BaseSyncableList(channel, cmdPrefix, indexProperty, flags, parent)
{
}

void SyncableList::set(const int index, const QString &property, const QVariant &value, const QUuid &origin)
{
	if (!m_flags.testFlag(AllowSet))
	{
		return;
	}
	Q_ASSERT(index >= 0 && index < m_rows.size());
	m_rows[index].insert(property, value);
	emit broadcast(m_channel, command("changed"), {{m_indexProperty, toJson(m_rows.at(index).value(m_indexProperty))},
												  {property, toJson(value)}}, origin);
	emit changed(index, property);
}
QVariant SyncableList::get(const int index, const QString &property) const
{
	Q_ASSERT(index >= 0 && index < m_rows.size());
	return m_rows.at(index).value(property);
}
QVariantMap SyncableList::getAll(const int index) const
{
	Q_ASSERT(index >= 0 && index < m_rows.size());
	return m_rows.at(index);
}
void SyncableList::add(const QMap<QString, QVariant> &values, const QUuid &origin)
{
	if (!m_flags.testFlag(AllowAdd))
	{
		return;
	}
	Q_ASSERT(values.contains(m_indexProperty));
	const int existingRow = findIndex(values.value(m_indexProperty));
	if (existingRow >= 0)
	{
		if (m_flags.testFlag(AllowOverwrite))
		{
			remove(existingRow, origin);
		}
		else if (!origin.isNull())
		{
			emit broadcast(m_channel, command("error"), {{"error", "Unable to overwrite " + values.value(m_indexProperty).toString()}}, origin);
			return;
		}
		else
		{
			return;
		}
	}
	m_rows.append(values);
	emit broadcast(m_channel, command("added"), QJsonObject::fromVariantMap(values), origin);
}
void SyncableList::remove(const int index, const QUuid &origin)
{
	if (!m_flags.testFlag(AllowRemove))
	{
		return;
	}
	const QMap<QString, QVariant> values = m_rows.takeAt(index);
	emit broadcast(m_channel, command("removed"), {{m_indexProperty, toJson(values.value(m_indexProperty))}}, origin);
}
int SyncableList::findIndex(const QVariant &index) const
{
	for (int i = 0; i < m_rows.size(); ++i)
	{
		if (m_rows.at(i).value(m_indexProperty) == index)
		{
			return i;
		}
	}
	return -1;
}
QStringList SyncableList::keys() const
{
	QSet<QString> out;
	for (const QVariantMap &row : m_rows)
	{
		out |= row.keys().toSet();
	}
	return out.toList();
}

SyncableQObjectList::SyncableQObjectList(const QString &channel, const QString &cmdPrefix, const QString &indexProperty, const Flags &flags, QObject *parent)
	: BaseSyncableList(channel, cmdPrefix, indexProperty, flags & ~AllowExternalAdd, parent)
{
}

void SyncableQObjectList::add(QObject *obj)
{
	if (m_mapping.contains(indexValue(obj)))
	{
		if (m_flags.testFlag(AllowOverwrite))
		{
			remove(m_mapping.value(indexValue(obj)));
		}
		else
		{
			return;
		}
	}
	m_objects.append(obj);
	m_mapping.insert(indexValue(obj), obj);
	subscribeTo(m_channel + ':' + indexValue(obj).toString());
	emit broadcast(m_channel, command("added"), QJsonObject::fromVariantMap(objToExt(obj)));

	const QMetaObject *mo = obj->metaObject();
	for (const QString &property : m_objPropToExtProp.keys())
	{
		const QMetaProperty prop = mo->property(mo->indexOfProperty(property.toLatin1().constData()));
		Q_ASSERT(prop.isValid());
		if (!prop.isConstant())
		{
			Q_ASSERT(prop.hasNotifySignal());
			const QMetaMethod signal = prop.notifySignal();
			if (!m_signalToProperty[signal].contains(property))
			{
				m_signalToProperty[signal].append(property);
			}
			connect(obj, signal, this, findOwnSlot("propertyChanged()"));
		}
	}
	for (auto it = m_extToWrappedMapping.constBegin(); it != m_extToWrappedMapping.constEnd(); ++it)
	{
		QObject *wrapped = wrappedObject(obj, it.key());
		m_wrappedToWrapper.insert(wrapped, obj);
		const QMetaObject *wrappedMo = wrapped->metaObject();
		const QMetaProperty prop = wrappedMo->property(wrappedMo->indexOfProperty(it.value().second.toUtf8().constData()));
		Q_ASSERT(prop.isValid());
		if (!prop.isConstant())
		{
			Q_ASSERT(prop.hasNotifySignal());
			const QMetaMethod signal = prop.notifySignal();
			if (!m_signalToProperty[signal].contains(it.key()))
			{
				m_signalToProperty[signal].append(it.key());
			}
			connect(wrapped, signal, this, findOwnSlot("wrappedPropertyChanged()"));
		}
	}
}
void SyncableQObjectList::remove(QObject *obj)
{
	unsubscribeFrom(m_channel + ':' + indexValue(obj).toString());
	remove(m_objects.indexOf(obj), QUuid());
}

void SyncableQObjectList::set(const int index, const QString &property, const QVariant &value, const QUuid &origin)
{
	Q_ASSERT(m_extPropToObjProp.contains(property) || m_extToWrappedMapping.contains(property));
	if (m_extPropToObjProp.contains(property))
	{
		m_objects.at(index)->setProperty(m_extPropToObjProp.value(property).toUtf8().constData(), transformToList(property, value));
	}
	else if (m_extToWrappedMapping.contains(property))
	{
		QObject *wrapped = wrappedObject(m_objects.at(index), property);
		wrapped->setProperty(m_extToWrappedMapping[property].second.toUtf8().constData(), transformToList(property, value));
	}
	emit changed(index, property);
}
QVariant SyncableQObjectList::get(const int index, const QString &property) const
{
	Q_ASSERT(m_extPropToObjProp.contains(property) || m_extToWrappedMapping.contains(property));
	if (m_extPropToObjProp.contains(property))
	{
		return transformFromList(property, m_objects.at(index)->property(m_extPropToObjProp.value(property).toUtf8().constData()));
	}
	else if (m_extToWrappedMapping.contains(property))
	{
		QObject *wrapped = wrappedObject(m_objects.at(index), property);
		return transformFromList(property, wrapped->property(m_extToWrappedMapping[property].second.toUtf8().constData()));
	}
	else
	{
		return QVariant();
	}
}
QVariantMap SyncableQObjectList::getAll(const int index) const
{
	return objToExt(m_objects.at(index));
}
void SyncableQObjectList::add(const QMap<QString, QVariant> &values, const QUuid &origin)
{
	Q_ASSERT_X(false, "SyncableQObjectList::add", "SyncableQObjectList does not allow adding an object by values");
}
void SyncableQObjectList::remove(const int index, const QUuid &origin)
{
	QObject *obj = m_objects.takeAt(index);
	emit broadcast(m_channel, command("removed"), {{m_indexProperty, toJson(obj->property(m_extPropToObjProp[m_indexProperty].toUtf8().constData()))}}, origin);
	delete obj;
}

int SyncableQObjectList::findIndex(const QVariant &index) const
{
	return m_objects.indexOf(m_mapping.value(index));
}
QStringList SyncableQObjectList::keys() const
{
	return m_extPropToObjProp.keys() + m_extToWrappedMapping.keys();
}
QVariantMap SyncableQObjectList::objToExt(QObject *obj) const
{
	QVariantMap out;
	for (auto it = m_objPropToExtProp.constBegin(); it != m_objPropToExtProp.constEnd(); ++it)
	{
		out.insert(it.value(), transformFromList(it.value(), obj->property(it.key().toUtf8().constData())));
	}
	for (auto it = m_wrappedPropMapping.constBegin(); it != m_wrappedPropMapping.constEnd(); ++it)
	{
		QObject *wrapped = wrappedObject(obj, it.value().second);
		out.insert(it.value().second, transformFromList(it.value().second, wrapped->property(it.key().toUtf8().constData())));
	}
	return out;
}
void SyncableQObjectList::extToObj(QObject *obj, const QVariantMap &values)
{
	for (auto it = values.constBegin(); it != values.constEnd(); ++it)
	{
		if (m_extPropToObjProp.contains(it.key()))
		{
			obj->setProperty(m_extPropToObjProp[it.key()].toUtf8().constData(), transformToList(it.key(), it.value()));
		}
		if (m_extToWrappedMapping.contains(it.key()))
		{
			QObject *wrapped = wrappedObject(obj, it.key());
			wrapped->setProperty(m_extToWrappedMapping[it.key()].second.toUtf8().constData(), transformToList(it.key(), it.value()));
		}
	}
}
QVariant SyncableQObjectList::indexValue(QObject *obj) const
{
	const int index = m_objects.indexOf(obj);
	if (index == -1)
	{
		return QVariant();
	}
	return get(m_objects.indexOf(obj), m_indexProperty);
}

QObject *SyncableQObjectList::wrappedObject(QObject *obj, const QString &extProp) const
{
	Q_ASSERT(obj->metaObject()->property(obj->metaObject()->indexOfProperty(m_extToWrappedMapping[extProp].first.toUtf8().constData())).isConstant());
	return obj->property(m_extToWrappedMapping[extProp].first.toUtf8().constData()).value<QObject *>();
}

QMetaMethod SyncableQObjectList::findOwnSlot(const char *slot) const
{
	Q_ASSERT(metaObject()->indexOfSlot(slot) != -1);
	return metaObject()->method(metaObject()->indexOfSlot(slot));
}

void SyncableQObjectList::addPropertyMapping(const QString &objectProperty, const QString &externalProperty)
{
	m_objPropToExtProp.insert(objectProperty, externalProperty);
	m_extPropToObjProp.insert(externalProperty, objectProperty);
}
void SyncableQObjectList::addPropertyMapping(const QString &wrappedObjectProperty, const QString &objectProperty, const QString &externalProperty)
{
	m_wrappedPropMapping.insert(objectProperty, qMakePair(wrappedObjectProperty, externalProperty));
	m_extToWrappedMapping.insert(externalProperty, qMakePair(wrappedObjectProperty, objectProperty));
}

void SyncableQObjectList::addCommandMapping(const QString &cmd, const char *method, const QStringList &arguments)
{
	m_commandMapping.insert(cmd, qMakePair(QMetaObject::normalizedSignature(method), arguments));
}

void SyncableQObjectList::propertyChanged()
{
	QObject *obj = sender();
	for (const QString &property : m_signalToProperty.value(obj->metaObject()->method(senderSignalIndex())))
	{
		emit broadcast(m_channel, command("changed"), {{m_indexProperty, toJson(indexValue(obj))},
													  {property, toJson(get(m_objects.indexOf(obj), property))}});
		emit changed(m_objects.indexOf(obj), property);
	}
}
void SyncableQObjectList::wrappedPropertyChanged()
{
	QObject *obj = m_wrappedToWrapper[sender()];
	for (const QString &property : m_signalToProperty.value(obj->metaObject()->method(senderSignalIndex())))
	{
		emit broadcast(m_channel, command("changed"), {{m_indexProperty, toJson(indexValue(obj))},
					   {property, toJson(get(m_objects.indexOf(obj), property))}});
		emit changed(m_objects.indexOf(sender()), property);
	}
}

template<typename T>
inline static const void *toConstVoidStart(const T &t)
{
	return static_cast<const void *>(&t);
}
static const void *variantToData(const QVariant &variant)
{
	switch (variant.type())
	{
	case QVariant::Bool: return toConstVoidStart(variant.toBool());
	case QVariant::Int: return toConstVoidStart(variant.toInt());
	case QVariant::UInt: return toConstVoidStart(variant.toUInt());
	case QVariant::LongLong: return toConstVoidStart(variant.toLongLong());
	case QVariant::ULongLong: return toConstVoidStart(variant.toULongLong());
	case QVariant::Double: return toConstVoidStart(variant.toDouble());
	case QVariant::Char: return toConstVoidStart(variant.toChar());
	case QVariant::Map: return toConstVoidStart(variant.toMap());
	case QVariant::List: return toConstVoidStart(variant.toString());
	case QVariant::String: return toConstVoidStart(variant.toString());
	case QVariant::StringList: return toConstVoidStart(variant.toStringList());
	case QVariant::ByteArray: return toConstVoidStart(variant.toByteArray());
	case QVariant::Date: return toConstVoidStart(variant.toDate());
	case QVariant::Time: return toConstVoidStart(variant.toTime());
	case QVariant::DateTime: return toConstVoidStart(variant.toDateTime());
	case QVariant::Url: return toConstVoidStart(variant.toUrl());
	case QVariant::Hash: return toConstVoidStart(variant.toHash());
	case QVariant::Uuid: return toConstVoidStart(variant.toUuid());
	default:
		Q_ASSERT_X(false, "SyncableList, variantToData", "Variant type not implemented");
		return nullptr;
	}
}
static QGenericArgument variantToArgument(const QVariant &variant)
{
	return QGenericArgument(variant.typeName(), variantToData(variant));
}

void SyncableQObjectList::toClient(const QJsonObject &data)
{
	using namespace Json;
	BaseSyncableList::toClient(data);

	const QString channel = ensureString(data, "channel");
	const QString cmd = ensureString(data, "cmd");
	const QUuid msgId = ensureUuid(data, "msgId");

	if (channel.startsWith(m_channel + ':') && m_commandMapping.contains(cmd))
	{
		const QString id = QString(channel).remove(m_channel + ':');
		QObject *obj = m_mapping.value(id);
		const QMetaObject *mo = obj->metaObject();
		const QMetaMethod method = mo->method(mo->indexOfMethod(m_commandMapping.value(cmd).first.constData()));
		Q_ASSERT(method.isValid());

		// TODO return data?
		const QStringList args = m_commandMapping.value(cmd).second;
		if (args.isEmpty())
		{
			if (method.parameterType(0) == qMetaTypeId<QUuid>())
			{
				Q_ASSERT(method.parameterCount() == 1);
				method.invoke(obj, Q_ARG(QUuid, msgId));
			}
			else
			{
				Q_ASSERT(method.parameterCount() == 0);
				method.invoke(obj);
			}
		}
		else
		{
			Q_ASSERT(method.parameterCount() == args.size());
			switch (args.size())
			{
			case 0: method.invoke(obj); break;
			case 1: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0)))); break;
			case 2: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1)))); break;
			case 3: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2)))); break;
			case 4: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3)))); break;
			case 5: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4)))); break;
			case 6: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4))),
								  variantToArgument(ensureVariant(data, args.at(5)))); break;
			case 7: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4))),
								  variantToArgument(ensureVariant(data, args.at(5))),
								  variantToArgument(ensureVariant(data, args.at(6)))); break;
			case 8: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4))),
								  variantToArgument(ensureVariant(data, args.at(5))),
								  variantToArgument(ensureVariant(data, args.at(6))),
								  variantToArgument(ensureVariant(data, args.at(7)))); break;
			case 9: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4))),
								  variantToArgument(ensureVariant(data, args.at(5))),
								  variantToArgument(ensureVariant(data, args.at(6))),
								  variantToArgument(ensureVariant(data, args.at(7))),
								  variantToArgument(ensureVariant(data, args.at(8)))); break;
			case 10: method.invoke(obj, variantToArgument(ensureVariant(data, args.at(0))),
								  variantToArgument(ensureVariant(data, args.at(1))),
								  variantToArgument(ensureVariant(data, args.at(2))),
								  variantToArgument(ensureVariant(data, args.at(3))),
								  variantToArgument(ensureVariant(data, args.at(4))),
								  variantToArgument(ensureVariant(data, args.at(5))),
								  variantToArgument(ensureVariant(data, args.at(6))),
								  variantToArgument(ensureVariant(data, args.at(7))),
								  variantToArgument(ensureVariant(data, args.at(8))),
								  variantToArgument(ensureVariant(data, args.at(9)))); break;
			}
		}
	}
}
