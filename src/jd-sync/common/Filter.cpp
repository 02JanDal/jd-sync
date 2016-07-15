#include "Filter.h"

#include <QRegExp>
#include <QRegularExpression>

#include <jd-util/Json.h>
#include <jd-util/Functional.h>

using namespace JD::Util;

static QString toString(const FilterPart::Op op)
{
	switch (op) {
	case FilterPart::Equal: return "==";
	case FilterPart::Greater: return ">";
	case FilterPart::GreaterOrEqual: return ">=";
	case FilterPart::Less: return "<";
	case FilterPart::LessOrEqual: return "<=";
	case FilterPart::InSet: return "in";
	case FilterPart::Like: return "like";
	}
}
static FilterPart::Op filterPartOpFromString(const QString &str)
{
	if (str == "==") {
		return FilterPart::Equal;
	} else if (str == ">") {
		return FilterPart::Greater;
	} else if (str == ">=") {
		return FilterPart::GreaterOrEqual;
	} else if (str == "<") {
		return FilterPart::Less;
	} else if (str == "<=") {
		return FilterPart::LessOrEqual;
	} else if (str == "in") {
		return FilterPart::InSet;
	} else if (str == "like") {
		return FilterPart::Like;
	} else {
		throw Json::JsonException("Unknown operation: " + str);
	}
}
static QString toString(const FilterGroup::Op op)
{
	switch (op) {
	case FilterGroup::And: return "and";
	case FilterGroup::Or: return "or";
	}
}
static FilterGroup::Op filterGroupOpFromString(const QString &str)
{
	if (str == "and") {
		return FilterGroup::And;
	} else if (str == "or") {
		return FilterGroup::Or;
	} else {
		throw Json::JsonException("Unknown operation: " + str);
	}
}

FilterPart::FilterPart(const QString &property, const int op, const QVariant &value, const bool negated)
	: m_property(property), m_value(value), m_operation(Op(op)), m_negated(negated)
{
	if (m_operation == Like && m_value.type() == QVariant::String) {
		m_value = QRegExp(m_value.toString(), Qt::CaseInsensitive, QRegExp::Wildcard);
	}
}

bool FilterPart::matches(const QVariantHash &record) const
{
	auto matchesInternal = [this, record]()
	{
		switch (m_operation) {
		case FilterPart::Equal: return record.value(m_property) == m_value;
		case FilterPart::Greater: return record.value(m_property) > m_value;
		case FilterPart::GreaterOrEqual: return record.value(m_property) >= m_value;
		case FilterPart::Less: return record.value(m_property) < m_value;
		case FilterPart::LessOrEqual: return record.value(m_property) <= m_value;
		case FilterPart::InSet: return m_value.toList().contains(record.value(m_property));
		case FilterPart::Like: {
			const QString value = record.value(m_property).toString();
			if (m_value.type() == QVariant::RegExp) {
				return value.contains(m_value.toRegExp());
			} else if (m_value.type() == QVariant::RegularExpression) {
				return value.contains(m_value.toRegularExpression());
			} else {
				return value.contains(m_value.toString());
			}
		}
		}
	};

	if (m_negated) {
		return !matchesInternal();
	} else {
		return matchesInternal();
	}
}

QJsonObject FilterPart::toJson() const
{
	return QJsonObject({
						   {"p", m_property},
						   {"v", Json::toJson(m_value)},
						   {"o", toString(m_operation)},
						   {"n", m_negated}
					   });
}
FilterPart FilterPart::fromJson(const QJsonObject &obj)
{
	return FilterPart(
				Json::ensureString(obj, "p"),
				filterPartOpFromString(Json::ensureString(obj, "o")),
				Json::ensureVariant(obj, "v"),
				Json::ensureBoolean(obj, "n", false)
				);
}

