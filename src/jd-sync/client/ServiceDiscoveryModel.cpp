#include "ServiceDiscoveryModel.h"

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include "../common/3rdparty/avahi-qt/qt-watch.h"

#include <jd-util/Exception.h>
#include <jd-util/Compiler.h>

DECLARE_EXCEPTION(ServiceDiscovery)
Q_LOGGING_CATEGORY(ServiceDiscovery, "tablesync.service-discovery.client")

class ServiceDiscoveryModelPrivate
{
public:
	explicit ServiceDiscoveryModelPrivate(ServiceDiscoveryModel *m, const QString &t)
		: model(m), type(t)
	{
		int error;
		client = avahi_client_new(avahi_qt_poll_get(), AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
		if (!client) {
			throw ServiceDiscoveryException(errorFromCode(error));
		}

		browser = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, type.toLatin1().constData(), nullptr, AvahiLookupFlags(0), service_browser_callback, this);

		qCDebug(ServiceDiscovery) << "Service discovery browser has been set up";
	}
	~ServiceDiscoveryModelPrivate()
	{
		if (browser) {
			avahi_service_browser_free(browser);
		}
		if (client) {
			avahi_client_free(client);
		}
		qCDebug(ServiceDiscovery) << "Service discovery browser has been destroyed";
	}

	ServiceDiscoveryModel *model;
	QString type;

	struct Service
	{
		QString address;
		QString domain;
		QString host;
		quint16 port;
		QString name;
		QHash<QString, QString> txt;
	};
	QVector<Service> services;

	AvahiClient *client = nullptr;
	AvahiServiceBrowser *browser = nullptr;

	void notifyError(const QString &error)
	{
		emit model->error(error);
	}
	void add(const QString &address, const QString &domain, const QString &host, const quint16 port, const QString &name, const QHash<QString, QString> &txt)
	{
		model->beginInsertRows(QModelIndex(), services.size(), services.size());
		services.append(Service{address, domain, host, port, name, txt});
		model->endInsertRows();
		qCDebug(ServiceDiscovery) << "Discovered competition at" << address << "called" << name;
	}
	void remove(const QString &name)
	{
		auto it = std::find_if(services.begin(), services.end(), [name](const Service &s) { return s.name == name; });
		if (it != services.end()) {
			const int index = std::distance(services.begin(), it);
			qCDebug(ServiceDiscovery) << "Competition at" << services.at(index).address << "called" << name << "disappeared";

			model->beginRemoveRows(QModelIndex(), index, index);
			services.removeAt(index);
			model->endRemoveRows();
		}
	}

	static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata)
	{
		ServiceDiscoveryModelPrivate *data = static_cast<ServiceDiscoveryModelPrivate *>(userdata);

		switch (state) {
		case AVAHI_CLIENT_FAILURE:
			data->notifyError(data->errorFromClient());
			break;

		case AVAHI_CLIENT_S_REGISTERING: DECL_FALLTHROUGH;
		case AVAHI_CLIENT_S_RUNNING: DECL_FALLTHROUGH;
		case AVAHI_CLIENT_S_COLLISION: DECL_FALLTHROUGH;
		case AVAHI_CLIENT_CONNECTING: break;
		}
	}
	static void service_browser_callback(AvahiServiceBrowser *browser, AvahiIfIndex iface, AvahiProtocol proto,
										 AvahiBrowserEvent event,
										 const char *name, const char *type, const char *domain,
										 AvahiLookupResultFlags flags,
										 void *userdata)
	{
		ServiceDiscoveryModelPrivate *data = static_cast<ServiceDiscoveryModelPrivate *>(userdata);

		switch (event) {
		case AVAHI_BROWSER_FAILURE:
			data->notifyError(data->errorFromClient());
			break;

		case AVAHI_BROWSER_NEW:
			if (!avahi_service_resolver_new(data->client, iface, proto, name, type, domain, AVAHI_PROTO_UNSPEC, AvahiLookupFlags(0), resolver_callback, userdata)) {
				data->notifyError(data->errorFromClient());
			}
			break;

		case AVAHI_BROWSER_REMOVE:
			data->remove(QString::fromLatin1(name));
			break;

		case AVAHI_BROWSER_ALL_FOR_NOW: DECL_FALLTHROUGH;
		case AVAHI_BROWSER_CACHE_EXHAUSTED: break;
		}
	}
	static void resolver_callback(AvahiServiceResolver *resolver, AvahiIfIndex iface, AvahiProtocol proto,
								  AvahiResolverEvent event,
								  const char *name, const char *type, const char *domain, const char *hostname,
								  const AvahiAddress *address, uint16_t port, AvahiStringList *txt,
								  AvahiLookupResultFlags flags, void *userdata)
	{
		ServiceDiscoveryModelPrivate *data = static_cast<ServiceDiscoveryModelPrivate *>(userdata);

		switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			data->notifyError(data->errorFromClient());
			break;

		case AVAHI_RESOLVER_FOUND:
			char addr[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(addr, sizeof(addr), address);

			data->add(QString::fromLatin1(addr),
					  QString::fromLatin1(domain),
					  QString::fromLatin1(hostname),
					  port,
					  QString::fromLatin1(name),
					  stringList(txt));
			break;
		}

		avahi_service_resolver_free(resolver);
	}

	static QString errorFromCode(const int code)
	{
		return QString::fromLocal8Bit(avahi_strerror(code));
	}
	QString errorFromClient()
	{
		return errorFromCode(avahi_client_errno(client));
	}

	static QHash<QString, QString> stringList(AvahiStringList *strings)
	{
		QHash<QString, QString> out;
		while (strings) {
			char *key, *value;
			std::size_t size;
			avahi_string_list_get_pair(strings, &key, &value, &size);
			if (value) {
				out.insert(QString::fromLatin1(key), QString::fromLatin1(value, size));
				avahi_free(value);
			} else {
				out.insert(QString::fromLatin1(key), QString());
			}
			avahi_free(key);
			strings = avahi_string_list_get_next(strings);
		}
		return out;
	}
};

ServiceDiscoveryModel::ServiceDiscoveryModel(const QString &type, QObject *parent)
	: QAbstractListModel(parent), d(new ServiceDiscoveryModelPrivate(this, type))
{
}
ServiceDiscoveryModel::~ServiceDiscoveryModel()
{
	delete d;
}

int ServiceDiscoveryModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : d->services.size();
}
int ServiceDiscoveryModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant ServiceDiscoveryModel::data(const QModelIndex &index, int role) const
{
	const ServiceDiscoveryModelPrivate::Service service = d->services.at(index.row());
	switch (role) {
	case Qt::DisplayRole:
		if (index.column() == 0) {
			return service.name;
		}
		break;
	case NameRole: return service.name;
	case AddressRole: return service.address;
	case DomainRole: return service.domain;
	case HostRole: return service.host;
	case PortRole: return service.port;
	case TxtRole: return QVariant::fromValue(service.txt);
	}

	return QVariant();
}
QVariant ServiceDiscoveryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0) {
		return tr("Name");
	}
	return QVariant();
}

Qt::ItemFlags ServiceDiscoveryModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> ServiceDiscoveryModel::roleNames() const
{
	QHash<int, QByteArray> out = QAbstractListModel::roleNames();
	out[NameRole] = "name";
	out[AddressRole] = "address";
	out[DomainRole] = "domain";
	out[HostRole] = "host";
	out[PortRole] = "port";
	out[TxtRole] = "txt";
	return out;
}
