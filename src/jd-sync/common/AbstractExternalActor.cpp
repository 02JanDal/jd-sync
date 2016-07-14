#include "AbstractExternalActor.h"

#include "MessageHub.h"
#include "Message.h"

AbstractExternalActor::AbstractExternalActor(MessageHub *h, QObject *parent)
	: AbstractThreadedActor(h, parent) {}

void AbstractExternalActor::received(const Message &message)
{
	if (message.isInternal()) {
		return;
	}
	sendToExternal(message);
}

void AbstractExternalActor::receivedFromExternal(const Message &message)
{
	send(message);
}
