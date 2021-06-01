#include "CountryManager/EU4CountryManager.h"
#include "gtest/gtest.h"
#include <gmock/gmock-matchers.h>
using testing::UnorderedElementsAre;

TEST(EU4World_CountryManagerTests, primitivesDefaultToBlank)
{
	const EU4::CountryManager manager;

	EXPECT_TRUE(manager.getCountries().empty());
}

TEST(EU4World_CountryManagerTests, countriesCanBeLoaded)
{
	std::stringstream input;
	input << "AAA = {}\n";
	input << "BBB = {}\n";
	EU4::CountryManager manager;
	manager.loadCountries(input);

	EXPECT_EQ(2, manager.getCountries().size());
	EXPECT_TRUE(manager.getCountries().contains("AAA"));
	EXPECT_TRUE(manager.getCountries().contains("BBB"));
}

TEST(EU4World_CountryManagerTests, SpecialCountriesAreIgnored)
{
	std::stringstream input;
	input << "--- = {}\n";
	input << "REB = {}\n";
	input << "PIR = {}\n";
	input << "NAT = {}\n";
	input << "AAA = {}\n";
	input << "BBB = {}\n";
	EU4::CountryManager manager;
	manager.loadCountries(input);

	EXPECT_EQ(2, manager.getCountries().size());
	EXPECT_TRUE(manager.getCountries().contains("AAA"));
	EXPECT_TRUE(manager.getCountries().contains("BBB"));
}

TEST(EU4World_CountryManagerTests, CountriesCanBeRetrieved)
{
	std::stringstream input;
	input << "AAA = {}\n";
	input << "BBB = {}\n";
	EU4::CountryManager manager;
	manager.loadCountries(input);

	EXPECT_EQ("AAA", manager.getCountry("AAA")->getTag());
	EXPECT_EQ("BBB", manager.getCountry("BBB")->getTag());
}

TEST(EU4World_CountryManagerTests, CountryMismatchReturnsNullptr)
{
	std::stringstream input;
	EU4::CountryManager manager;
	manager.loadCountries(input);

	EXPECT_EQ(nullptr, manager.getCountry("AAA"));
}

TEST(EU4World_CountryManagerTests, LinkingProvincesToOwnersWorks)
{
	std::stringstream provincesInput;
	provincesInput << "-1 = { owner=TAG }\n";
	provincesInput << "-2 = { owner=TAG }\n";
	provincesInput << "-3 = { owner=GAT }\n";
	provincesInput << "-4 = { }\n";				// unowned
	provincesInput << "-5 = { owner=BLA }\n"; // nonsense
	EU4::ProvinceManager provinceManager;
	provinceManager.loadProvinces(provincesInput);

	std::stringstream countryManagerInput;
	countryManagerInput << "TAG = {}\n";
	countryManagerInput << "GAT = {}\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);

	manager.linkProvincesToCountries(provinceManager);

	const auto& tag = manager.getCountry("TAG");
	const auto& gat = manager.getCountry("GAT");

	EXPECT_EQ(2, tag->getProvinces().size());
	EXPECT_EQ(1, tag->getProvinces()[0]->getID());
	EXPECT_EQ(2, tag->getProvinces()[1]->getID());

	EXPECT_EQ(1, gat->getProvinces().size());
	EXPECT_EQ(3, gat->getProvinces()[0]->getID());
}

TEST(EU4World_CountryManagerTests, LinkingProvincesToCoreOwnersWorks)
{
	std::stringstream provincesInput;
	provincesInput << "-1 = { cores = { TAG GAT } }\n";
	provincesInput << "-2 = { cores = { TAG } }\n";
	provincesInput << "-3 = { cores = { GAT } }\n";
	provincesInput << "-4 = { }\n";						// no cores
	provincesInput << "-5 = { cores = { BLA } }\n"; // nonsense
	EU4::ProvinceManager provinceManager;
	provinceManager.loadProvinces(provincesInput);

	std::stringstream countryManagerInput;
	countryManagerInput << "TAG = {}\n";
	countryManagerInput << "GAT = {}\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);

	manager.linkProvincesToCountries(provinceManager);

	const auto& tag = manager.getCountry("TAG");
	const auto& gat = manager.getCountry("GAT");

	EXPECT_EQ(2, tag->getCores().size());
	EXPECT_EQ(1, tag->getCores()[0]->getID());
	EXPECT_EQ(2, tag->getCores()[1]->getID());

	EXPECT_EQ(2, gat->getCores().size());
	EXPECT_EQ(1, gat->getCores()[0]->getID());
	EXPECT_EQ(3, gat->getCores()[1]->getID());
}

TEST(EU4World_CountryManagerTests, HREStatusInheritedFromCapitalProvince)
{
	std::stringstream provincesInput;
	provincesInput << "-1 = { hre = yes } }\n";
	provincesInput << "-2 = { }\n";
	EU4::ProvinceManager provinceManager;
	provinceManager.loadProvinces(provincesInput);

	std::stringstream countryManagerInput;
	countryManagerInput << "TAG = { capital = 1 }\n";
	countryManagerInput << "GAT = { capital = 2 }\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);

	manager.setHREAndEmperors("", "", provinceManager);

	const auto& tag = manager.getCountry("TAG");
	const auto& gat = manager.getCountry("GAT");

	EXPECT_TRUE(tag->isInHRE());
	EXPECT_FALSE(gat->isInHRE());
}

