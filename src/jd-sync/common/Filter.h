#pragma once

#include <QVariant>
#include <QVector>

class Filter;

class FilterPart
{
public:
	enum Op
	{
		Equal,
		Greater,
		GreaterOrEqual,
		Less,
		LessOrEqual,
		InSet,
		Like
	};

	explicit FilterPart(const QString &property, const int op, const QVariant &value, const bool negated = false);
	explicit FilterPart() {}

	QString property() const { return m_property; }
	QVariant value() const { return m_value; }
	Op operation() const { return m_operation; }
	bool isNegated() const { return m_negated; }

	bool matches(const QVariantHash &record) const;

	QJsonObject toJson() const;
	static FilterPart fromJson(const QJsonObject &obj);

	QString toSql(QVector<QPair<QString, QVariant>> *parameters) const;

private:
	QString m_property;
	QVariant m_value;
	Op m_operation;
	bool m_negated;
};

class FilterGroup
{
public:
	enum Op
	{
		And,
		Or
	};

	explicit FilterGroup(const QVector<FilterPart> &parts, const QVector<FilterGroup> &groups, const Op op = And, const bool negated = false);
	explicit FilterGroup(const FilterPart &part);
	explicit FilterGroup() {}

	QVector<FilterPart> parts() const { return m_parts; }
	QVector<FilterGroup> groups() const { return m_groups; }
	Op operation() const { return m_operation; }
	bool isNegated() const { return m_negated; }

	bool isEmpty() const;

	bool matches(const QVariantHash &record) const;

	QJsonObject toJson() const;
	static FilterGroup fromJson(const QJsonObject &obj);

	QString toSql(QVector<QPair<QString, QVariant>> *parameters) const;

private:
	QVector<FilterPart> m_parts;
	QVector<FilterGroup> m_groups;
	Op m_operation;
	bool m_negated;
};

class Filter : public FilterGroup
{
public:
	explicit Filter(const QVector<FilterPart> &parts, const QVector<FilterGroup> &groups, const Op op = And, const bool negated = false);
	explicit Filter(const FilterPart &part);
	explicit Filter(const FilterGroup &mainGroup);
	Filter() {}
};
