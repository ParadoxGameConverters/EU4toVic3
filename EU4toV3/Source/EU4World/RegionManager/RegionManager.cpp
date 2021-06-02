#include "RegionManager.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include "ParserHelpers.h"
#include "ProvinceManager/ProvinceManager.h"
#include <filesystem>
#include <fstream>
#include <ranges>
namespace fs = std::filesystem;

void EU4::RegionManager::loadRegions(const std::string& EU4Path, const Mods& mods)
{
	auto areaFilename = EU4Path + "/map/area.txt";
	auto regionFilename = EU4Path + "/map/region.txt";
	auto superRegionFilename = EU4Path + "/map/superregion.txt";

	for (const auto& [modName, modPath]: mods)
		if (commonItems::DoesFileExist(modPath + "/map/area.txt"))
		{
			Log(LogLevel::Info) << "-> Loading areas from mod: " << modName;
			areaFilename = modPath + "/map/area.txt";
			break;
		}

	for (const auto& [modName, modPath]: mods)
		if (commonItems::DoesFileExist(modPath + "/map/region.txt"))
		{
			Log(LogLevel::Info) << "-> Loading regions from mod: " << modName;
			areaFilename = modPath + "/map/region.txt";
			break;
		}

	for (const auto& [modName, modPath]: mods)
		if (commonItems::DoesFileExist(modPath + "/map/superregion.txt"))
		{
			Log(LogLevel::Info) << "-> Loading superregions from mod: " << modName;
			areaFilename = modPath + "/map/superregion.txt";
			break;
		}

	std::ifstream areaStream(fs::u8path(areaFilename));
	if (!areaStream.is_open())
		throw std::runtime_error("Could not open map/area.txt!");
	registerAreaKeys();
	parseStream(areaStream);
	clearRegisteredKeywords();
	areaStream.close();

	std::ifstream superRegionStream(fs::u8path(superRegionFilename));
	if (!superRegionStream.is_open())
		throw std::runtime_error("Could not open map/superregion.txt!");
	registerSuperRegionKeys();
	parseStream(superRegionStream);
	clearRegisteredKeywords();
	superRegionStream.close();

	std::ifstream regionStream(fs::u8path(regionFilename));
	if (!regionStream.is_open())
		throw std::runtime_error("Could not open map/region.txt!");
	registerRegionKeys();
	parseStream(regionStream);
	clearRegisteredKeywords();
	regionStream.close();

	linkSuperRegions();
	linkRegions();
	superGroupMapper.loadSuperGroups();
}

void EU4::RegionManager::loadRegions(std::istream& areaStream, std::istream& regionStream, std::istream& superRegionStream)
{
	registerAreaKeys();
	parseStream(areaStream);
	clearRegisteredKeywords();

	registerSuperRegionKeys();
	parseStream(superRegionStream);
	clearRegisteredKeywords();

	registerRegionKeys();
	parseStream(regionStream);
	clearRegisteredKeywords();

	linkSuperRegions();
	linkRegions();
	// load supergroups manually with loadSuperGroups() and then applySuperGroups() when testing.
}

void EU4::RegionManager::registerAreaKeys()
{
	registerRegex(R"([\w_]+)", [this](const std::string& areaName, std::istream& theStream) {
		areas.emplace(areaName, std::make_shared<Area>(theStream));
	});
}

void EU4::RegionManager::registerSuperRegionKeys()
{
	registerRegex(R"([\w_]+)", [this](const std::string& superRegionName, std::istream& theStream) {
		const commonItems::stringList theList(theStream);
		std::vector<std::string> tempRegions;
		for (const auto& region: theList.getStrings())
		{
			if (region == "restrict_charter")
				continue;
			tempRegions.emplace_back(region);
		}
		superRegions.emplace(superRegionName, std::make_shared<SuperRegion>(tempRegions));
	});
}

void EU4::RegionManager::registerRegionKeys()
{
	registerRegex(R"([\w_]+)", [this](const std::string& regionName, std::istream& theStream) {
		regions.emplace(regionName, std::make_shared<Region>(theStream));
	});
}


bool EU4::RegionManager::provinceIsInRegion(int provinceID, const std::string& regionName) const
{
	if (const auto& regionItr = regions.find(regionName); regionItr != regions.end())
		return regionItr->second->regionContainsProvince(provinceID);

	// "Regions" are such a fluid term.
	if (const auto& superRegionItr = superRegions.find(regionName); superRegionItr != superRegions.end())
		return superRegionItr->second->superRegionContainsProvince(provinceID);

	// And sometimes they don't mean what people think they mean at all.
	if (const auto& areaItr = areas.find(regionName); areaItr != areas.end())
		return areaItr->second->areaContainsProvince(provinceID);

	return false;
}

std::optional<std::string> EU4::RegionManager::getParentAreaName(const int provinceID) const
{
	for (const auto& [areaName, area]: areas)
		if (area->areaContainsProvince(provinceID))
			return areaName;

	Log(LogLevel::Warning) << "Province ID " << provinceID << " has no parent area name!";
	return std::nullopt;
}