TEST(EU4World_CountryManagerTests, EmperorshipsAreForwardedToCountries)
{
	const EU4::ProvinceManager provinceManager;

	std::stringstream countryManagerInput;
	countryManagerInput << "TAG = { }\n";
	countryManagerInput << "GAT = { }\n";
	countryManagerInput << "AAA = { }\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);

	manager.setHREAndEmperors("TAG", "GAT", provinceManager);

	const auto& tag = manager.getCountry("TAG");
	const auto& gat = manager.getCountry("GAT");
	const auto& aaa = manager.getCountry("AAA");

	EXPECT_TRUE(tag->isHREmperor());
	EXPECT_FALSE(tag->isCelestialEmperor());
	EXPECT_FALSE(gat->isHREmperor());
	EXPECT_TRUE(gat->isCelestialEmperor());
	EXPECT_FALSE(aaa->isHREmperor());
	EXPECT_FALSE(aaa->isCelestialEmperor());
}

TEST(EU4World_CountryManagerTests, RevolutionaryIsForwardedToCountries)
{
	std::stringstream countryManagerInput;
	countryManagerInput << "TAG = { }\n";
	countryManagerInput << "GAT = { }\n";
	countryManagerInput << "AAA = { }\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);

	manager.setRevolutionTarget("GAT");

	const auto& tag = manager.getCountry("TAG");
	const auto& gat = manager.getCountry("GAT");
	const auto& aaa = manager.getCountry("AAA");

	EXPECT_FALSE(tag->isRevolutionary());
	EXPECT_TRUE(gat->isRevolutionary());
	EXPECT_FALSE(aaa->isRevolutionary());
}

TEST(EU4World_CountryManagerTests, JapanCanBeUnited)
{
	std::stringstream provincesInput;
	provincesInput << "-1 = { owner = AAA cores={ AAA } }\n";
	provincesInput << "-2 = { owner = BBB cores={ BBB } }\n";
	provincesInput << "-3 = { owner = CCC cores={ CCC } }\n";
	provincesInput << "-4 = { owner = DDD cores={ DDD } }\n";
	EU4::ProvinceManager provinceManager;
	provinceManager.loadProvinces(provincesInput);

	std::stringstream nationMergeInput;
	nationMergeInput << "merge_daimyos = yes";
	mappers::NationMergeMapper mapper;
	mapper.loadNationMerge(nationMergeInput);

	std::stringstream countryManagerInput;
	countryManagerInput << "AAA = { government = { reform_stack={ reforms={ daimyo } } } }\n";
	countryManagerInput << "BBB = { government = { reform_stack={ reforms={ indep_daimyo } } } }\n";
	countryManagerInput << "CCC = { government = { reform_stack={ reforms={ shogunate } } } }\n";
	countryManagerInput << "DDD = { government = { reform_stack={ reforms={ plutocracy } } } }\n"; // control group.
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);
	manager.linkProvincesToCountries(provinceManager);
	manager.loadNationMergeMapper(mapper);
	manager.mergeNations();

	EXPECT_THAT(provinceManager.getProvince(1)->getCores(), UnorderedElementsAre("CCC"));
	EXPECT_THAT(provinceManager.getProvince(2)->getCores(), UnorderedElementsAre("CCC"));
	EXPECT_THAT(provinceManager.getProvince(3)->getCores(), UnorderedElementsAre("CCC"));
	EXPECT_THAT(provinceManager.getProvince(4)->getCores(), UnorderedElementsAre("DDD"));
	EXPECT_EQ("CCC", provinceManager.getProvince(1)->getOwnerTag());
	EXPECT_EQ("CCC", provinceManager.getProvince(2)->getOwnerTag());
	EXPECT_EQ("CCC", provinceManager.getProvince(3)->getOwnerTag());
	EXPECT_EQ("DDD", provinceManager.getProvince(4)->getOwnerTag());
}

TEST(EU4World_CountryManagerTests, NationsCanBeMerged)
{
	std::stringstream provincesInput;
	provincesInput << "-1 = { owner = AAA cores={ AAA } }\n";
	provincesInput << "-2 = { owner = BBB cores={ BBB } }\n";
	provincesInput << "-3 = { owner = CCC cores={ CCC } }\n";
	provincesInput << "-4 = { owner = DDD cores={ DDD } }\n";
	EU4::ProvinceManager provinceManager;
	provinceManager.loadProvinces(provincesInput);

	std::stringstream nationMergeInput;
	nationMergeInput << "test = {\n";
	nationMergeInput << " merge = yes\n";
	nationMergeInput << " master = BBB\n";
	nationMergeInput << " slave = AAA\n";
	nationMergeInput << " slave = DDD\n";
	nationMergeInput << "}\n";
	mappers::NationMergeMapper mapper;
	mapper.loadNationMerge(nationMergeInput);

	std::stringstream countryManagerInput;
	countryManagerInput << "AAA = { }\n";
	countryManagerInput << "BBB = { }\n";
	countryManagerInput << "CCC = { }\n"; // control group.
	countryManagerInput << "DDD = { }\n";
	EU4::CountryManager manager;
	manager.loadCountries(countryManagerInput);
	manager.linkProvincesToCountries(provinceManager);
	manager.loadNationMergeMapper(mapper);
	manager.mergeNations();

	EXPECT_THAT(provinceManager.getProvince(1)->getCores(), UnorderedElementsAre("BBB"));
	EXPECT_THAT(provinceManager.getProvince(2)->getCores(), UnorderedElementsAre("BBB"));
	EXPECT_THAT(provinceManager.getProvince(3)->getCores(), UnorderedElementsAre("CCC"));
	EXPECT_THAT(provinceManager.getProvince(4)->getCores(), UnorderedElementsAre("BBB"));
	EXPECT_EQ("BBB", provinceManager.getProvince(1)->getOwnerTag());
	EXPECT_EQ("BBB", provinceManager.getProvince(2)->getOwnerTag());
	EXPECT_EQ("CCC", provinceManager.getProvince(3)->getOwnerTag());
	EXPECT_EQ("BBB", provinceManager.getProvince(4)->getOwnerTag());
}
