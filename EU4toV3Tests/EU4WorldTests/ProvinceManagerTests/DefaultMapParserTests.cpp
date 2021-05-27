#include "ProvinceManager/DefaultMapParser.h"
#include "gtest/gtest.h"
#include <gmock/gmock-matchers.h>
using testing::UnorderedElementsAre;

TEST(EU4World_DefaultMapParserTests, primitivesDefaultToBlank)
{
	std::stringstream input;
	EU4::DefaultMapParser defaults;
	defaults.loadDefaultMap(input);

	EXPECT_TRUE(defaults.getSeaIDs().empty());
	EXPECT_TRUE(defaults.getRandomsIDs().empty());
	EXPECT_TRUE(defaults.getLakeIDs().empty());
}

TEST(EU4World_DefaultMapParserTests, seasCanBeLoaded)
{
	std::stringstream input;
	input << "sea_starts = { 1 2 3 }";
	EU4::DefaultMapParser defaults;
	defaults.loadDefaultMap(input);

	EXPECT_THAT(defaults.getSeaIDs(), UnorderedElementsAre(1, 2, 3));
}

TEST(EU4World_DefaultMapParserTests, randomsCanBeLoaded)
{
	std::stringstream input;
	input << "only_used_for_random = { 1 2 3 }";
	EU4::DefaultMapParser defaults;
	defaults.loadDefaultMap(input);

	EXPECT_THAT(defaults.getRandomsIDs(), UnorderedElementsAre(1, 2, 3));
}

TEST(EU4World_DefaultMapParserTests, lakesCanBeLoaded)
{
	std::stringstream input;
	input << "lakes = { 1 2 3 }";
	EU4::DefaultMapParser defaults;
	defaults.loadDefaultMap(input);

	EXPECT_THAT(defaults.getLakeIDs(), UnorderedElementsAre(1, 2, 3));
}
