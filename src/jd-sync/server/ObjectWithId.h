#pragma once

#include <QUuid>

class ObjectWithId
{
public:
	explicit ObjectWithId() : m_id(QUuid::createUuid()) {}
	explicit ObjectWithId(const QUuid &id) : m_id(id) {}

	QUuid id() const { return m_id; }

private:
	QUuid m_id;
};

#define OBJECTWITHID Q_PROPERTY(QUuid id READ id CONSTANT)
