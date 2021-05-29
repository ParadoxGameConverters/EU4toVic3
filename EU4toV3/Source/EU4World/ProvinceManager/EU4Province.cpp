#include "EU4Province.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include <cmath>

EU4::Province::Province(const std::string& numString, std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();

	provID = 0 - stoi(numString);

	// Provinces without starting culture (nativeless provinces) won't have all entries
	if (!provinceHistory.hasInitializedHistory() && !culture.empty() && !religion.empty())
	{
		provinceHistory.setStartingCulture(culture);
		provinceHistory.setStartingReligion(religion);
	}
}

void EU4::Province::registerKeys()
{
	registerSetter("name", name);
	registerSetter("culture", culture);
	registerSetter("religion", religion);
	registerSetter("base_tax", baseTax);
	registerSetter("base_production", baseProduction);
	registerSetter("base_manpower", baseManpower);
	registerSetter("owner", ownerTag);
	registerSetter("controller", controllerTag);
	registerKeyword("cores", [this](std::istream& theStream) {
		const auto& coreList = commonItems::getStrings(theStream);
		cores.insert(coreList.begin(), coreList.end());
	});
	registerKeyword("territorial_core", [this](std::istream& theStream) {
		commonItems::ignoreItem("unused", theStream);
		territorialCore = true;
	});
	registerKeyword("hre", [this](std::istream& theStream) {
		inHRE = commonItems::getString(theStream) == "yes";
	});
	registerKeyword("history", [this](std::istream& theStream) {
		const ProvinceHistory theHistory(theStream);
		provinceHistory = theHistory;
	});
	registerKeyword("buildings", [this](std::istream& theStream) {
		const ProvinceBuildings theBuildings(theStream);
		buildings = theBuildings.getBuildings();
	});
	registerKeyword("great_projects", [this](std::istream& theStream) {
		const auto& projectList = commonItems::getStrings(theStream);
		greatProjects.insert(projectList.begin(), projectList.end());
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void EU4::Province::determineProvinceWeight(const BuildingCostLoader& buildingTypes)
{
	// Uncolonized/unowned province is worth exactly zero.
	if (ownerTag.empty())
		return;

	auto buildingWeight = 0.0;
	for (const auto& buildingName: buildings)
	{
		const auto& buildingCost = buildingTypes.getBuildingCost(buildingName);
		if (buildingCost)
			buildingWeight += *buildingCost;
	}
	// This is the cost of all built buildings, scaled to 1% (every 100 gold in buildings = 1 dev).
	buildingWeight /= 100;

	// This is how much dev there is and how much was invested in the province.
	const auto currentDevelopment = baseTax + baseProduction + baseManpower;
	const auto developmentDelta = currentDevelopment - provinceHistory.getOriginalDevelopment();

	// Province weight is absolute dev + buildings. It is used in extreme popShaping.
	provinceWeight = currentDevelopment + buildingWeight;

	// Investment weight is invested dev + buildings. Its FACTOR is used in devPush popShaping.
	const auto investedWeight = developmentDelta + buildingWeight;

	if (investedWeight > 0)
	{
		// provinces with modifier weights under 10 (under-invested with no buildings) get a penalty for popShaping, (realistically) up to -10.
		investmentFactor = (std::log10(investedWeight) - 1) * 10;
	}
	else
	{
		investmentFactor = -10 + investedWeight / 10; // For negatives, go linearly into debt.
	}
}
