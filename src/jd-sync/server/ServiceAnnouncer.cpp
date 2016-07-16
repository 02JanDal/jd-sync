#include "ServiceAnnouncer.h"

#include <QVector>

#include <avahi-client/publish.h>
#include <avahi-client/client.h>
#include <avahi-common/error.h>
#include <avahi-common/alternative.h>
#include <avahi-common/malloc.h>

#include <jd-util/Exception.h>
#include <jd-util/Compiler.h>

#include "../common/3rdparty/avahi-qt/qt-watch.h"

DECLARE_EXCEPTION(ServiceAnnouncer)

class ServiceAnnouncerPrivate
{
public:
	explicit ServiceAnnouncerPrivate(ServiceAnnouncer *a)
		: announcer(a)
	{
		int error;
		client = avahi_client_new(avahi_qt_poll_get(), AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
		if (!client) {
			throw ServiceAnnouncerException(QString("Failed to create client: %1").arg(errorFromCode(error)));
		}
	}
	~ServiceAnnouncerPrivate()
	{
		if (group) {
			avahi_entry_group_free(group);
		}
		if (client) {
			avahi_client_free(client);
		}
	}

	ServiceAnnouncer *announcer;
	QVector<std::tuple<QString, quint16, QString, QHash<QString, QString>>> services;

	AvahiClient *client = nullptr;
	AvahiEntryGroup *group = nullptr;

	void updateServices()
	{
		if (avahi_client_get_state(client) == AVAHI_CLIENT_S_RUNNING) {
			if (group) {
				avahi_entry_group_reset(group);
			}
			registerServices();
		}
	}
	void registerServices()
	{
		// first time here? create a new group
		if (!group) {
			group = avahi_entry_group_new(client, entry_group_callback, this);
			if (!group) {
				throw ServiceAnnouncerException("Unable to create entry group: " + errorFromClient());
			}
		}

		// empty (either because new or because reset)? fill the group
		if (avahi_entry_group_is_empty(group) && !services.isEmpty()) {
			try {
				for (const auto &service : services) {
					int attempts = 10;
					char *name = avahi_strdup(std::get<0>(service).toLatin1().constData());
					while (true) {
						int error = avahi_entry_group_add_service_strlst(
									group,
									AVAHI_IF_UNSPEC,
									AVAHI_PROTO_UNSPEC,
									AvahiPublishFlags(0),
									name,
									std::get<2>(service).toLatin1().constData(),
									nullptr,
									nullptr,
									std::get<1>(service),
									stringList(std::get<3>(service))
									);
						if (error == AVAHI_ERR_COLLISION && --attempts > 0) {
							char *n = avahi_alternative_service_name(name);
							avahi_free(name);
							name = n;
							continue; // try again!
						} else if (error) {
							throw ServiceAnnouncerException("Unable to add service to entry group: " + errorFromCode(error));
						}
						break;
					}
				}

				int error = avahi_entry_group_commit(group);
				if (error) {
					throw ServiceAnnouncerException("Unable to commit entry group: " + errorFromCode(error));
				}
			} catch (...) {
				avahi_entry_group_reset(group);
				throw;
			}
		}
	}
	void notifyError(const QString &error)
	{
		announcer->error(error);
	}

	static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata)
	{
		ServiceAnnouncerPrivate *data = static_cast<ServiceAnnouncerPrivate *>(userdata);
		data->client = client;
		try {
			switch (state) {
			case AVAHI_CLIENT_S_RUNNING:
				data->registerServices();
				break;

			case AVAHI_CLIENT_FAILURE:
				throw ServiceAnnouncerException(data->errorFromClient());

			case AVAHI_CLIENT_S_COLLISION: DECL_FALLTHROUGH;
			case AVAHI_CLIENT_S_REGISTERING:
				if (data->group) {
					avahi_entry_group_reset(data->group);
				}
				break;

			case AVAHI_CLIENT_CONNECTING:
				break;
			}
		} catch (Exception &e) {
			data->notifyError(e.cause());
		}
	}
	static void entry_group_callback(AvahiEntryGroup *group, AvahiEntryGroupState state, void *userdata)
	{
		ServiceAnnouncerPrivate *data = static_cast<ServiceAnnouncerPrivate *>(userdata);

		switch (state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED: break;
		case AVAHI_ENTRY_GROUP_COLLISION:
			// re-register all names
			avahi_entry_group_reset(group);
			data->registerServices();
			break;

		case AVAHI_ENTRY_GROUP_FAILURE:
			data->notifyError(data->errorFromClient());
			break;

		case AVAHI_ENTRY_GROUP_REGISTERING: DECL_FALLTHROUGH;
		case AVAHI_ENTRY_GROUP_UNCOMMITED: break;
		}
	}

	static QString errorFromCode(const int code)
	{
		return QString::fromLocal8Bit(avahi_strerror(code));
	}
	QString errorFromClient()
	{
		return errorFromCode(avahi_client_errno(client));
	}

	static AvahiStringList *stringList(const QHash<QString, QString> &txt)
	{
		if (txt.isEmpty()) {
			return nullptr;
		}

		const QStringList keys = txt.keys();
		AvahiStringList *strings = avahi_string_list_new((keys.first() + '=' + txt[keys.first()]).toLatin1().constData(), nullptr);
		for (const QString &key : keys.mid(1)) {
			strings = avahi_string_list_add_pair(strings, key.toLatin1().constData(), txt[key].toLatin1().constData());
		}
		return strings;
	}
};

ServiceAnnouncer::ServiceAnnouncer(QObject *parent)
	: QObject(parent), d(new ServiceAnnouncerPrivate(this))
{
}
ServiceAnnouncer::~ServiceAnnouncer()
{
	delete d;
}

void ServiceAnnouncer::registerService(const QString &name, const quint16 port, const QString &type, const QHash<QString, QString> &txt)
{
	d->services.append(std::make_tuple(name, port, type, txt));
	d->updateServices();
}

void ServiceAnnouncer::unregisterService(const QString &name)
{
	d->services.erase(std::find_if(d->services.begin(), d->services.end(), [name](const auto &tuple) { return std::get<0>(tuple) == name; }));
	d->updateServices();
}