std::optional<std::string> EU4::RegionManager::getParentRegionName(const int provinceID) const
{
	for (const auto& [regionName, region]: regions)
		if (region->regionContainsProvince(provinceID))
			return regionName;

	Log(LogLevel::Warning) << "Province ID " << provinceID << " has no parent region name!";
	return std::nullopt;
}

std::optional<std::string> EU4::RegionManager::getParentSuperRegionName(const int provinceID) const
{
	for (const auto& [superRegionName, superRegion]: superRegions)
		if (superRegion->superRegionContainsProvince(provinceID))
			return superRegionName;

	Log(LogLevel::Warning) << "Province ID " << provinceID << " has no parent superregion name!";
	return std::nullopt;
}

std::optional<std::string> EU4::RegionManager::getParentSuperGroupName(const int provinceID) const
{
	for (const auto& superRegion: superRegions | std::views::values)
		if (superRegion->superRegionContainsProvince(provinceID))
			return superRegion->getSuperGroup();

	Log(LogLevel::Warning) << "Province ID " << provinceID << " has no parent supergroup name!";
	return std::nullopt;
}

bool EU4::RegionManager::regionNameIsValid(const std::string& regionName) const
{
	const auto& regionItr = regions.find(regionName);
	if (regionItr != regions.end())
		return true;

	// Who knows what the mapper needs. All kinds of stuff.
	const auto& superRegionItr = superRegions.find(regionName);
	if (superRegionItr != superRegions.end())
		return true;

	// And more stuff, what's the worst that could happen?
	const auto& areaItr = areas.find(regionName);
	if (areaItr != areas.end())
		return true;

	return false;
}

void EU4::RegionManager::linkSuperRegions()
{
	for (const auto& [superRegionName, superRegion]: superRegions)
	{
		const auto& requiredRegions = superRegion->getRegions();
		for (const auto& requiredRegionName: requiredRegions | std::views::keys)
		{
			const auto& regionItr = regions.find(requiredRegionName);
			if (regionItr != regions.end())
			{
				superRegion->linkRegion(std::pair(regionItr->first, regionItr->second));
			}
			else
			{
				throw std::runtime_error("Superregion's " + superRegionName + " region " + requiredRegionName + " does not exist!");
			}
		}
	}
}

void EU4::RegionManager::linkRegions()
{
	for (const auto& [regionName, region]: regions)
	{
		const auto& requiredAreas = region->getAreas();
		for (const auto& requiredAreaName: requiredAreas | std::views::keys)
		{
			const auto& areaItr = areas.find(requiredAreaName);
			if (areaItr != areas.end())
			{
				region->linkArea(std::pair(areaItr->first, areaItr->second));
			}
			else
			{
				throw std::runtime_error("Region's " + regionName + " area " + requiredAreaName + " does not exist!");
			}
		}
	}
}

bool EU4::RegionManager::provinceIsValid(int provinceID) const
{
	for (const auto& area: areas | std::views::values)
		if (area->areaContainsProvince(provinceID))
			return true;
	return false;
}

void EU4::RegionManager::applySuperGroups()
{
	for (const auto& [superRegionName, superRegion]: superRegions)
	{
		superRegion->setAssimilationFactor(superGroupMapper.getAssimilationFactor(superRegionName));
		const auto& superGroup = superGroupMapper.getGroupForSuperRegion(superRegionName);
		if (superGroup)
		{
			superRegion->setSuperGroup(*superGroup);
		}
		else
		{
			Log(LogLevel::Warning) << "Superregion " << superRegionName << " doesn't have a supergroup in world_supergroups.txt!";
			superRegion->setSuperGroup("old_world"); // defaulting to the safe choice.
		}
	}
}

std::optional<double> EU4::RegionManager::getAssimilationFactor(int provinceID) const
{
	for (const auto& superRegion: superRegions | std::views::values)
		if (superRegion->superRegionContainsProvince(provinceID))
			return superRegion->getAssimilationFactor();

	Log(LogLevel::Warning) << "Province ID " << provinceID << " has no assimilation factor!";
	return std::nullopt;
}

std::optional<std::string> EU4::RegionManager::getColonialRegionForProvince(int province) const
{
	return colonialRegionLoader.getColonialRegionForProvince(province);
}

void EU4::RegionManager::catalogueNativeCultures(const ProvinceManager& provinceManager)
{
	for (const auto& [provinceID, province]: provinceManager.getAllProvinces())
	{
		if (province->getStartingCulture().empty())
			continue;
		const auto& superRegionName = getParentSuperRegionName(provinceID);
		if (superRegionName)
			superRegions.at(*superRegionName)->registerNativeCulture(province->getStartingCulture());
	}
}

bool EU4::RegionManager::doesProvinceRequireNeoCulture(int provinceID, const std::string& culture) const
{
	// This one is funny. A province requires a neoculture if:
	// 1. it belongs to a colonial region
	// 2. the culture given (presumably from that very province) is not native to the province's superRegion.
	// result of this function fuels generation of a new neoculture in cultureManager.

	if (!getColonialRegionForProvince(provinceID))
		return false; // not in colonial region.

	const auto& superRegionName = getParentSuperRegionName(provinceID);
	if (superRegionName)
		return !superRegions.at(*superRegionName)->superRegionContainsNativeCulture(culture);

	return false;
}
