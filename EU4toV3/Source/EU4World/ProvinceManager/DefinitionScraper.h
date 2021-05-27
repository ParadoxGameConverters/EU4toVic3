#ifndef DEFINITION_SCRAPER
#define DEFINITION_SCRAPER
#include <set>
#include <string>
#include "Mods/ModLoader.h"
#include <optional>

namespace EU4
{
class DefinitionScraper
{
  public:
	void loadDefinitions(const std::string& EU4Path, const Mods& mods);
	void loadDefinitions(std::istream& theStream);

	[[nodiscard]] const auto& getProvinceIDs() const { return provinceIDs; }

  private:
	void parseStream(std::istream& theStream);
	[[nodiscard]] std::optional<int> parseLine(const std::string& line);

	std::set<int> provinceIDs;
};

} // namespace EU4
#endif // DEFINITION_SCRAPER
