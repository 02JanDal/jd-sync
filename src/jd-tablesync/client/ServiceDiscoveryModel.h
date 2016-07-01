#pragma once

#include <QAbstractListModel>
#include <QLoggingCategory>

class ServiceDiscoveryModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit ServiceDiscoveryModel(const QString &type, QObject *parent = nullptr);

	enum ExtraRoles
	{
		NameRole = Qt::UserRole,
		AddressRole,
		DomainRole,
		HostRole,
		PortRole,
		TxtRole
	};

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QHash<int, QByteArray> roleNames() const override;

signals:
	void error(const QString &error);

private:
	friend class ServiceDiscoveryModelPrivate;
	class ServiceDiscoveryModelPrivate *d;
};

Q_DECLARE_LOGGING_CATEGORY(ServiceDiscovery)
