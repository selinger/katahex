#include "../game/board.h"

/*
 * board.cpp
 * Originally from an unreleased project back in 2010, modified since.
 * Authors: brettharrison (original), David Wu (original and later modificationss).
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "../core/rand.h"

using namespace std;

//STATIC VARS-----------------------------------------------------------------------------
bool Board::IS_ZOBRIST_INITALIZED = false;
Hash128 Board::ZOBRIST_SIZE_X_HASH[MAX_LEN+1];
Hash128 Board::ZOBRIST_SIZE_Y_HASH[MAX_LEN+1];
Hash128 Board::ZOBRIST_BOARD_HASH[MAX_ARR_SIZE][4];
Hash128 Board::ZOBRIST_PLAYER_HASH[4];
Hash128 Board::ZOBRIST_KO_LOC_HASH[MAX_ARR_SIZE];
Hash128 Board::ZOBRIST_KO_MARK_HASH[MAX_ARR_SIZE][4];
Hash128 Board::ZOBRIST_BOARD_HASH2[MAX_ARR_SIZE][4];
Hash128 Board::ZOBRIST_CAPTURE_B_HASH[2*MAX_ARR_SIZE];
Hash128 Board::ZOBRIST_CAPTURE_W_HASH[2*MAX_ARR_SIZE];
Hash128 Board::ZOBRIST_PASSNUM_B_HASH[2*MAX_ARR_SIZE];
Hash128 Board::ZOBRIST_PASSNUM_W_HASH[2*MAX_ARR_SIZE];
const Hash128 Board::ZOBRIST_GAME_IS_OVER = //Based on sha256 hash of Board::ZOBRIST_GAME_IS_OVER
  Hash128(0xb6f9e465597a77eeULL, 0xf1d583d960a4ce7fULL);

//LOCATION--------------------------------------------------------------------------------
Loc Location::getLoc(int x, int y, int x_size)
{
  return (x+1) + (y+1)*(x_size+1);
}
int Location::getX(Loc loc, int x_size)
{
  return (loc % (x_size+1)) - 1;
}
int Location::getY(Loc loc, int x_size)
{
  return (loc / (x_size+1)) - 1;
}
void Location::getAdjacentOffsets(short adj_offsets[8], int x_size)
{
  adj_offsets[0] = -(x_size+1);
  adj_offsets[1] = -1;
  adj_offsets[2] = 1;
  adj_offsets[3] = (x_size+1);
  adj_offsets[4] = (x_size+1)-1;
  adj_offsets[5] = -(x_size+1)+1;
  adj_offsets[6] = -(x_size+1)-1;
  adj_offsets[7] = (x_size+1)+1;
}

bool Location::isAdjacent(Loc loc0, Loc loc1, int x_size)
{
  return loc0 == loc1 - (x_size+1) || loc0 == loc1 - 1 || loc0 == loc1 + 1 || loc0 == loc1 + (x_size+1)|| loc0 == loc1 + (x_size+1)-1|| loc0 == loc1 - (x_size+1)+1;
}

Loc Location::getMirrorLoc(Loc loc, int x_size, int y_size) {
  if(loc == Board::NULL_LOC || loc == Board::PASS_LOC)
    return loc;
  return getLoc(x_size-1-getX(loc,x_size),y_size-1-getY(loc,x_size),x_size);
}

Loc Location::getCenterLoc(int x_size, int y_size) {
  if(x_size % 2 == 0 || y_size % 2 == 0)
    return Board::NULL_LOC;
  return getLoc(x_size / 2, y_size / 2, x_size);
}

Loc Location::getCenterLoc(const Board& b) {
  return getCenterLoc(b.x_size,b.y_size);
}

bool Location::isCentral(Loc loc, int x_size, int y_size) {
  int x = getX(loc,x_size);
  int y = getY(loc,x_size);
  return x >= (x_size-1)/2 && x <= x_size/2 && y >= (y_size-1)/2 && y <= y_size/2;
}

bool Location::isNearCentral(Loc loc, int x_size, int y_size) {
  int x = getX(loc,x_size);
  int y = getY(loc,x_size);
  return x >= (x_size-1)/2-1 && x <= x_size/2+1 && y >= (y_size-1)/2-1 && y <= y_size/2+1;
}


#define FOREACHADJ(BLOCK) {int ADJOFFSET = -(x_size+1); {BLOCK}; ADJOFFSET = -1; {BLOCK}; ADJOFFSET = 1; {BLOCK}; ADJOFFSET = x_size+1; {BLOCK}; ADJOFFSET = x_size+1-1; {BLOCK}; ADJOFFSET = -(x_size+1)+1; {BLOCK}};
#define ADJ0 (-(x_size+1))
#define ADJ1 (-1)
#define ADJ2 (1)
#define ADJ3 (x_size+1)
#define ADJ4 ((x_size+1)-1)
#define ADJ5 (-(x_size+1)+1)

//CONSTRUCTORS AND INITIALIZATION----------------------------------------------------------

Board::Board()
{
  init(DEFAULT_LEN,DEFAULT_LEN);
}

Board::Board(int x, int y)
{
  init(x,y);
}


Board::Board(const Board& other)
{
  x_size = other.x_size;
  y_size = other.y_size;

  memcpy(colors, other.colors, sizeof(Color)*MAX_ARR_SIZE);
  memcpy(chain_data, other.chain_data, sizeof(ChainData)*MAX_ARR_SIZE);
  memcpy(chain_head, other.chain_head, sizeof(Loc)*MAX_ARR_SIZE);
  memcpy(next_in_chain, other.next_in_chain, sizeof(Loc)*MAX_ARR_SIZE);

  ko_loc = other.ko_loc;
  // empty_list = other.empty_list;
  pos_hash = other.pos_hash;
  numBlackCaptures = other.numBlackCaptures;
  numWhiteCaptures = other.numWhiteCaptures;
  numBlackPasses = other.numBlackPasses;
  numWhitePasses = other.numWhitePasses;

  memcpy(adj_offsets, other.adj_offsets, sizeof(short)*8);
}

void Board::init(int xS, int yS)
{
  assert(IS_ZOBRIST_INITALIZED);
  if(xS < 0 || yS < 0 || xS > MAX_LEN || yS > MAX_LEN)
    throw StringError("Board::init - invalid board size");

  x_size = xS;
  y_size = yS;

  for(int i = 0; i < MAX_ARR_SIZE; i++)
    colors[i] = C_WALL;

  for(int y = 0; y < y_size; y++)
  {
    for(int x = 0; x < x_size; x++)
    {
      Loc loc = (x+1) + (y+1)*(x_size+1);
      colors[loc] = C_EMPTY;
      // empty_list.add(loc);
    }
  }

  ko_loc = NULL_LOC;
  pos_hash = ZOBRIST_SIZE_X_HASH[x_size] ^ ZOBRIST_SIZE_Y_HASH[y_size];
  numBlackCaptures = 0;
  numWhiteCaptures = 0;
  numBlackPasses = 0;
  numWhitePasses = 0;

  Location::getAdjacentOffsets(adj_offsets,x_size);
}

void Board::initHash()
{
  if(IS_ZOBRIST_INITALIZED)
    return;
  Rand rand("Board::initHash()");

  auto nextHash = [&rand]() {
    uint64_t h0 = rand.nextUInt64();
    uint64_t h1 = rand.nextUInt64();
    return Hash128(h0,h1);
  };

  for(int i = 0; i<4; i++)
    ZOBRIST_PLAYER_HASH[i] = nextHash();

  //afffected by the size of the board we compile with.
  for(int i = 0; i<MAX_ARR_SIZE; i++) {
    for(Color j = 0; j<4; j++) {
      if(j == C_EMPTY || j == C_WALL)
        ZOBRIST_BOARD_HASH[i][j] = Hash128();
      else
        ZOBRIST_BOARD_HASH[i][j] = nextHash();

      if(j == C_EMPTY || j == C_WALL)
        ZOBRIST_KO_MARK_HASH[i][j] = Hash128();
      else
        ZOBRIST_KO_MARK_HASH[i][j] = nextHash();
    }
    ZOBRIST_KO_LOC_HASH[i] = nextHash();
  }

  for(int i=0;i<2*MAX_ARR_SIZE;i++)
  {
    ZOBRIST_CAPTURE_B_HASH[i]=nextHash();
    ZOBRIST_CAPTURE_W_HASH[i]=nextHash();
    ZOBRIST_PASSNUM_B_HASH[i]=nextHash();
    ZOBRIST_PASSNUM_W_HASH[i]=nextHash();
  }
  ZOBRIST_CAPTURE_B_HASH[0]=Hash128();
  ZOBRIST_CAPTURE_W_HASH[0]=Hash128();
  ZOBRIST_PASSNUM_B_HASH[0]=Hash128();
  ZOBRIST_PASSNUM_W_HASH[0]=Hash128();


  //Reseed the random number generator so that these size hashes are also
  //not affected by the size of the board we compile with
  rand.init("Board::initHash() for ZOBRIST_SIZE hashes");
  for(int i = 0; i<MAX_LEN+1; i++) {
    ZOBRIST_SIZE_X_HASH[i] = nextHash();
    ZOBRIST_SIZE_Y_HASH[i] = nextHash();
  }

  //Reseed and compute one more set of zobrist hashes, mixed a bit differently
  rand.init("Board::initHash() for second set of ZOBRIST hashes");
  for(int i = 0; i<MAX_ARR_SIZE; i++) {
    for(Color j = 0; j<4; j++) {
      ZOBRIST_BOARD_HASH2[i][j] = nextHash();
      ZOBRIST_BOARD_HASH2[i][j].hash0 = Hash::murmurMix(ZOBRIST_BOARD_HASH2[i][j].hash0);
      ZOBRIST_BOARD_HASH2[i][j].hash1 = Hash::splitMix64(ZOBRIST_BOARD_HASH2[i][j].hash1);
    }
  }

  IS_ZOBRIST_INITALIZED = true;
}

Hash128 Board::getSitHashWithSimpleKo(Player pla) const {
  Hash128 h = pos_hash;
  if(ko_loc != Board::NULL_LOC)
    h = h ^ Board::ZOBRIST_KO_LOC_HASH[ko_loc];
  h ^= Board::ZOBRIST_PLAYER_HASH[pla];
  return h;
}

void Board::clearSimpleKoLoc() {
  ko_loc = NULL_LOC;
}
void Board::setSimpleKoLoc(Loc loc) {
  ko_loc = loc;
}


//Gets the number of stones of the chain at loc. Precondition: location must be black or white.
int Board::getChainSize(Loc loc) const
{
  return chain_data[chain_head[loc]].num_locs;
}

//Gets the number of liberties of the chain at loc. Assertion: location must be black or white.
int Board::getNumLiberties(Loc loc) const
{
  return chain_data[chain_head[loc]].num_liberties;
}

//Check if moving here would be a self-capture
bool Board::isSuicide(Loc loc, Player pla) const
{
  if(loc == PASS_LOC)
    return false;

  Player opp = getOpp(pla);
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;

    if(colors[adj] == C_EMPTY)
      return false;
    else if(colors[adj] == pla)
    {
      if(getNumLiberties(adj) > 1)
        return false;
    }
    else if(colors[adj] == opp)
    {
      if(getNumLiberties(adj) == 1)
        return false;
    }
  );

  return true;
}

//Check if moving here is would be an illegal self-capture
bool Board::isIllegalSuicide(Loc loc, Player pla, bool isMultiStoneSuicideLegal) const
{
  Player opp = getOpp(pla);
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;

    if(colors[adj] == C_EMPTY)
      return false;
    else if(colors[adj] == pla)
    {
      if(isMultiStoneSuicideLegal || getNumLiberties(adj) > 1)
        return false;
    }
    else if(colors[adj] == opp)
    {
      if(getNumLiberties(adj) == 1)
        return false;
    }
  );

  return true;
}


//Returns the number of liberties a new stone placed here would have, or max if it would be >= max.
int Board::getNumLibertiesAfterPlay(Loc loc, Player pla, int max) const
{
  Player opp = getOpp(pla);

  int numLibs = 0;
  Loc libs[MAX_PLAY_SIZE];
  int numCapturedGroups = 0;
  Loc capturedGroupHeads[6];

  //First, count immediate liberties and groups that would be captured
  for(int i = 0; i < 6; i++) {
    Loc adj = loc + adj_offsets[i];
    if(colors[adj] == C_EMPTY) {
      libs[numLibs++] = adj;
      if(numLibs >= max)
        return max;
    }
    else if(colors[adj] == opp && getNumLiberties(adj) == 1) {
      libs[numLibs++] = adj;
      if(numLibs >= max)
        return max;

      Loc head = chain_head[adj];
      bool alreadyFound = false;
      for(int j = 0; j<numCapturedGroups; j++) {
        if(capturedGroupHeads[j] == head)
        {alreadyFound = true; break;}
      }
      if(!alreadyFound)
        capturedGroupHeads[numCapturedGroups++] = head;
    }
  }

  auto wouldBeEmpty = [numCapturedGroups,&capturedGroupHeads,this,opp](Loc lc) {
    if(this->colors[lc] == C_EMPTY)
      return true;
    if(this->colors[lc] == opp) {
      for(int i = 0; i<numCapturedGroups; i++)
        if(capturedGroupHeads[i] == this->chain_head[lc])
          return true;
    }
    return false;
  };

  //Next, walk through all stones of all surrounding groups we would connect with and count liberties, avoiding overlap.
  int numConnectingGroups = 0;
  Loc connectingGroupHeads[6];
  for(int i = 0; i<6; i++) {
    Loc adj = loc + adj_offsets[i];
    if(colors[adj] == pla) {
      Loc head = chain_head[adj];
      bool alreadyFound = false;
      for(int j = 0; j<numConnectingGroups; j++) {
        if(connectingGroupHeads[j] == head)
        {alreadyFound = true; break;}
      }
      if(!alreadyFound) {
        connectingGroupHeads[numConnectingGroups++] = head;

        Loc cur = adj;
        do
        {
          for(int k = 0; k < 6; k++) {
            Loc possibleLib = cur + adj_offsets[k];
            if(possibleLib != loc && wouldBeEmpty(possibleLib)) {
              bool alreadyCounted = false;
              for(int l = 0; l<numLibs; l++) {
                if(libs[l] == possibleLib)
                {alreadyCounted = true; break;}
              }
              if(!alreadyCounted) {
                libs[numLibs++] = possibleLib;
                if(numLibs >= max)
                  return max;
              }
            }
          }

          cur = next_in_chain[cur];
        } while (cur != adj);
      }
    }
  }
  return numLibs;
}

//Check if moving here is illegal due to simple ko
bool Board::isKoBanned(Loc loc) const
{
  return loc == ko_loc;
}

bool Board::isOnBoard(Loc loc) const {
  return loc >= 0 && loc < MAX_ARR_SIZE && colors[loc] != C_WALL;
}

//Check if moving here is illegal.
bool Board::isLegal(Loc loc, Player pla, bool isMultiStoneSuicideLegal) const
{
  if(pla != P_BLACK && pla != P_WHITE)
    return false;
  return loc == PASS_LOC || (
    loc >= 0 &&
    loc < MAX_ARR_SIZE &&
    (colors[loc] == C_EMPTY) &&
    !isKoBanned(loc) &&
    !isIllegalSuicide(loc, pla, isMultiStoneSuicideLegal)
  );
}

//Check if moving here is illegal, ignoring simple ko
bool Board::isLegalIgnoringKo(Loc loc, Player pla, bool isMultiStoneSuicideLegal) const
{
  if(pla != P_BLACK && pla != P_WHITE)
    return false;
  return loc == PASS_LOC || (
    loc >= 0 &&
    loc < MAX_ARR_SIZE &&
    (colors[loc] == C_EMPTY) &&
    !isIllegalSuicide(loc, pla, isMultiStoneSuicideLegal)
  );
}

bool Board::wouldBeCapture(Loc loc, Player pla) const {
  if(colors[loc] != C_EMPTY)
    return false;
  Player opp = getOpp(pla);
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;
    if(colors[adj] == opp)
    {
      if(getNumLiberties(adj) == 1)
        return true;
    }
  );

  return false;
}


bool Board::isAdjacentToPla(Loc loc, Player pla) const {
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;
    if(colors[adj] == pla)
      return true;
  );
  return false;
}


bool Board::isAdjacentToChain(Loc loc, Loc chain) const {
  if(colors[chain] == C_EMPTY)
    return false;
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;
    if(colors[adj] == colors[chain] && chain_head[adj] == chain_head[chain])
      return true;
  );
  return false;
}


bool Board::isEmpty() const {
  for(int y = 0; y < y_size; y++) {
    for(int x = 0; x < x_size; x++) {
      Loc loc = Location::getLoc(x,y,x_size);
      if(colors[loc] != C_EMPTY)
        return false;
    }
  }
  return true;
}

int Board::numStonesOnBoard() const {
  int num = 0;
  for(int y = 0; y < y_size; y++) {
    for(int x = 0; x < x_size; x++) {
      Loc loc = Location::getLoc(x,y,x_size);
      if(colors[loc] == C_BLACK || colors[loc] == C_WHITE)
        num += 1;
    }
  }
  return num;
}

int Board::numPlaStonesOnBoard(Player pla) const {
  int num = 0;
  for(int y = 0; y < y_size; y++) {
    for(int x = 0; x < x_size; x++) {
      Loc loc = Location::getLoc(x,y,x_size);
      if(colors[loc] == pla)
        num += 1;
    }
  }
  return num;
}

bool Board::setStone(Loc loc, Color color)
{
  if(loc < 0 || loc >= MAX_ARR_SIZE || colors[loc] == C_WALL)
    return false;
  if(color != C_BLACK && color != C_WHITE && color != C_EMPTY)
    return false;

  if(colors[loc] == color)
  {}
  else if(colors[loc] == C_EMPTY)
    playMoveAssumeLegal(loc,color);
  else if(color == C_EMPTY)
    removeSingleStone(loc);
  else {
    removeSingleStone(loc);
    if(!isSuicide(loc,color))
      playMoveAssumeLegal(loc,color);
  }

  ko_loc = NULL_LOC;
  return true;
}


//Attempts to play the specified move. Returns true if successful, returns false if the move was illegal.
bool Board::playMove(Loc loc, Player pla, bool isMultiStoneSuicideLegal)
{
  if(isLegal(loc,pla,isMultiStoneSuicideLegal))
  {
    playMoveAssumeLegal(loc,pla);
    return true;
  }
  return false;
}

//Plays the specified move, assuming it is legal, and returns a MoveRecord for the move
Board::MoveRecord Board::playMoveRecorded(Loc loc, Player pla)
{
  MoveRecord record;
  record.loc = loc;
  record.pla = pla;
  record.ko_loc = ko_loc;
  record.capDirs = 0;

  if(loc != PASS_LOC) {
    Player opp = getOpp(pla);

    { int adj = loc + ADJ0;
      if(colors[adj] == opp && getNumLiberties(adj) == 1)
        record.capDirs |= (((uint8_t)1) << 0); }
    { int adj = loc + ADJ1;
      if(colors[adj] == opp && getNumLiberties(adj) == 1)
        record.capDirs |= (((uint8_t)1) << 1); }
    { int adj = loc + ADJ2;
      if(colors[adj] == opp && getNumLiberties(adj) == 1)
        record.capDirs |= (((uint8_t)1) << 2); }
    { int adj = loc + ADJ3;
      if(colors[adj] == opp && getNumLiberties(adj) == 1)
        record.capDirs |= (((uint8_t)1) << 3); }
    { int adj = loc + ADJ4;
    if(colors[adj] == opp && getNumLiberties(adj) == 1)
      record.capDirs |= (((uint8_t)1) << 4); }
    { int adj = loc + ADJ5;
    if(colors[adj] == opp && getNumLiberties(adj) == 1)
      record.capDirs |= (((uint8_t)1) << 5); }

    if(record.capDirs == 0 && isSuicide(loc,pla))
      record.capDirs = 0x40;
  }

  playMoveAssumeLegal(loc, pla);
  return record;
}

//Undo the move given by record. Moves MUST be undone in the order they were made.
//Undos will NOT typically restore the precise representation in the board to the way it was. The heads of chains
//might change, the order of the circular lists might change, etc.
void Board::undo(Board::MoveRecord record)
{
  ko_loc = record.ko_loc;

  Loc loc = record.loc;
  if (loc == PASS_LOC)
  {
    if (record.pla == P_BLACK)setBlackPasses(numBlackPasses - 1);
    else if (record.pla == P_WHITE)setWhitePasses(numWhitePasses - 1);
    return;
  }

  //Re-fill stones in all captured directions
  for(int i = 0; i<6; i++)
  {
    int adj = loc + adj_offsets[i];
    if(record.capDirs & (1 << i))
    {
      if(colors[adj] == C_EMPTY) {
        addChain(adj, getOpp(record.pla));

        int numUncaptured = chain_data[chain_head[adj]].num_locs;
        if(record.pla == P_BLACK)
          setWhiteCaptures(numWhiteCaptures - numUncaptured);
        else
          setBlackCaptures(numBlackCaptures - numUncaptured);
      }
    }
  }
  //Re-fill suicided stones
  if(record.capDirs == 0x40) {
    assert(colors[loc] == C_EMPTY);
    addChain(loc,record.pla);
    int numUncaptured = chain_data[chain_head[loc]].num_locs;
    if(record.pla == P_BLACK)
      setBlackCaptures(numBlackCaptures - numUncaptured);
    else
      setWhiteCaptures(numWhiteCaptures - numUncaptured);
  }

  //Delete the stone played here.
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][colors[loc]];
  colors[loc] = C_EMPTY;
  // empty_list.add(loc);

  //Uneat opp liberties
  changeSurroundingLiberties(loc, getOpp(record.pla),+1);

  //If this was not a single stone, we may need to recompute the chain from scratch
  if(chain_data[chain_head[loc]].num_locs > 1)
  {
    int numNeighbors = 0;
    FOREACHADJ(
      int adj = loc + ADJOFFSET;
      if(colors[adj] == record.pla)
        numNeighbors++;
    );

    //If the move had exactly one neighbor, we know its undoing didn't disconnect the group,
    //so don't need to rebuild the whole chain.
    if(numNeighbors <= 1) {
      //If the undone move was the location of the head, we need to move the head.
      Loc head = chain_head[loc];
      if(head == loc) {
        Loc newHead = next_in_chain[loc];
        //Run through the whole chain and make their heads point to the new head
        Loc cur = loc;
        do
        {
          chain_head[cur] = newHead;
          cur = next_in_chain[cur];
        } while (cur != loc);

        //Move over the head data
        chain_data[newHead] = chain_data[head];
        head = newHead;
      }

      //Extract this move out of the circlar list of stones. Unfortunately we don't have a prev pointer, so we need to walk the loop.
      {
        //Starting at the head is likely to need to walk less since whenever we merge a single stone into an existing group
        //we put it right after the old head.
        Loc cur = head;
        while(next_in_chain[cur] != loc)
          cur = next_in_chain[cur];
        //Advance the pointer to put loc out of the loop
        next_in_chain[cur] = next_in_chain[loc];
      }

      //Lastly, fix up liberties. Removing this stone removed all liberties next to this stone
      //that weren't already liberties of the group.
      int libertyDelta = 0;
      FOREACHADJ(
        int adj = loc + ADJOFFSET;
        if(colors[adj] == C_EMPTY && !isLibertyOf(adj,head)) libertyDelta--;
      );
      //Removing this stone itself added a liberty to the group though.
      libertyDelta++;
      chain_data[head].num_liberties += libertyDelta;
      //And update the count of stones in the group
      chain_data[head].num_locs--;
    }
    //More than one neighbor. Removing this stone potentially disconnects the group into two, so we just do a complete rebuild
    //of the resulting group(s).
    else {
      //Run through the whole chain and make their heads point to nothing
      Loc cur = loc;
      do
      {
        chain_head[cur] = NULL_LOC;
        cur = next_in_chain[cur];
      } while (cur != loc);

      //Rebuild each chain adjacent now
      for(int i = 0; i<6; i++)
      {
        int adj = loc + adj_offsets[i];
        if(colors[adj] == record.pla && chain_head[adj] == NULL_LOC)
          rebuildChain(adj, record.pla);
      }
    }
  }
}

Hash128 Board::getPosHashAfterMove(Loc loc, Player pla) const {
  if(loc == PASS_LOC)
    return pos_hash;
  assert(loc != NULL_LOC);

  Hash128 hash = pos_hash;
  hash ^= ZOBRIST_BOARD_HASH[loc][pla];

  Player opp = getOpp(pla);

  //Count immediate liberties and groups that would be captured
  bool wouldBeSuicide = true;
  int numCapturedGroups = 0;
  Loc capturedGroupHeads[6];
  for(int i = 0; i < 6; i++) {
    Loc adj = loc + adj_offsets[i];
    if(colors[adj] == C_EMPTY)
      wouldBeSuicide = false;
    else if(colors[adj] == pla && getNumLiberties(adj) > 1)
      wouldBeSuicide = false;
    else if(colors[adj] == opp) {
      //Capture!
      if(getNumLiberties(adj) == 1) {
        //Make sure we haven't already counted it
        Loc head = chain_head[adj];
        bool alreadyFound = false;
        for(int j = 0; j<numCapturedGroups; j++) {
          if(capturedGroupHeads[j] == head)
          {alreadyFound = true; break;}
        }
        if(!alreadyFound) {
          capturedGroupHeads[numCapturedGroups++] = head;
          wouldBeSuicide = false;

          //Now iterate through the group to update the hash
          Loc cur = adj;
          do {
            hash ^= ZOBRIST_BOARD_HASH[cur][opp];
            cur = next_in_chain[cur];
          } while (cur != adj);
        }
      }
    }
  }

  //Update hash for suicidal moves
  if(wouldBeSuicide) {
    assert(numCapturedGroups == 0);

    for(int i = 0; i < 6; i++) {
      Loc adj = loc + adj_offsets[i];
      //Suicide capture!
      if(colors[adj] == pla && getNumLiberties(adj) == 1) {
        //Make sure we haven't already counted it
        Loc head = chain_head[adj];
        bool alreadyFound = false;
        for(int j = 0; j<numCapturedGroups; j++) {
          if(capturedGroupHeads[j] == head)
          {alreadyFound = true; break;}
        }
        if(!alreadyFound) {
          capturedGroupHeads[numCapturedGroups++] = head;

          //Now iterate through the group to update the hash
          Loc cur = adj;
          do {
            hash ^= ZOBRIST_BOARD_HASH[cur][pla];
            cur = next_in_chain[cur];
          } while (cur != adj);
        }
      }
    }

    //Don't forget the stone we'd place would also die
    hash ^= ZOBRIST_BOARD_HASH[loc][pla];
  }

  return hash;
}

//Plays the specified move, assuming it is legal.
void Board::playMoveAssumeLegal(Loc loc, Player pla)
{
  //Pass?
  if(loc == PASS_LOC)
  {
    ko_loc = NULL_LOC;
    if (pla == P_BLACK)setBlackPasses(numBlackPasses + 1);
    else if (pla == P_WHITE)setWhitePasses(numWhitePasses + 1);
    return;
  }

  Player opp = getOpp(pla);

  //Add the new stone as an independent group
  colors[loc] = pla;
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][pla];
  chain_data[loc].owner = pla;
  chain_data[loc].num_locs = 1;
  chain_data[loc].num_liberties = getNumImmediateLiberties(loc);
  chain_head[loc] = loc;
  next_in_chain[loc] = loc;
  // empty_list.remove(loc);

  //Merge with surrounding friendly chains and capture any necessary opp chains
  int num_captured = 0; //Number of stones captured
  Loc possible_ko_loc = NULL_LOC;  //What location a ko ban might become possible in
  int num_opps_seen = 0;  //How many opp chains we have seen so far
  Loc opp_heads_seen[6];   //Heads of the opp chains seen so far

  for(int i = 0; i < 6; i++)
  {
    int adj = loc + adj_offsets[i];

    //Friendly chain!
    if(colors[adj] == pla)
    {
      //Already merged?
      if(chain_head[adj] == chain_head[loc])
        continue;

      //Otherwise, eat one liberty and merge them
      chain_data[chain_head[adj]].num_liberties--;
      mergeChains(adj,loc);
    }

    //Opp chain!
    else if(colors[adj] == opp)
    {
      Loc opp_head = chain_head[adj];

      //Have we seen it already?
      bool seen = false;
      for(int j = 0; j<num_opps_seen; j++)
        if(opp_heads_seen[j] == opp_head)
        {seen = true; break;}

      if(seen)
        continue;

      //Not already seen! Eat one liberty from it and mark it as seen
      chain_data[opp_head].num_liberties--;
      opp_heads_seen[num_opps_seen++] = opp_head;

      //Kill it?
      if(getNumLiberties(adj) == 0)
      {
        num_captured += removeChain(adj);
        possible_ko_loc = adj;
      }
    }
  }

  //We have a ko if 1 stone was captured and the capturing move is one isolated stone
  //And the capturing move itself now only has one liberty
    ko_loc = NULL_LOC;

  if(pla == P_BLACK)
    setWhiteCaptures(numWhiteCaptures + num_captured);
  else
    setBlackCaptures(numBlackCaptures + num_captured);

  //Handle suicide
  if(getNumLiberties(loc) == 0) {
    int numSuicided = chain_data[chain_head[loc]].num_locs;
    removeChain(loc);

    if(pla == P_BLACK)
      setBlackCaptures(numBlackCaptures + numSuicided);
    else
      setWhiteCaptures(numWhiteCaptures + numSuicided);
  }
}

int Board::getNumImmediateLiberties(Loc loc) const
{
  int num_libs = 0;
  if(colors[loc + ADJ0] == C_EMPTY) num_libs++;
  if(colors[loc + ADJ1] == C_EMPTY) num_libs++;
  if(colors[loc + ADJ2] == C_EMPTY) num_libs++;
  if(colors[loc + ADJ3] == C_EMPTY) num_libs++;
  if(colors[loc + ADJ4] == C_EMPTY) num_libs++;
  if(colors[loc + ADJ5] == C_EMPTY) num_libs++;

  return num_libs;
}

//Loc is a liberty of head's chain if loc is empty and adjacent to a stone of head.
//Assumes loc is empty
bool Board::isLibertyOf(Loc loc, Loc head) const
{
  Loc adj;
  adj = loc + ADJ0;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;
  adj = loc + ADJ1;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;
  adj = loc + ADJ2;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;
  adj = loc + ADJ3;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;
  adj = loc + ADJ4;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;
  adj = loc + ADJ5;
  if(colors[adj] == colors[head] && chain_head[adj] == head)
    return true;

  return false;
}

void Board::mergeChains(Loc loc1, Loc loc2)
{
  //Find heads
  Loc head1 = chain_head[loc1];
  Loc head2 = chain_head[loc2];

  assert(head1 != head2);
  assert(chain_data[head1].owner == chain_data[head2].owner);

  //Make sure head2 is the smaller chain.
  if(chain_data[head1].num_locs < chain_data[head2].num_locs)
  {
    Loc temp = head1;
    head1 = head2;
    head2 = temp;
  }

  //Iterate through each stone of head2's chain to make it a member of head1's chain.
  //Count new liberties for head1 as we go.
  chain_data[head1].num_locs += chain_data[head2].num_locs;
  int numNewLiberties = 0;
  Loc loc = head2;
  while(true)
  {
    //Any adjacent liberty is a new liberty for head1 if it is not adjacent to a stone of head1
    FOREACHADJ(
      Loc adj = loc + ADJOFFSET;
      if(colors[adj] == C_EMPTY && !isLibertyOf(adj,head1))
        numNewLiberties++;
    );

    //Now, add this stone to head1.
    chain_head[loc] = head1;

    //If are not back around, we are done.
    if(next_in_chain[loc] != head2)
      loc = next_in_chain[loc];
    else
      break;
  }

  //Add in the liberties
  chain_data[head1].num_liberties += numNewLiberties;

  //We link up (head1 -> next1 -> ... -> last1 -> head1) and (head2 -> next2 -> ... -> last2 -> head2)
  //as: head1 -> head2 -> next2 -> ... -> last2 -> next1 -> ... -> last1 -> head1
  //loc is now last_2
  next_in_chain[loc] = next_in_chain[head1];
  next_in_chain[head1] = head2;
}

//Returns number of stones captured
int Board::removeChain(Loc loc)
{
  int num_stones_removed = 0; //Num stones removed
  Player opp = getOpp(colors[loc]);

  //Walk around the chain...
  Loc cur = loc;
  do
  {
    //Empty out this location
    pos_hash ^= ZOBRIST_BOARD_HASH[cur][colors[cur]];
    colors[cur] = C_EMPTY;
    num_stones_removed++;
    // empty_list.add(cur);

    //For each distinct opp chain around, add a liberty to it.
    changeSurroundingLiberties(cur,opp,+1);

    cur = next_in_chain[cur];

  } while (cur != loc);

  return num_stones_removed;
}

//Remove a single stone, even a stone part of a larger group.
void Board::removeSingleStone(Loc loc)
{
  Player pla = colors[loc];

  //Save the entire chain's stone locations
  int num_locs = chain_data[chain_head[loc]].num_locs;
  int locs[MAX_PLAY_SIZE];
  int idx = 0;
  Loc cur = loc;
  do
  {
    locs[idx++] = cur;
    cur = next_in_chain[cur];
  } while (cur != loc);
  assert(idx == num_locs);

  //Delete the entire chain
  removeChain(loc);

  //Then add all the other stones back one by one.
  for(int i = 0; i<num_locs; i++) {
    if(locs[i] != loc)
      playMoveAssumeLegal(locs[i],pla);
  }
}

//Add a chain of the given player to the given region of empty space, floodfilling it.
//Assumes that this region does not border any chains of the desired color already
void Board::addChain(Loc loc, Player pla)
{
  chain_data[loc].num_liberties = 0;
  chain_data[loc].num_locs = 0;
  chain_data[loc].owner = pla;

  //Add a chain with links front -> ... -> loc -> loc with all head pointers towards loc
  Loc front = addChainHelper(loc, loc, loc, pla);

  //Now, we make loc point to front, and that completes the circle!
  next_in_chain[loc] = front;
}

//Floodfill a chain of the given color into this region of empty spaces
//Make the specified loc the head for all the chains and updates the chainData of head with the number of stones.
//Does NOT connect the stones into a circular list. Rather, it produces an linear linked list with the tail pointing
//to tailTarget, and returns the head of the list. The tail is guaranteed to be loc.
Loc Board::addChainHelper(Loc head, Loc tailTarget, Loc loc, Player pla)
{
  //Add stone here
  colors[loc] = pla;
  pos_hash ^= ZOBRIST_BOARD_HASH[loc][pla];
  chain_head[loc] = head;
  chain_data[head].num_locs++;
  next_in_chain[loc] = tailTarget;
  // empty_list.remove(loc);

  //Eat opp liberties
  changeSurroundingLiberties(loc,getOpp(pla),-1);

  //Recursively add stones around us.
  Loc nextTailTarget = loc;
  for(int i = 0; i<6; i++)
  {
    Loc adj = loc + adj_offsets[i];
    if(colors[adj] == C_EMPTY)
      nextTailTarget = addChainHelper(head,nextTailTarget,adj,pla);
  }
  return nextTailTarget;
}

//Floods through a chain of the specified player already on the board
//rebuilding its links and counting its liberties as we go.
//Requires that all their heads point towards
//some invalid location, such as NULL_LOC or a location not of color.
//The head of the chain will be loc.
void Board::rebuildChain(Loc loc, Player pla)
{
  chain_data[loc].num_liberties = 0;
  chain_data[loc].num_locs = 0;
  chain_data[loc].owner = pla;

  //Rebuild chain with links front -> ... -> loc -> loc with all head pointers towards loc
  Loc front = rebuildChainHelper(loc, loc, loc, pla);

  //Now, we make loc point to front, and that completes the circle!
  next_in_chain[loc] = front;
}

//Does same thing as addChain, but floods through a chain of the specified color already on the board
//rebuilding its links and also counts its liberties as we go. Requires that all their heads point towards
//some invalid location, such as NULL_LOC or a location not of color.
Loc Board::rebuildChainHelper(Loc head, Loc tailTarget, Loc loc, Player pla)
{
  Loc adj0 = loc + ADJ0;
  Loc adj1 = loc + ADJ1;
  Loc adj2 = loc + ADJ2;
  Loc adj3 = loc + ADJ3;
  Loc adj4 = loc + ADJ4;
  Loc adj5 = loc + ADJ5;

  //Count new liberties
  int numHeadLibertiesToAdd = 0;
  if(colors[adj0] == C_EMPTY && !isLibertyOf(adj0,head)) numHeadLibertiesToAdd++;
  if(colors[adj1] == C_EMPTY && !isLibertyOf(adj1,head)) numHeadLibertiesToAdd++;
  if(colors[adj2] == C_EMPTY && !isLibertyOf(adj2,head)) numHeadLibertiesToAdd++;
  if(colors[adj3] == C_EMPTY && !isLibertyOf(adj3,head)) numHeadLibertiesToAdd++;
  if(colors[adj4] == C_EMPTY && !isLibertyOf(adj4,head)) numHeadLibertiesToAdd++;
  if(colors[adj5] == C_EMPTY && !isLibertyOf(adj5,head)) numHeadLibertiesToAdd++;
  chain_data[head].num_liberties += numHeadLibertiesToAdd;

  //Add stone here to the chain by setting its head
  chain_head[loc] = head;
  chain_data[head].num_locs++;
  next_in_chain[loc] = tailTarget;

  //Recursively add stones around us.
  Loc nextTailTarget = loc;
  if(colors[adj0] == pla && chain_head[adj0] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj0,pla);
  if(colors[adj1] == pla && chain_head[adj1] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj1,pla);
  if(colors[adj2] == pla && chain_head[adj2] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj2,pla);
  if(colors[adj3] == pla && chain_head[adj3] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj3,pla);
  if(colors[adj4] == pla && chain_head[adj4] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj4,pla);
  if(colors[adj5] == pla && chain_head[adj5] != head) nextTailTarget = rebuildChainHelper(head,nextTailTarget,adj5,pla);
  return nextTailTarget;
}

//Apply the specified delta to the liberties of all adjacent groups of the specified color
void Board::changeSurroundingLiberties(Loc loc, Player pla, int delta)
{
  Loc adj0 = loc + ADJ0;
  Loc adj1 = loc + ADJ1;
  Loc adj2 = loc + ADJ2;
  Loc adj3 = loc + ADJ3;
  Loc adj4 = loc + ADJ4;
  Loc adj5 = loc + ADJ5;

  if(colors[adj0] == pla)
    chain_data[chain_head[adj0]].num_liberties += delta;
  if(colors[adj1] == pla
     && !(colors[adj0] == pla && chain_head[adj0] == chain_head[adj1]))
    chain_data[chain_head[adj1]].num_liberties += delta;
  if(colors[adj2] == pla
     && !(colors[adj0] == pla && chain_head[adj0] == chain_head[adj2])
     && !(colors[adj1] == pla && chain_head[adj1] == chain_head[adj2]))
    chain_data[chain_head[adj2]].num_liberties += delta;
  if(colors[adj3] == pla
    && !(colors[adj0] == pla && chain_head[adj0] == chain_head[adj3])
    && !(colors[adj1] == pla && chain_head[adj1] == chain_head[adj3])
    && !(colors[adj2] == pla && chain_head[adj2] == chain_head[adj3]))
    chain_data[chain_head[adj3]].num_liberties += delta;
  if(colors[adj4] == pla
    && !(colors[adj0] == pla && chain_head[adj0] == chain_head[adj4])
    && !(colors[adj1] == pla && chain_head[adj1] == chain_head[adj4])
    && !(colors[adj2] == pla && chain_head[adj2] == chain_head[adj4])
    && !(colors[adj3] == pla && chain_head[adj3] == chain_head[adj4]))
    chain_data[chain_head[adj4]].num_liberties += delta;
  if(colors[adj5] == pla
    && !(colors[adj0] == pla && chain_head[adj0] == chain_head[adj5])
    && !(colors[adj1] == pla && chain_head[adj1] == chain_head[adj5])
    && !(colors[adj2] == pla && chain_head[adj2] == chain_head[adj5])
    && !(colors[adj3] == pla && chain_head[adj3] == chain_head[adj5])
    && !(colors[adj4] == pla && chain_head[adj4] == chain_head[adj5]))
    chain_data[chain_head[adj5]].num_liberties += delta;
}


// Board::PointList::PointList()
// {
//   std::memset(list_, NULL_LOC, sizeof(list_));
//   std::memset(indices_, -1, sizeof(indices_));
//   size_ = 0;
// }

// Board::PointList::PointList(const Board::PointList& other)
// {
//   std::memcpy(list_, other.list_, sizeof(list_));
//   std::memcpy(indices_, other.indices_, sizeof(indices_));
//   size_ = other.size_;
// }

// void Board::PointList::operator=(const Board::PointList& other)
// {
//   if(this == &other)
//     return;
//   std::memcpy(list_, other.list_, sizeof(list_));
//   std::memcpy(indices_, other.indices_, sizeof(indices_));
//   size_ = other.size_;
// }

// void Board::PointList::add(Loc loc)
// {
//   //assert (size_ < MAX_PLAY_SIZE);
//   list_[size_] = loc;
//   indices_[loc] = size_;
//   size_++;
// }

// void Board::PointList::remove(Loc loc)
// {
//   //assert(size_ >= 0);
//   int index = indices_[loc];
//   //assert(index >= 0 && index < size_);
//   //assert(list_[index] == loc);
//   Loc end_loc = list_[size_-1];
//   list_[index] = end_loc;
//   list_[size_-1] = NULL_LOC;
//   indices_[end_loc] = index;
//   indices_[loc] = -1;
//   size_--;
// }

// int Board::PointList::size() const
// {
//   return size_;
// }

// Loc& Board::PointList::operator[](int n)
// {
//   assert (n < size_);
//   return list_[n];
// }

// bool Board::PointList::contains(Loc loc) const {
//   return indices_[loc] != -1;
// }

int Location::distance(Loc loc0, Loc loc1, int x_size) {
  int dx = getX(loc1,x_size) - getX(loc0,x_size);
  int dy = (loc1-loc0-dx) / (x_size+1);
  return (dx >= 0 ? dx : -dx) + (dy >= 0 ? dy : -dy);
}

int Location::euclideanDistanceSquared(Loc loc0, Loc loc1, int x_size) {
  int dx = getX(loc1,x_size) - getX(loc0,x_size);
  int dy = (loc1-loc0-dx) / (x_size+1);
  return dx*dx + dy*dy;
}

//TACTICAL STUFF--------------------------------------------------------------------

//Helper, find liberties of group at loc. Fills in buf, returns the number of liberties.
//bufStart is where to start checking to avoid duplicates. bufIdx is where to start actually writing.
int Board::findLiberties(Loc loc, vector<Loc>& buf, int bufStart, int bufIdx) const {
  int numFound = 0;
  Loc cur = loc;
  do
  {
    for(int i = 0; i < 6; i++) {
      Loc lib = cur + adj_offsets[i];
      if(colors[lib] == C_EMPTY) {
        //Check for dups
        bool foundDup = false;
        for(int j = bufStart; j < bufIdx+numFound; j++) {
          if(buf[j] == lib) {
            foundDup = true;
            break;
          }
        }
        if(!foundDup) {
          if(bufIdx+numFound >= buf.size())
            buf.resize(buf.size() * 3/2 + 64);
          buf[bufIdx+numFound] = lib;
          numFound++;
        }
      }
    }

    cur = next_in_chain[cur];
  } while (cur != loc);

  return numFound;
}


void Board::checkConsistency() const {
  const string errLabel = string("Board::checkConsistency(): ");

  bool chainLocChecked[MAX_ARR_SIZE];
  for(int i = 0; i<MAX_ARR_SIZE; i++)
    chainLocChecked[i] = false;

  vector<Loc> buf;
  auto checkChainConsistency = [&buf,errLabel,&chainLocChecked,this](Loc loc) {
    Player pla = colors[loc];
    Loc head = chain_head[loc];
    Loc cur = loc;
    int stoneCount = 0;
    int pseudoLibs = 0;
    bool foundChainHead = false;
    do {
      chainLocChecked[cur] = true;

      if(colors[cur] != pla)
        throw StringError(errLabel + "Chain is not all the same color");
      if(chain_head[cur] != head)
        throw StringError(errLabel + "Chain does not all have the same head");

      stoneCount++;
      pseudoLibs += getNumImmediateLiberties(cur);
      if(cur == head)
        foundChainHead = true;

      if(stoneCount > MAX_PLAY_SIZE)
        throw StringError(errLabel + "Chain exceeds size of board - broken circular list?");
      cur = next_in_chain[cur];

      if(cur < 0 || cur >= MAX_ARR_SIZE)
        throw StringError(errLabel + "Chain location is outside of board bounds, data corruption?");

    } while (cur != loc);

    if(!foundChainHead)
      throw StringError(errLabel + "Chain loop does not contain head");

    const ChainData& data = chain_data[head];
    if(data.owner != pla)
      throw StringError(errLabel + "Chain data owner does not match stones");
    if(data.num_locs != stoneCount)
      throw StringError(errLabel + "Chain data num_locs does not match actual stone count");
    if(data.num_liberties > pseudoLibs)
      throw StringError(errLabel + "Chain data liberties exceeds pseudoliberties");
    if(data.num_liberties <= 0)
      throw StringError(errLabel + "Chain data liberties is nonpositive");

    int numFoundLibs = findLiberties(loc,buf,0,0);
    if(numFoundLibs != data.num_liberties)
      throw StringError(errLabel + "FindLiberties found a different number of libs");
  };

  Hash128 tmp_pos_hash = ZOBRIST_SIZE_X_HASH[x_size] ^ ZOBRIST_SIZE_Y_HASH[y_size];
  int emptyCount = 0;
  for(Loc loc = 0; loc < MAX_ARR_SIZE; loc++) {
    int x = Location::getX(loc,x_size);
    int y = Location::getY(loc,x_size);
    if(x < 0 || x >= x_size || y < 0 || y >= y_size) {
      if(colors[loc] != C_WALL)
        throw StringError(errLabel + "Non-WALL value outside of board legal area");
    }
    else {
      if(colors[loc] == C_BLACK || colors[loc] == C_WHITE) {
        if(!chainLocChecked[loc])
          checkChainConsistency(loc);
        // if(empty_list.contains(loc))
        //   throw StringError(errLabel + "Empty list contains filled location");

        tmp_pos_hash ^= ZOBRIST_BOARD_HASH[loc][colors[loc]];
        tmp_pos_hash ^= ZOBRIST_BOARD_HASH[loc][C_EMPTY];
      }
      else if(colors[loc] == C_EMPTY) {
        // if(!empty_list.contains(loc))
        //   throw StringError(errLabel + "Empty list doesn't contain empty location");
        emptyCount += 1;
      }
      else
        throw StringError(errLabel + "Non-(black,white,empty) value within board legal area");
    }
  }
  tmp_pos_hash ^= ZOBRIST_CAPTURE_B_HASH[numBlackCaptures];
  tmp_pos_hash ^= ZOBRIST_CAPTURE_W_HASH[numWhiteCaptures];
  tmp_pos_hash ^= ZOBRIST_PASSNUM_B_HASH[numBlackPasses];
  tmp_pos_hash ^= ZOBRIST_PASSNUM_W_HASH[numWhitePasses];
  if(pos_hash != tmp_pos_hash)
    throw StringError(errLabel + "Pos hash does not match expected");

  // if(empty_list.size_ != emptyCount)
  //   throw StringError(errLabel + "Empty list size is not the number of empty points");
  // for(int i = 0; i<emptyCount; i++) {
  //   Loc loc = empty_list.list_[i];
  //   int x = Location::getX(loc,x_size);
  //   int y = Location::getY(loc,x_size);
  //   if(x < 0 || x >= x_size || y < 0 || y >= y_size)
  //     throw StringError(errLabel + "Invalid empty list loc");
  //   if(empty_list.indices_[loc] != i)
  //     throw StringError(errLabel + "Empty list index for loc in index i is not i");
  // }

  if(ko_loc != NULL_LOC) {
    int x = Location::getX(ko_loc,x_size);
    int y = Location::getY(ko_loc,x_size);
    if(x < 0 || x >= x_size || y < 0 || y >= y_size)
      throw StringError(errLabel + "Invalid simple ko loc");
    if(getNumImmediateLiberties(ko_loc) != 0)
      throw StringError(errLabel + "Simple ko loc has immediate liberties");
  }

  short tmpAdjOffsets[8];
  Location::getAdjacentOffsets(tmpAdjOffsets,x_size);
  for(int i = 0; i<8; i++)
    if(tmpAdjOffsets[i] != adj_offsets[i])
      throw StringError(errLabel + "Corrupted adj_offsets array");
}

bool Board::isEqualForTesting(const Board& other, bool checkNumCaptures, bool checkSimpleKo) const {
  checkConsistency();
  other.checkConsistency();
  if(x_size != other.x_size)
    return false;
  if(y_size != other.y_size)
    return false;
  if(checkSimpleKo && ko_loc != other.ko_loc)
    return false;
  if(numBlackCaptures != other.numBlackCaptures)
    return false;
  if(numWhiteCaptures != other.numWhiteCaptures)
    return false;
  if(numBlackPasses != other.numBlackPasses)
    return false;
  if(numWhitePasses != other.numWhitePasses)
    return false;
  if(pos_hash != other.pos_hash)
    return false;
  for(int i = 0; i<MAX_ARR_SIZE; i++) {
    if(colors[i] != other.colors[i])
      return false;
  }
  //We don't require that the chain linked lists are in the same order.
  //Consistency check ensures that all the linked lists are consistent with colors array, which we checked.
  return true;
}



//IO FUNCS------------------------------------------------------------------------------------------

char PlayerIO::colorToChar(Color c)
{
  switch(c) {
  case C_BLACK: return 'X';
  case C_WHITE: return 'O';
  case C_EMPTY: return '.';
  default:  return '#';
  }
}

string PlayerIO::playerToString(Color c)
{
  switch(c) {
  case C_BLACK: return "Black";
  case C_WHITE: return "White";
  case C_EMPTY: return "Empty";
  default:  return "Wall";
  }
}

string PlayerIO::playerToStringShort(Color c)
{
  switch(c) {
  case C_BLACK: return "B";
  case C_WHITE: return "W";
  case C_EMPTY: return "E";
  default:  return "";
  }
}

bool PlayerIO::tryParsePlayer(const string& s, Player& pla) {
  string str = Global::toLower(s);
  if(str == "black" || str == "b") {
    pla = P_BLACK;
    return true;
  }
  else if(str == "white" || str == "w") {
    pla = P_WHITE;
    return true;
  }
  return false;
}

Player PlayerIO::parsePlayer(const string& s) {
  Player pla = C_EMPTY;
  bool suc = tryParsePlayer(s,pla);
  if(!suc)
    throw StringError("Could not parse player: " + s);
  return pla;
}

string Location::toStringMach(Loc loc, int x_size)
{
  if(loc == Board::PASS_LOC)
    return string("pass");
  if(loc == Board::NULL_LOC)
    return string("null");
  char buf[128];
  sprintf(buf,"(%d,%d)",getX(loc,x_size),getY(loc,x_size));
  return string(buf);
}

string Location::toString(Loc loc, int x_size, int y_size)
{
  if(x_size > 25*25)
    return toStringMach(loc,x_size);
  if(loc == Board::PASS_LOC)
    return string("pass");
  if(loc == Board::NULL_LOC)
    return string("null");
  const char* xChar = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
  int x = getX(loc,x_size);
  int y = getY(loc,x_size);
  if(x >= x_size || x < 0 || y < 0 || y >= y_size)
    return toStringMach(loc,x_size);

  char buf[128];
  if(x <= 24)
    sprintf(buf,"%c%d",xChar[x],y_size-y);
  else
    sprintf(buf,"%c%c%d",xChar[x/25-1],xChar[x%25],y_size-y);
  return string(buf);
}

string Location::toString(Loc loc, const Board& b) {
  return toString(loc,b.x_size,b.y_size);
}

string Location::toStringMach(Loc loc, const Board& b) {
  return toStringMach(loc,b.x_size);
}

static bool tryParseLetterCoordinate(char c, int& x) {
  if(c >= 'A' && c <= 'H')
    x = c-'A';
  else if(c >= 'a' && c <= 'h')
    x = c-'a';
  else if(c >= 'J' && c <= 'Z')
    x = c-'A'-1;
  else if(c >= 'j' && c <= 'z')
    x = c-'a'-1;
  else
    return false;
  return true;
}

bool Location::tryOfString(const string& str, int x_size, int y_size, Loc& result) {
  string s = Global::trim(str);
  if(s.length() < 2)
    return false;
  if(Global::isEqualCaseInsensitive(s,string("pass")) || Global::isEqualCaseInsensitive(s,string("pss"))) {
    result = Board::PASS_LOC;
    return true;
  }
  if(s[0] == '(') {
    if(s[s.length()-1] != ')')
      return false;
    s = s.substr(1,s.length()-2);
    vector<string> pieces = Global::split(s,',');
    if(pieces.size() != 2)
      return false;
    int x;
    int y;
    bool sucX = Global::tryStringToInt(pieces[0],x);
    bool sucY = Global::tryStringToInt(pieces[1],y);
    if(!sucX || !sucY)
      return false;
    result = Location::getLoc(x,y,x_size);
    return true;
  }
  else {
    int x;
    if(!tryParseLetterCoordinate(s[0],x))
      return false;

    //Extended format
    if((s[1] >= 'A' && s[1] <= 'Z') || (s[1] >= 'a' && s[1] <= 'z')) {
      int x1;
      if(!tryParseLetterCoordinate(s[1],x1))
        return false;
      x = (x+1) * 25 + x1;
      s = s.substr(2,s.length()-2);
    }
    else {
      s = s.substr(1,s.length()-1);
    }

    int y;
    bool sucY = Global::tryStringToInt(s,y);
    if(!sucY)
      return false;
    y = y_size - y;
    if(x < 0 || y < 0 || x >= x_size || y >= y_size)
      return false;
    result = Location::getLoc(x,y,x_size);
    return true;
  }
}

bool Location::tryOfStringAllowNull(const string& str, int x_size, int y_size, Loc& result) {
  if(str == "null") {
    result = Board::NULL_LOC;
    return true;
  }
  return tryOfString(str, x_size, y_size, result);
}

bool Location::tryOfString(const string& str, const Board& b, Loc& result) {
  return tryOfString(str,b.x_size,b.y_size,result);
}

bool Location::tryOfStringAllowNull(const string& str, const Board& b, Loc& result) {
  return tryOfStringAllowNull(str,b.x_size,b.y_size,result);
}

Loc Location::ofString(const string& str, int x_size, int y_size) {
  Loc result;
  if(tryOfString(str,x_size,y_size,result))
    return result;
  throw StringError("Could not parse board location: " + str);
}

Loc Location::ofStringAllowNull(const string& str, int x_size, int y_size) {
  Loc result;
  if(tryOfStringAllowNull(str,x_size,y_size,result))
    return result;
  throw StringError("Could not parse board location: " + str);
}

Loc Location::ofString(const string& str, const Board& b) {
  return ofString(str,b.x_size,b.y_size);
}


Loc Location::ofStringAllowNull(const string& str, const Board& b) {
  return ofStringAllowNull(str,b.x_size,b.y_size);
}

vector<Loc> Location::parseSequence(const string& str, const Board& board) {
  vector<string> pieces = Global::split(Global::trim(str),' ');
  vector<Loc> locs;
  for(size_t i = 0; i<pieces.size(); i++) {
    string piece = Global::trim(pieces[i]);
    if(piece.length() <= 0)
      continue;
    locs.push_back(Location::ofString(piece,board));
  }
  return locs;
}

void Board::printBoard(ostream& out, const Board& board, Loc markLoc, const vector<Move>* hist) {
  if(hist != NULL)
    out << "MoveNum: " << hist->size() << " ";
  out << "HASH: " << board.pos_hash << "\n";
  bool showCoords = board.x_size <= 50 && board.y_size <= 50;
  if(showCoords) {
    const char* xChar = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
    out << "  ";
    for(int x = 0; x < board.x_size; x++) {
      if(x <= 24) {
        out << " ";
        out << xChar[x];
      }
      else {
        out << "A" << xChar[x-25];
      }
    }
    out << "\n";
  }

  for(int y = 0; y < board.y_size; y++)
  {
    if(showCoords) {
      char buf[16];
      sprintf(buf,"%2d",board.y_size-y);
      out << buf << ' ';
    }
    for(int x = 0; x < board.x_size; x++)
    {
      Loc loc = Location::getLoc(x,y,board.x_size);
      char s = PlayerIO::colorToChar(board.colors[loc]);
      if(board.colors[loc] == C_EMPTY && markLoc == loc)
        out << '@';
      else
        out << s;

      bool histMarked = false;
      if(hist != NULL) {
        size_t start = hist->size() >= 3 ? hist->size()-3 : 0;
        for(size_t i = 0; start+i < hist->size(); i++) {
          if((*hist)[start+i].loc == loc) {
            out << (1+i);
            histMarked = true;
            break;
          }
        }
      }

      if(x < board.x_size-1 && !histMarked)
        out << ' ';
    }
    out << "\n";
  }
  out << "\n";
}

ostream& operator<<(ostream& out, const Board& board) {
  Board::printBoard(out,board,Board::NULL_LOC,NULL);
  return out;
}


string Board::toStringSimple(const Board& board, char lineDelimiter) {
  string s;
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      s += PlayerIO::colorToChar(board.colors[loc]);
    }
    s += lineDelimiter;
  }
  return s;
}

Board Board::parseBoard(int xSize, int ySize, const string& s) {
  return parseBoard(xSize,ySize,s,'\n');
}

Board Board::parseBoard(int xSize, int ySize, const string& s, char lineDelimiter) {
  Board board(xSize,ySize);
  vector<string> lines = Global::split(Global::trim(s),lineDelimiter);

  //Throw away coordinate labels line if it exists
  if(lines.size() == ySize+1 && Global::isPrefix(lines[0],"A"))
    lines.erase(lines.begin());

  if(lines.size() != ySize)
    throw StringError("Board::parseBoard - string has different number of board rows than ySize");

  for(int y = 0; y<ySize; y++) {
    string line = Global::trim(lines[y]);
    //Throw away coordinates if they exist
    size_t firstNonDigitIdx = 0;
    while(firstNonDigitIdx < line.length() && Global::isDigit(line[firstNonDigitIdx]))
      firstNonDigitIdx++;
    line.erase(0,firstNonDigitIdx);
    line = Global::trim(line);

    if(line.length() != xSize && line.length() != 2*xSize-1)
      throw StringError("Board::parseBoard - line length not compatible with xSize");

    for(int x = 0; x<xSize; x++) {
      char c;
      if(line.length() == xSize)
        c = line[x];
      else
        c = line[x*2];

      Loc loc = Location::getLoc(x,y,board.x_size);
      if(c == '.' || c == ' ' || c == '*' || c == ',' || c == '`')
        continue;
      else if(c == 'o' || c == 'O')
        board.setStone(loc,P_WHITE);
      else if(c == 'x' || c == 'X')
        board.setStone(loc,P_BLACK);
      else
        throw StringError(string("Board::parseBoard - could not parse board character: ") + c);
    }
  }
  return board;
}

nlohmann::json Board::toJson(const Board& board) {
  nlohmann::json data;
  data["xSize"] = board.x_size;
  data["ySize"] = board.y_size;
  data["stones"] = Board::toStringSimple(board,'|');
  data["koLoc"] = Location::toString(board.ko_loc,board);
  data["numBlackCaptures"] = board.numBlackCaptures;
  data["numWhiteCaptures"] = board.numWhiteCaptures;
  data["numBlackPasses"] = board.numBlackPasses;
  data["numWhitePasses"] = board.numWhitePasses;
  return data;
}

Board Board::ofJson(const nlohmann::json& data) {
  int xSize = data["xSize"].get<int>();
  int ySize = data["ySize"].get<int>();
  Board board = Board::parseBoard(xSize,ySize,data["stones"].get<string>(),'|');
  board.setSimpleKoLoc(Location::ofStringAllowNull(data["koLoc"].get<string>(),board));
  board.setBlackCaptures ( data["numBlackCaptures"].get<int>());
  board.setWhiteCaptures ( data["numWhiteCaptures"].get<int>());
  board.setBlackPasses ( data["numBlackPasses"].get<int>());
  board.setWhitePasses ( data["numWhitePasses"].get<int>());
  return board;
}

void Board::setBlackCaptures(int x)
{
  pos_hash ^= ZOBRIST_CAPTURE_B_HASH[numBlackCaptures];
  numBlackCaptures = x;
  pos_hash ^= ZOBRIST_CAPTURE_B_HASH[numBlackCaptures];
}

void Board::setWhiteCaptures(int x)
{
  pos_hash ^= ZOBRIST_CAPTURE_W_HASH[numWhiteCaptures];
  numWhiteCaptures = x;
  pos_hash ^= ZOBRIST_CAPTURE_W_HASH[numWhiteCaptures];
}

void Board::setBlackPasses(int x)
{
  pos_hash ^= ZOBRIST_PASSNUM_B_HASH[numBlackPasses];
  numBlackPasses = x;
  pos_hash ^= ZOBRIST_PASSNUM_B_HASH[numBlackPasses];
}

void Board::setWhitePasses(int x)
{
  pos_hash ^= ZOBRIST_PASSNUM_W_HASH[numWhitePasses];
  numWhitePasses = x;
  pos_hash ^= ZOBRIST_PASSNUM_W_HASH[numWhitePasses];
}

bool Board::isAdjacentToPlaHead(Player pla, Loc loc, Loc plaHead) const {
  FOREACHADJ(
    Loc adj = loc + ADJOFFSET;
    if(colors[adj] == pla && chain_head[adj] == plaHead)
      return true;
  );
  return false;
}

//Count empty spaces in the connected empty region containing initialLoc (which must be empty)
//not counting where emptyCounted == true, incrementing count each time and marking emptyCounted.
//Return early and true if count > bound. Return false otherwise.
bool Board::countEmptyHelper(bool* emptyCounted, Loc initialLoc, int& count, int bound) const {
  if(emptyCounted[initialLoc])
    return false;
  count += 1;
  emptyCounted[initialLoc] = true;
  if(count > bound)
    return true;

  int numLeft = 0;
  int numExpanded = 0;
  int toExpand[MAX_ARR_SIZE];
  toExpand[numLeft++] = initialLoc;
  while(numExpanded < numLeft) {
    Loc loc = toExpand[numExpanded++];
    FOREACHADJ(
      Loc adj = loc + ADJOFFSET;
      if(colors[adj] == C_EMPTY && !emptyCounted[adj]) {
        count += 1;
        emptyCounted[adj] = true;
        if(count > bound)
          return true;
        toExpand[numLeft++] = adj;
      }
    );
  }
  return false;
}
