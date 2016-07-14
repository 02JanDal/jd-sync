#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <jd-util/Json.h>

#include "Message.h"
#include "MessageHub.h"

#include "DummyActor.h"

TEST_CASE("message creation, setters and getters", "[Message]") {
	Message m1{};
	REQUIRE(m1.isNull());

	Message m2{"a", "test1", QJsonObject({{"asdf", "fdsa"}})};
	REQUIRE(m2.channel() == "a");
	REQUIRE(m2.command() == "test1");
	REQUIRE(m2.data().isObject());
	REQUIRE_NOTHROW(m2.dataObject());
	REQUIRE_THROWS_AS(m2.dataArray(), Json::JsonException);
	REQUIRE(m2.dataObject() == QJsonObject({{"asdf", "fdsa"}}));
	REQUIRE(m2.to() == nullptr);
	REQUIRE(m2.from() == nullptr);
	REQUIRE(!m2.isInternal());
	REQUIRE(!m2.isBypassingAuth());
	REQUIRE(!m2.id().isNull());

	Message m3{"b", "test2", QJsonArray({"asdf"})};
	REQUIRE(m3.channel() == "b");
	REQUIRE(m3.command() == "test2");
	REQUIRE(m3.data().isArray());
	REQUIRE_NOTHROW(m3.dataArray());
	REQUIRE_THROWS_AS(m3.dataObject(), Json::JsonException);
	REQUIRE(m3.dataArray() == QJsonArray({"asdf"}));

	Message m4{"c", "test3", "asdf"};
	REQUIRE(m4.channel() == "c");
	REQUIRE(m4.command() == "test3");
	REQUIRE(m4.data().isString());
	REQUIRE_THROWS_AS(m4.dataArray(), Json::JsonException);
	REQUIRE_THROWS_AS(m4.dataObject(), Json::JsonException);
	REQUIRE(m4.data().toString() == "asdf");

	Message m5{"d", "test4"};
	m5.setChannel("d2");
	REQUIRE(m5.channel() == "d2");

	Message m6{"e", "test5"};
	m6.setCommand("test5a");
	REQUIRE(m6.command() == "test5a");

	Message m7{"f", "test6"};
	m7.setData(QJsonObject());
	REQUIRE(m7.data().isObject());

	Message m8{"g", "test7"};
	m8.setFlags(Message::BypassAuth);
	REQUIRE(m8.isBypassingAuth());
	m8.setFlags(Message::Internal);
	REQUIRE(m8.isInternal());
	m8.setFlags(Message::BypassAuth | Message::Internal);
	REQUIRE(m8.isBypassingAuth());
	REQUIRE(m8.isInternal());
}

TEST_CASE("message reply creation, setters and getters", "[Message]") {
	Message m1{"a", "test1", QJsonObject({{"foo", "bar"}})};
	Message m2 = m1.createCopy();
	REQUIRE(m2.channel() == "a");
	REQUIRE(m2.command() == "test1");
	REQUIRE(m2.data() == m1.data());
	REQUIRE(m1.id() != m2.id());

	Message m3 = m1.createReply("test2", QJsonArray({"asdf"}));
	REQUIRE(m3.isReply());
	REQUIRE(m3.replyTo() == m1.id());
	REQUIRE(m3.channel() == m1.channel());
	REQUIRE(m3.command() == "test2");
	REQUIRE(m3.data() == QJsonArray({"asdf"}));

	Message m4 = m1.createReply("b", "test3", QJsonValue());
	REQUIRE(m4.channel() == "b");
	REQUIRE(m4.command() == "test3");
	REQUIRE(m4.replyTo() == m1.id());

	Message m5 = m1.createErrorReply("foobar");
	REQUIRE(m5.replyTo() == m1.id());
	REQUIRE(m5.channel() == m1.channel());
	REQUIRE(m5.command() == "error");
	REQUIRE(m5.data() == QJsonObject({{"msg", "foobar"}}));

	MessageHub hub;
	DummyActor a{&hub};
	Message m6{"c", "test4"};
	DummyActor::setMessageFrom(m6, &a);
	Message m7 = m6.createTargetedReply("test5", QJsonArray());
	REQUIRE(m7.to() == m6.from());
	REQUIRE(m7.to() == &a);
	REQUIRE(m7.command() == "test5");
	REQUIRE(m7.channel() == "c");
	REQUIRE(m7.data() == QJsonArray());
}

TEST_CASE("message json (de-)serialization", "[Message]") {
	Message m1{"a", "test1"};
	REQUIRE(m1.toJson() == QJsonObject({
										   {"id", Json::toJson(m1.id())},
										   {"ch", m1.channel()},
										   {"cmd", m1.command()}
									   }));

	Message m2{"b", "test2", QJsonArray({"asdf"})};
	REQUIRE(m2.toJson() == QJsonObject({
										   {"id", Json::toJson(m2.id())},
										   {"ch", "b"},
										   {"cmd", "test2"},
										   {"data", QJsonArray({"asdf"})}
									   }));

	Message m3 = m2.createReply("test3", QJsonValue());
	REQUIRE(m3.toJson() == QJsonObject({
										   {"id", Json::toJson(m3.id())},
										   {"ch", "b"},
										   {"cmd", "test3"},
										   {"reply", Json::toJson(m2.id())}
									   }));

	REQUIRE(m1.toJson() == Message::fromJson(m1.toJson()).toJson());
	REQUIRE(m2.toJson() == Message::fromJson(m2.toJson()).toJson());
	REQUIRE(m3.toJson() == Message::fromJson(m3.toJson()).toJson());
}
