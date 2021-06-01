#include "WarParser/WarParser.h"
#include "gtest/gtest.h"

TEST(EU4World_WarParserTests, primitivesDefaultToDefaults)
{
	std::stringstream input;
	const EU4::WarParser war(input);

	EXPECT_TRUE(war.getAttackers().empty());
	EXPECT_TRUE(war.getDefenders().empty());
	EXPECT_TRUE(war.getName().empty());
	EXPECT_FALSE(war.getDetails().startDate.isSet());
	EXPECT_EQ(0, war.getDetails().targetProvinceID);
	EXPECT_TRUE(war.getDetails().targetTag.empty());
	EXPECT_TRUE(war.getDetails().warGoalClass.empty());
	EXPECT_TRUE(war.getDetails().warGoalType.empty());
}

TEST(EU4World_WarParserTests, warCanBeLoaded)
{
	std::stringstream input;
	input << "name = \"Silly War\"";
	input << "history = {\n";
	input << "	9.9.9 = { it begins }\n";
	input << "	10.10.10 = { it doesn't end }\n";
	input << "}\n";
	input << "attackers = { ULM }\n";
	input << "defenders = { FRA TUR HAB }\n";
	input << "take_province = {\n";
	input << "	province = 13\n";
	input << "	type = conquest\n";
	input << "	tag = FRA\n";
	input << "}\n";
	const EU4::WarParser war(input);

	EXPECT_EQ("ULM", war.getAttackers()[0]);
	EXPECT_EQ("FRA", war.getDefenders()[0]);
	EXPECT_EQ("TUR", war.getDefenders()[1]);
	EXPECT_EQ("HAB", war.getDefenders()[2]);
	EXPECT_EQ("Silly War", war.getName());
	EXPECT_EQ(date("9.9.9"), war.getDetails().startDate);
	EXPECT_EQ(13, war.getDetails().targetProvinceID);
	EXPECT_EQ("FRA", war.getDetails().targetTag);
	EXPECT_EQ("take_province", war.getDetails().warGoalClass);
	EXPECT_EQ("conquest", war.getDetails().warGoalType);
}