QString FilterPart::toSql(QVector<QPair<QString, QVariant> > *parameters) const
{
	QString sql = m_property + ' ';
	if (m_negated) {
		switch (m_operation) {
		case Equal:          sql += "!="; break;
		case Greater:        sql += "<="; break;
		case GreaterOrEqual: sql += '<'; break;
		case Less:           sql += ">="; break;
		case LessOrEqual:    sql += ">"; break;
		case InSet:          sql += "NOT IN"; break;
		case Like:           sql += "NOT LIKE"; break;
		}
	} else {
		switch (m_operation) {
		case Equal:          sql += '='; break;
		case Greater:        sql += '>'; break;
		case GreaterOrEqual: sql += ">="; break;
		case Less:           sql += '<'; break;
		case LessOrEqual:    sql += "<="; break;
		case InSet:          sql += "IN"; break;
		case Like:           sql += "LIKE"; break;
		}
	}

	if (m_operation == InSet) {
		sql += " (" + Functional::collection(QVector<QString>(m_value.toList().size(), "?")).join(',') + ')';
		for (const QVariant &value : m_value.toList()) {
			parameters->append(qMakePair(m_property, value));
		}
	} else if (m_operation == Like) {
		sql += '?';
		// TODO handle \* and escape % before replacing
		parameters->append(qMakePair(m_property, m_value.toString().replace('*', '%')));
	} else {
		sql += '?';
		parameters->append(qMakePair(m_property, m_value));
	}
	return sql;
}

FilterGroup::FilterGroup(const QVector<FilterPart> &parts, const QVector<FilterGroup> &groups, const FilterGroup::Op op, const bool negated)
	: m_parts(parts), m_groups(groups), m_operation(op), m_negated(negated) {}

FilterGroup::FilterGroup(const FilterPart &part)
	: FilterGroup(QVector<FilterPart>() << part, QVector<FilterGroup>()) {}

bool FilterGroup::isEmpty() const
{
	return m_parts.isEmpty() && m_groups.isEmpty();
}

bool FilterGroup::matches(const QVariantHash &record) const
{
	auto matchesInternal = [this, record]()
	{
		if (m_operation == And) {
			return std::all_of(m_parts.begin(), m_parts.end(), [record](const FilterPart &p) { return p.matches(record); })
					&& std::all_of(m_groups.begin(), m_groups.end(), [record](const FilterGroup &g) { return g.matches(record); });
		} else if (m_operation == Or) {
			return std::any_of(m_parts.begin(), m_parts.end(), [record](const FilterPart &p) { return p.matches(record); })
					&& std::any_of(m_groups.begin(), m_groups.end(), [record](const FilterGroup &g) { return g.matches(record); });
		} else {
			throw NotReachableException();
		}
	};

	if (m_negated) {
		return !matchesInternal();
	} else {
		return matchesInternal();
	}
}

QJsonObject FilterGroup::toJson() const
{
	using namespace JD::Util;
	return QJsonObject({
						   {"p", Json::toJsonArray(Functional::map(m_parts, [](const FilterPart &p) { return p.toJson(); }))},
						   {"g", Json::toJsonArray(Functional::map(m_groups, [](const FilterGroup &g) { return g.toJson(); }))},
						   {"o", toString(m_operation)},
						   {"n", m_negated}
					   });
}
FilterGroup FilterGroup::fromJson(const QJsonObject &obj)
{
	using namespace JD::Util;
	return FilterGroup(
				Functional::map(Json::ensureIsArrayOf<QJsonObject>(obj, "p"), &FilterPart::fromJson),
				Functional::map(Json::ensureIsArrayOf<QJsonObject>(obj, "g"), &FilterGroup::fromJson),
				filterGroupOpFromString(Json::ensureString(obj, "o")),
				Json::ensureBoolean(obj, QStringLiteral("n"))
				);
}

QString FilterGroup::toSql(QVector<QPair<QString, QVariant>> *parameters) const
{
	const QString joiner = m_operation == And ? " AND " : " OR ";
	const QString negation = m_negated ? "NOT " : "";
	const QVector<QString> parts = Functional::map(m_parts, [parameters](const FilterPart &part) { return part.toSql(parameters); });
	const QVector<QString> groups = Functional::map(m_groups, [parameters](const FilterGroup &group) { return group.toSql(parameters); });
	return negation + '(' + Functional::collection(parts + groups).join(joiner) + ')';
}

Filter::Filter(const QVector<FilterPart> &parts, const QVector<FilterGroup> &groups, const FilterGroup::Op op, const bool negated)
	: FilterGroup(parts, groups, op, negated) {}
Filter::Filter(const FilterPart &part)
	: FilterGroup(part) {}
Filter::Filter(const FilterGroup &mainGroup)
	: FilterGroup(mainGroup.parts(), mainGroup.groups(), mainGroup.operation(), mainGroup.isNegated()) {}
