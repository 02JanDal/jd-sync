#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "MessageHub.h"
#include "Message.h"
#include "AbstractActor.h"

#include "DummyActor.h"

TEST_CASE("actors register and unregister", "[MessageHub][AbstractActor]") {
	MessageHub hub;
	DummyActor a1{&hub};
	DummyActor a2{&hub};
	DummyActor a3{&hub};
	DummyActor a4{&hub};
	REQUIRE(hub.actors() == QSet<AbstractActor *>({&a1, &a2, &a3, &a4}));
	REQUIRE(a1.hub() == &hub);

	SECTION("can unregister") {
		hub.unregisterActor(&a2);
		REQUIRE(!hub.actors().contains(&a2));
	}

	SECTION("can reregister") {
		hub.unregisterActor(&a3);
		hub.registerActor(&a3);
		REQUIRE(hub.actors().contains(&a3));
	}
}

TEST_CASE("actors subscribe", "[MessageHub][AbstractActor]") {
	MessageHub hub;
	DummyActor a1{&hub};
	DummyActor a2{&hub};
	DummyActor a3{&hub};
	DummyActor a4{&hub};
	REQUIRE(a1.channels().isEmpty());
	REQUIRE(a2.channels().isEmpty());
	REQUIRE(a3.channels().isEmpty());
	REQUIRE(a4.channels().isEmpty());

	SECTION("can subscribe") {
		a1.subscribeTo("a");
		REQUIRE(a1.channels() == QSet<QString>({"a"}));
	}
	SECTION("can unsubscribe") {
		a2.subscribeTo("a");
		REQUIRE(a2.channels() == QSet<QString>({"a"}));
		a2.unsubscribeFrom("a");
		REQUIRE(a2.channels().isEmpty());
	}
	SECTION("retains subscribtion after re-register") {
		a1.subscribeTo("a");
		a3.subscribeTo("a");
		hub.unregisterActor(&a3);
		a1.send(Message("a", "test1"));
		hub.registerActor(&a3);
		a1.send(Message("a", "test2"));

		REQUIRE(a3.messages() == QVector<Message>({Message("a", "test2")}));
	}
	SECTION("subscription by message") {
		a4.send(Message("client", "subscribe", QJsonObject({{"channel", "a"}})));
		REQUIRE(a4.channels() == QSet<QString>({"a"}));
		a4.send(Message("client", "unsubscribe", QJsonObject({{"channel", "a"}})));
		REQUIRE(a4.channels().isEmpty());
	}
}

TEST_CASE("messages are routed", "[MessageHub][AbstractActor]") {
	MessageHub hub;
	DummyActor a1{&hub};
	DummyActor a2{&hub};
	DummyActor a3{&hub};
	DummyActor a4{&hub};

	a1.subscribeTo("a");
	a2.subscribeTo("a");
	a2.subscribeTo("b");
	a3.subscribeTo("a");
	a3.subscribeTo("b");
	a4.subscribeTo("*");

	a1.send(Message("a", "test1"));
	a2.send(Message("a", "test2"));
	a2.send(Message("b", "test3"));

	REQUIRE(a1.messages() == QVector<Message>({Message("a", "test2")}));
	REQUIRE(a2.messages() == QVector<Message>({Message("a", "test1")}));
	REQUIRE(a3.messages() == QVector<Message>({Message("a", "test1"), Message("a", "test2"), Message("b", "test3")}));
	REQUIRE(a3.messages() == QVector<Message>({Message("a", "test1"), Message("a", "test2"), Message("b", "test3")}));
}

TEST_CASE("resets are sent", "[MessageHub][AbstractActor]") {
	MessageHub hub;
	DummyActor a1{&hub};
	DummyActor a2{&hub};
	DummyActor a3{&hub};
	REQUIRE(a1.resets() == 0);
	REQUIRE(a2.resets() == 0);
	REQUIRE(a3.resets() == 0);

	a1.send(Message("client", "reset"));
	REQUIRE(a1.resets() == 1);
	REQUIRE(a2.resets() == 1);
	REQUIRE(a3.resets() == 1);
}

TEST_CASE("pings are replied to", "[MessageHub]") {
	MessageHub hub;
	DummyActor a{&hub};
	a.subscribeTo("client.ping");
	REQUIRE(a.messages().isEmpty());
	a.send(Message("client.ping", "request", QJsonObject({{"timestamp", 42424242}})));
	REQUIRE(a.messages() == QVector<Message>({Message("client.ping", "reply", QJsonObject({{"timestamp", 42424242}}))}));
}

TEST_CASE("targeted replies are targeted", "[MessageHub]") {
	MessageHub hub;
	DummyActor a1{&hub};
	DummyActor a2{&hub};
	DummyActor a3{&hub};

	a1.subscribeTo("a");
	a2.subscribeTo("a");
	a3.subscribeTo("a");

	a1.send(Message("a", "test1"));
	a2.send(a2.messages().last().createTargetedReply("test2"));
	REQUIRE(a1.messages() == QVector<Message>({Message("a", "test2")})); // a1 sends test1, so will only receive test2
	REQUIRE(a2.messages() == QVector<Message>({Message("a", "test1")})); // a2 sends test2, so will only receive test1
	REQUIRE(a3.messages() == QVector<Message>({Message("a", "test1")})); // a3 is not the target of test2, so will only receive test1
}
