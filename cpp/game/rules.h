#ifndef GAME_RULES_H_
#define GAME_RULES_H_

#include "../core/global.h"
#include "../core/hash.h"

#include "../external/nlohmann_json/json.hpp"

struct Rules {

  static const int KO_POSITIONAL = 1;
  static const int KO_SITUATIONAL = 2;
  int koRule;


  static const int TAX_NONE = 0;
  static const int TAX_SEKI = 1;
  static const int TAX_ALL = 2;
  int taxRule;

  bool multiStoneSuicideLegal;
  bool hasButton;


  //Mostly an informational value - doesn't affect the actual implemented rules, but GTP or Analysis may, at a
  //high level, use this info to adjust passing behavior - whether it's okay to pass without capturing dead stones.
  //Only relevant for area scoring.
  bool friendlyPassOk;

  float komi;
  //Min and max acceptable komi in various places involving user input validation
  static constexpr float MIN_USER_KOMI = -150.0f;
  static constexpr float MAX_USER_KOMI = 150.0f;

  Rules();
  Rules(
    int koRule,
    int taxRule,
    bool multiStoneSuicideLegal,
    bool hasButton,
    bool friendlyPassOk,
    float komi
  );
  ~Rules();

  bool operator==(const Rules& other) const;
  bool operator!=(const Rules& other) const;

  bool equalsIgnoringKomi(const Rules& other) const;
  bool gameResultWillBeInteger() const;

  static Rules getTrompTaylorish();

  static std::set<std::string> koRuleStrings();
  static std::set<std::string> taxRuleStrings();
  static int parseKoRule(const std::string& s);
  static int parseTaxRule(const std::string& s);
  static std::string writeKoRule(int koRule);
  static std::string writeTaxRule(int taxRule);

  static bool komiIsIntOrHalfInt(float komi);

  static Rules parseRules(const std::string& str);
  static Rules parseRulesWithoutKomi(const std::string& str, float komi);
  static bool tryParseRules(const std::string& str, Rules& buf);
  static bool tryParseRulesWithoutKomi(const std::string& str, Rules& buf, float komi);

  static Rules updateRules(const std::string& key, const std::string& value, Rules priorRules);

  friend std::ostream& operator<<(std::ostream& out, const Rules& rules);
  std::string toString() const;
  std::string toStringNoKomi() const;
  std::string toStringNoKomiMaybeNice() const;
  std::string toJsonString() const;
  std::string toJsonStringNoKomi() const;
  std::string toJsonStringNoKomiMaybeOmitStuff() const;
  nlohmann::json toJson() const;
  nlohmann::json toJsonNoKomi() const;
  nlohmann::json toJsonNoKomiMaybeOmitStuff() const;

  static const Hash128 ZOBRIST_KO_RULE_HASH[4];
  static const Hash128 ZOBRIST_TAX_RULE_HASH[3];
  static const Hash128 ZOBRIST_MULTI_STONE_SUICIDE_HASH;
  static const Hash128 ZOBRIST_BUTTON_HASH;

private:
  nlohmann::json toJsonHelper(bool omitKomi, bool omitDefaults) const;
};

#endif  // GAME_RULES_H_
