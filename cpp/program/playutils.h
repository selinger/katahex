#ifndef PROGRAM_PLAY_UTILS_H_
#define PROGRAM_PLAY_UTILS_H_

#include "../core/config_parser.h"
#include "../program/play.h"
#include "../search/asyncbot.h"

//This is a grab-bag of various useful higher-level functions that select moves or evaluate the board in various ways.

namespace PlayUtils {
  ExtraBlackAndKomi chooseExtraBlackAndKomi(
    float base, float stdev, double allowIntegerProb, 
    double bigStdevProb, float bigStdev, double sqrtBoardArea, Rand& rand
  );
  void setKomiWithoutNoise(const ExtraBlackAndKomi& extraBlackAndKomi, BoardHistory& hist); //Also ignores allowInteger
  void setKomiWithNoise(const ExtraBlackAndKomi& extraBlackAndKomi, BoardHistory& hist, Rand& rand);

  ReportedSearchValues getWhiteScoreValues(
    Search* bot,
    const Board& board,
    const BoardHistory& hist,
    Player pla,
    int64_t numVisits,
    const OtherGameProperties& otherGameProps
  );

  Loc chooseRandomLegalMove(const Board& board, const BoardHistory& hist, Player pla, Rand& gameRand, Loc banMove);
  int chooseRandomLegalMoves(const Board& board, const BoardHistory& hist, Player pla, Rand& gameRand, Loc* buf, int len);

  Loc chooseRandomPolicyMove(
    const NNOutput* nnOutput,
    const Board& board,
    const BoardHistory& hist,
    Player pla,
    Rand& gameRand,
    double temperature,
    bool allowPass,
    Loc banMove
  );

  Loc getGameInitializationMove(
    Search* botB, Search* botW, Board& board, const BoardHistory& hist, Player pla, NNResultBuf& buf,
    Rand& gameRand, double temperature
  );
  void initializeGameUsingPolicy(
    Search* botB, Search* botW, Board& board, BoardHistory& hist, Player& pla,
    Rand& gameRand, bool doEndGameIfAllPassAlive,
    double proportionOfBoardArea, double temperature
  );

  float roundAndClipKomi(double unrounded, const Board& board, bool looseClipping);


  double getSearchFactor(
    double searchFactorWhenWinningThreshold,
    double searchFactorWhenWinning,
    const SearchParams& params,
    const std::vector<double>& recentWinLossValues,
    Player pla
  );

  void adjustKomiToEven(
    Search* botB,
    Search* botW, //can be NULL if only one bot
    const Board& board,
    BoardHistory& hist,
    Player pla,
    int64_t numVisits,
    const OtherGameProperties& otherGameProps,
    Rand& rand
  );

  double getHackedLCBForWinrate(const Search* search, const AnalysisData& data, Player pla);

  std::vector<double> computeOwnership(
    Search* bot,
    const Board& board,
    const BoardHistory& hist,
    Player pla,
    int64_t numVisits
  );


  struct BenchmarkResults {
    int numThreads = 0;
    int totalPositionsSearched = 0;
    int totalPositions = 0;
    int64_t totalVisits = 0;
    double totalSeconds = 0;
    int64_t numNNEvals = 0;
    int64_t numNNBatches = 0;
    double avgBatchSize = 0;

    std::string toStringNotDone() const;
    std::string toString() const;
    std::string toStringWithElo(const BenchmarkResults* baseline, double secondsPerGameMove) const;

    double computeEloEffect(double secondsPerGameMove) const;

    static void printEloComparison(const std::vector<BenchmarkResults>& results, double secondsPerGameMove);
  };

  //Run benchmark on sgf positions. ALSO prints to stdout the ongoing result as it benchmarks.
  BenchmarkResults benchmarkSearchOnPositionsAndPrint(
    const SearchParams& params,
    const CompactSgf* sgf,
    int numPositionsToUse,
    NNEvaluator* nnEval,
    const BenchmarkResults* baseline,
    double secondsPerGameMove,
    bool printElo
  );

  void printGenmoveLog(std::ostream& out, const AsyncBot* bot, const NNEvaluator* nnEval, Loc moveLoc, double timeTaken, Player perspective);

  Rules genRandomRules(Rand& rand);


}


#endif //PROGRAM_PLAY_UTILS_H_
