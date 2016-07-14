#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "Request.h"
#include "MessageHub.h"

#include "DummyActor.h"

TEST_CASE("request", "[Request]") {
	MessageHub hub;

	SECTION("success and error") {
		DummyActor actor{&hub};
		actor.subscribeTo("simple");

		Message received, error;
		Request request{&hub, Message("simple", "test1")};
		request.then([&received](const Message &msg) { received = msg; })
				.error([&error](const Message &msg) { error = msg; })
				.send();
		REQUIRE(actor.messages() == QVector<Message>({Message("simple", "test1")}));

		REQUIRE(received.isNull());
		REQUIRE(error.isNull());
		actor.send(actor.messages().first().createReply("test2", QJsonValue()));
		REQUIRE(received == Message("simple", "test2"));
		REQUIRE(error.isNull());

		received = Message();
		actor.send(actor.messages().first().createErrorReply("foobar"));
		REQUIRE(received.isNull());
		REQUIRE(error == Message("simple", "error", QJsonObject({{"msg", "foobar"}})));
	}
}
