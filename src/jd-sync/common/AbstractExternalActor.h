#pragma once

#include "AbstractThreadedActor.h"

class AbstractExternalActor : public AbstractThreadedActor
{
public:
	INTROSPECTION

	AbstractExternalActor(MessageHub *h, QObject *parent = nullptr);
	virtual ~AbstractExternalActor() {}

private:
	void received(const Message &message) override final;

protected:
	void receivedFromExternal(const Message &message);
	virtual void sendToExternal(const Message &message) = 0;
};
