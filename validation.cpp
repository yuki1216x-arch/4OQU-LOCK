#include <memory>
#include <iostream>
#include <vector>
#include <exception>
#include <cassert>
#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::vector;

constexpr int MAX_DEPTH = 168; // 168

const string base[2] = {"white_table", "black_table"};

constexpr unsigned long long int placement_count[2][17] {
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL}
};

class LocInfo_vali {
  char m_cube[2];
  char m_height;   // height
  char m_nb;       // number of black piece
  char m_nw;       // number of white piece
  char m_uk;
  char m_top;      // type of the top piece

public:
  LocInfo_vali(char c0, char c1, char height, char nb, char nw,
		   char uk, char top) noexcept
    : m_cube{c0, c1}, m_height(height), m_nb(nb), m_nw(nw), m_uk(uk), m_top(top) {
    assert(ok()); }
  
  bool is_ob() const noexcept { return m_cube[0] == 99; }
  
  int get_cube(int i) const noexcept {
    assert(i < 0 && 1 < i);
    return m_cube[i]; }
  int get_nb() const noexcept { return m_nb; }
  int get_nw() const noexcept { return m_nw; }
  int get_uk() const noexcept { return m_uk; }
  
  bool ok() const noexcept {
    // out of bounce
    if (m_cube[0] == 'N') {
	if (m_cube[1] != 'N') return false;
	if (m_top     != 'E') return false;
	if (m_height  != 99)  return false;
	if (m_nb      != 99)  return false;
	if (m_nw      != 99)  return false;
	if (m_uk      != 99)  return false; }
    // zero piece
    else if (m_cube[0] == '.') {
      if (m_cube[1] != '.') return false;
      if (m_top     != '.') return false;
      if (m_height    != 0)   return false;
      if (m_nb      != 0)   return false;
      if (m_nw      != 0)   return false;
      if (m_uk      != 0)   return false; }
    // one piece
    else if (m_cube[1] == '.') {
      if (m_top != m_cube[0]) return false;
      if (m_height != 1)        return false;
      if (m_cube[0] == 'W') {
	if (m_nb != 0) return false;
	if (m_nw != 1) return false;
	if (m_uk != 0) return false; }
      else if (m_cube[0] == 'B') {
	if (m_nb != 1) return false;
	if (m_nw != 0) return false;
	if (m_uk != 0) return false; }
      else if (m_cube[0] == 'U') {
	if (m_nb != 0) return false;
	if (m_nw != 0) return false;
	if (m_uk != 1) return false; }
      else return false; }
    // two pieces
    else if (m_cube[1] == 'W') {
      if (m_top != 'W') return false;
      if (m_height != 2)  return false;
      if (m_cube[0] == 'W') {
	if (m_nb != 0) return false;
	if (m_nw != 2) return false;
	if (m_uk != 0) return false; }
      else if (m_cube[0] == 'B') {
	if (m_nb != 1) return false;
	if (m_nw != 1) return false;
	if (m_uk != 0) return false; }
      else return false; }
    else if (m_cube[1] == 'B') {
      if (m_top != 'B') return false;
      if (m_height != 2)  return false;
      if (m_cube[0] == 'W') {
	if (m_nb != 1) return false;
	if (m_nw != 1) return false;
	if (m_uk != 0) return false; }
      else if (m_cube[0] == 'B') {
	if (m_nb != 2) return false;
	if (m_nw != 0) return false;
	if (m_uk != 0) return false; }
      else return false; }
    else if (m_cube[1] == 'N') {
      if (m_cube[0] != 'N') return false; }
    else return false;
    
    return true; }

  bool operator==(const LocInfo_vali &o) const noexcept {
    assert(o.ok());
    return (m_cube[0] == o.m_cube[0] && m_cube[1] == o.m_cube[1]); }
};

const LocInfo_vali tbl_objid2locinfo[8] =
  { LocInfo_vali('.', '.', 0, 0, 0, 0, '.'), //0
    LocInfo_vali('W', '.', 1, 0, 1, 0, 'W'), //1
    LocInfo_vali('B', '.', 1, 1, 0, 0, 'B'), //2
    LocInfo_vali('W', 'W', 2, 0, 2, 0, 'W'), //3
    LocInfo_vali('W', 'B', 2, 1, 1, 0, 'B'), //4
    LocInfo_vali('B', 'W', 2, 1, 1, 0, 'W'), //5
    LocInfo_vali('B', 'B', 2, 2, 0, 0, 'B'), //6
    LocInfo_vali('N', 'N', 99, 99, 99, 99, 'E')  //7  
  };

const LocInfo_vali tbl_objid2locinfo2[5] =
  { LocInfo_vali('.', '.', 0, 0, 0, 0, '.'), //0
    LocInfo_vali('W', '.', 1, 0, 1, 0, 'W'), //1
    LocInfo_vali('B', '.', 1, 1, 0, 0, 'B'), //2
    LocInfo_vali('U', '.', 1, 0, 0, 1, 'U'), //3
    LocInfo_vali('N', 'N', 99, 99, 99, 99, 'E')  //4  
  };

struct Node_vali {
  char nb;  // Number of black pieces up to the current node
  char nw;  // Number of white pieces up to the current node
  char uk;  // Number of unknown pieces up to the current node
  // whether an object is placed on this square(0/1)
  // at most one type of object can occupy a square
  char f;
  // whether a piece has been placed in the opponent's hand (0/1)
  // once placed, it will not become empty again
  char f1;
  // whether a piece has been placed in the player's hand (0/1)
  char f2;
  // whether an unknown piece has been placed in the player's hand (0/1)
  // after this, neither a white nor a black piece will be placed there.
  char f3;
  // Opponent's hand: left (set1), right (set2)
  // Player's hand: left (set3), right (set4)
  char nb_set1, nw_set1, nb_set2, nw_set2,
    nb_set3, nw_set3, nb_set4, nw_set4,
    nb_board, nw_board;
  char hand_id1, hand_id2; // variable that determines how many empty slots are allowed in the opponent/player's hand
  unsigned char locid;
  unsigned char loc_stateid;
};

// check whether a valid node exists when taking branch x(0/1) from node n
// d: depth of node n
bool IsNextLeaf0(const Node_vali* n, int depth, int x, int nmove, int vision) noexcept {
  assert(n);
  assert(0 <= depth && depth <= 167);
  assert(x == 0 || x == 1);
  assert(0 <= nmove && nmove < 16);
  int nhand = 16 - nmove;

  if(x == 1 && n->f == 1) return true; // Invalid: attempting to place an object on an already occupied square
  if(depth < 24) { // opponent's pieces in hand
    // Invalid: placing a non-empty value on squares beyond the current square number (remaining moves)
    if(x == 1 && n->loc_stateid != 0 && n->hand_id1 > nhand) return true;
    if(x == 1 && n->loc_stateid == 0 && n->hand_id1 <= nhand) return true;
    
    if(x == 1 && n->loc_stateid == 0 && n->f1 == 1) return true; // Invalid: placing an empty value after a piece has been placed in hand
    if(x == 0 && depth % 3 == 2 && n->f == 0) return true; // Invalid: value remains unset
    if(depth < 12) { // set1
      if(vision == 0) {
	// Invalid: exceeding the number of white pieces in set1
	if(x == 1 && n->loc_stateid == 1 && n->nw_set1 >= 1) return true;
	// Invalid: exceeding the number of black pieces in set1
	if(x == 1 && n->loc_stateid == 2 && n->nb_set1 >= 3) return true;
      } else {
	assert(vision == 1);
	// Invalid: exceeding the number of white pieces in set1
	if(x == 1 && n->loc_stateid == 1 && n->nw_set1 >= 3) return true;
	// Invalid: exceeding the number of black pieces in set1
	if(x == 1 && n->loc_stateid == 2 && n->nb_set1 >= 1) return true;
      }
    } else { // set2
      if(vision == 0) {
	// Invalid: exceeding the number of white pieces in set2
	if(x == 1 && n->loc_stateid == 1 && n->nw_set2 >= 1) return true;
	// Invalid: exceeding the number of black pieces in set2
	if(x == 1 && n->loc_stateid == 2 && n->nb_set2 >= 3) return true;
      } else {
	assert(vision == 1);
	// Invalid: exceeding the number of black pieces in set2
	if(x == 1 && n->loc_stateid == 1 && n->nw_set2 >= 3) return true;
	// Invalid: exceeding the number of white pieces in set2
	if(x == 1 && n->loc_stateid == 2 && n->nb_set2 >= 1) return true;
      }
    }
    return false;
  }
  if(depth < 136) { // on the board
    if(x == 0 && (depth - 24) % 7 == 6 && n->f == 0) return true; // Invalid: value remains unset

    // Invalid: placing more pieces than the limit
    if(vision == 0) {
      if(nhand > 13) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 6 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 6 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 6 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 6 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 6 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 2)) return true;
      } else if(nhand > 11) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 5 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 5 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 5 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 5 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 5 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 2)) return true;
      } else if(nhand > 9) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 4 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 4 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 4 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 4 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 4 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 2)) return true;
      } else if(nhand > 5) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 3 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 3 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 3 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 3 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 3 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 1)) return true;
      } else if(nhand > 3) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 1)) return true;
      } else if(nhand > 1) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 1)) return true;
      } else {
	assert(nhand >= 0);
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 0)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 0)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 0)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 0)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 0)) return true;
      }
    } else {
      assert(vision == 1);
      if(nhand > 12) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 6)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 6)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 6)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 6)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 6)) return true;
      } else if(nhand > 10) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 5)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 5)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 5)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 5)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 5)) return true;
      } else if(nhand > 8) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 4)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 4)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 4)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 4)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 2 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 4)) return true;
      } else if(nhand > 4) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 3)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 3)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 3)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 3)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 3)) return true;
      } else if(nhand > 2) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 2)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 2)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 2)) return true;
      } else if(nhand > 0) {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 1)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 1)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 1 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 1)) return true;
      } else {
	if((depth - 24) % 7 == 1 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 0)) return true;
	if((depth - 24) % 7 == 2 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 0)) return true;
	if((depth - 24) % 7 == 3 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 2) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board) < 0)) return true;
	if(((depth - 24) % 7 == 4 || (depth - 24) % 7 == 5) && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board + 1) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 1) < 0)) return true;
	if((depth - 24) % 7 == 6 && x == 1 && n->f == 0 &&
	   (8 - (n->nw_set1 + n->nw_set2 + n->nw_board) < 0 || 8 - (n->nb_set1 + n->nb_set2 + n->nb_board + 2) < 0)) return true;
      }
    }

    if(depth == 135) {
      if(vision == 0) {
	// Invalid: the number of black pieces exceeds that of white by two
	if(x == 1 && n->f == 0 && nhand > 8 && (n->nb_board + 2) - n->nw_board > 1 && n->nw_set1 == 0) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && n->nb_board - n->nw_board > 1 && n->nw_set1 == 0) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && (n->nb_board + 2) - n->nw_board > 1 && n->nw_set2 == 0) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && n->nb_board - n->nw_board > 1 && n->nw_set2 == 0) return true;
												    
	// Invalid: the number of white pieces exceeds that of black by two
	if(x == 1 && n->f == 0 && nhand > 8 && n->nw_board - (n->nb_board + 2) > 1 && n->nw_set1 == 1) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && n->nw_board - n->nb_board > 1 && n->nw_set1 == 1) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && n->nw_board - (n->nb_board + 2) > 1 && n->nw_set2 == 1) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && n->nw_board - n->nb_board > 1 && n->nw_set2 == 1) return true;

	// Invalid: incorrect number of opponent-colored pieces in the player's hand
	// (2 for remaining moves >9, otherwise 1)
	if(x == 1 && n->f == 0 && nhand > 9 && 8 - (n->nb_board + 2 + n->nb_set1 + n->nb_set2) > 2) return true;
	if(x == 0 && n->f == 1 && nhand > 9 && 8 - (n->nb_board + n->nb_set1 + n->nb_set2) > 2) return true;
	if(x == 1 && n->f == 0 && nhand <= 9 && 8 - (n->nb_board + 2 + n->nb_set1 + n->nb_set2) > 1) return true;
	if(x == 0 && n->f == 1 && nhand <= 9 && 8 - (n->nb_board + n->nb_set1 + n->nb_set2) > 1) return true;

	// Invalid: pieces on the board that do not exist in hand
	if(x == 1 && n->f == 0 && nhand > 8 && 1 - n->nw_set1 > n->nw_board) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && 1 - n->nw_set1 > n->nw_board) return true;
	if(x == 1 && n->f == 0 && nhand > 8 && 3 - n->nb_set1 > n->nb_board + 2) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && 3 - n->nb_set1 > n->nb_board) return true;
	
	if(x == 1 && n->f == 0 && nhand <= 8 && 4 + (1 - n->nw_set2) > n->nw_board) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && 4 + (1 - n->nw_set2) > n->nw_board) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && 4 + (3 - n->nw_set2) > n->nw_board + 2) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && 4 + (3 - n->nw_set2) > n->nw_board) return true;
      } else {
	assert(vision == 1);
	// Invalid: the number of black pieces exceeds that of white by two
	if(x == 1 && n->f == 0 && nhand > 8 && (n->nb_board + 2) - n->nw_board > 1 && n->nb_set1 == 1) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && n->nb_board - n->nw_board > 1 && n->nb_set1 == 1) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && (n->nb_board + 2) - n->nw_board > 1 && n->nb_set2 == 1) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && n->nb_board - n->nw_board > 1 && n->nb_set2 == 1) return true;
														    
	// Invalid: the number of white pieces exceeds that of black by two
	if(x == 1 && n->f == 0 && nhand > 8 && n->nw_board - (n->nb_board + 2) > 1 && n->nb_set1 == 0) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && n->nw_board - n->nb_board > 1 && n->nb_set1 == 0) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && n->nw_board - (n->nb_board + 2) > 1 && n->nb_set2 == 0) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && n->nw_board - n->nb_board > 1 && n->nb_set2 == 0) return true;

	// Invalid: incorrect number of opponent-colored pieces in the player's hand
	// (2 for remaining moves >8, otherwise 1)
	if(x == 1 && n->f == 0 && nhand > 8 && 8 - (n->nw_board + n->nw_set1 + n->nw_set2) > 2) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && 8 - (n->nw_board + n->nw_set1 + n->nw_set2) > 2) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && 8 - (n->nw_board + n->nw_set1 + n->nw_set2) > 1) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && 8 - (n->nw_board + n->nw_set1 + n->nw_set2) > 1) return true;

	// Invalid: pieces on the board that do not exist in hand
	if(x == 1 && n->f == 0 && nhand > 8 && 3 - n->nw_set1 > n->nw_board) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && 3 - n->nw_set1 > n->nw_board) return true;
	if(x == 1 && n->f == 0 && nhand > 8 && 1 - n->nb_set1 > n->nb_board + 2) return true;
	if(x == 0 && n->f == 1 && nhand > 8 && 1 - n->nb_set1 > n->nb_board) return true;

	if(x == 1 && n->f == 0 && nhand <= 8 && 4 + (3 - n->nw_set2) > n->nw_board) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && 4 + (3 - n->nw_set2) > n->nw_board) return true;
	if(x == 1 && n->f == 0 && nhand <= 8 && 4 + (1 - n->nw_set2) > n->nw_board + 2) return true;
	if(x == 0 && n->f == 1 && nhand <= 8 && 4 + (1 - n->nw_set2) > n->nw_board) return true;
      }

      // Invalid: (white - black) piece count on the board is out of bounds
      if(x == 1 && n->f == 0 && (n->nw_board - n->nb_board > 5 || n->nb_board - n->nw_board > 1)) return true;
      if(x == 0 && n->f == 1 && (n->nw_board - n->nb_board > 3 || n->nb_board - n->nw_board > 3)) return true;
    
      //TODO: verify if this check is necessary
      if(x == 1 && n->f == 0 && nhand == 8 && !(n->nw_board == 4 && n->nb_board == 2)) return true;
      if(x == 0 && n->f == 1 && nhand == 8 && !(n->nw_board == 4 && n->nb_board == 4)) return true;
    
      if(x == 1 && n->f == 0 && nhand > 8 && (n->nw_board > 4 || n->nb_board > 2)) return true;
      if(x == 0 && n->f == 1 && nhand > 8 && (n->nw_board > 4 || n->nb_board > 4)) return true;
      if(x == 1 && n->f == 0 && nhand < 8 && (n->nw_board < 4 || n->nb_board < 2)) return true;
      if(x == 0 && n->f == 1 && nhand < 8 && (n->nw_board < 4 || n->nb_board < 4)) return true;

      // TODO: verify if this check is necessary
      if(x == 1 && n->f == 0 && nhand > 8 && (n->nw_board + n->nw_set1 > 4 || n->nb_board + n->nb_set1 > 2)) return true;
      if(x == 0 && n->f == 1 && nhand > 8 && (n->nw_board + n->nw_set1 > 4 || n->nb_board + n->nb_set1 > 4)) return true;
    }
    return false;
  }
  // player's pieces in hand
  // Invalid: placing a non-empty value on squares beyond the current square number (remaining moves)
  if(x == 1 && n->loc_stateid != 0 && n->hand_id2 > nhand) return true;
  if(x == 1 && n->loc_stateid == 0 && n->hand_id2 <= nhand) return true;
  
  if(x == 1 && n->loc_stateid == 0 && n->f2 == 1) return true; // Invalid: placing an empty value after a piece has been placed in hand
  if(x == 1 && (n->loc_stateid == 1 || n->loc_stateid == 2) && n->f3 == 1) return true;// Invalid: placing a white or black piece after an unknown piece
  if(x == 1 && n->loc_stateid == 3 && n->f3 == 0) return true;// Invalid: placing an unknown piece before any white or black piece
  if(x == 0 && (depth - 136) % 4 == 3 && n->f == 0) return true; // Invalid: value remains unset
  if(depth < 152) { // set3
    if(vision == 0) {
      if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nw_board > 1) return true;
      if(x == 1 && n->loc_stateid == 2 && n->nb_board + n->nb_set1 + n->nb_set2 == 7) return true;
      
      // Invalid: exceeding the number of white pieces in set1
      if(x == 1 && n->loc_stateid == 1 && n->nw_set3 >= 3) return true;
      // Invalid: exceeding the number of black pieces in set1
      if(x == 1 && n->loc_stateid == 2 && n->nb_set3 >= 1) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(x == 1 && n->loc_stateid == 1 && n->nw_board - n->nw_set1 - n->nw_set2 >= 3 - n->nw_set3) return true;
      if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nb_set1 - n->nb_set2 >= 1 - n->nb_set3) return true;
    } else {
      assert(vision == 1);

      if(x == 1 && n->loc_stateid == 1 && n->nb_board - n->nw_board > 1) return true;
      if(x == 1 && n->loc_stateid == 1 && n->nb_board + n->nb_set1 + n->nb_set2 == 7) return true;
      
      // Invalid: exceeding the number of white pieces in set1
      if(x == 1 && n->loc_stateid == 1 && n->nw_set3 >= 1) return true;
      // Invalid: exceeding the number of black pieces in set1
      if(x == 1 && n->loc_stateid == 2 && n->nb_set3 >= 3) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(x == 1 && n->loc_stateid == 1 && n->nw_board - n->nw_set1 - n->nw_set2 >= 1 - n->nw_set3) return true;
      if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nb_set1 - n->nb_set2 >= 3 - n->nb_set3) return true;
    }
  } else { // set4
    if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nw_board > 1) return true;
    if(x == 1 && n->loc_stateid == 2 && n->nb_board + n->nb_set1 + n->nb_set2 == 8) return true;
    
    if(vision == 0) {
      // Invalid: exceeding the number of white pieces in set2
      if(x == 1 && n->loc_stateid == 1 && n->nw_set4 >= 3) return true;
      // Invalid: exceeding the number of black pieces in set2
      if(x == 1 && n->loc_stateid == 2 && n->nb_set4 >= 1) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(x == 1 && n->loc_stateid == 1 && n->nw_board - n->nw_set1 - n->nw_set2 >= 6 - n->nw_set4) return true;
      if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nb_set1 - n->nb_set2 >= 2 - n->nb_set4) return true;
    } else {
      assert(vision == 1);

      if(x == 1 && n->loc_stateid == 1 && n->nb_board - n->nw_board > 1) return true;
      if(x == 1 && n->loc_stateid == 1 && n->nb_board + n->nb_set1 + n->nb_set2 == 8) return true;
      
      // Invalid: exceeding the number of black pieces in set2
      if(x == 1 && n->loc_stateid == 1 && n->nw_set4 >= 1) return true;
      // Invalid: exceeding the number of white pieces in set2
      if(x == 1 && n->loc_stateid == 2 && n->nb_set4 >= 3) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(x == 1 && n->loc_stateid == 1 && n->nw_board - n->nw_set1 - n->nw_set2 >= 2 - n->nw_set4) return true;
      if(x == 1 && n->loc_stateid == 2 && n->nb_board - n->nb_set1 - n->nb_set2 >= 6 - n->nb_set4) return true;
    }
    // Invalid: total number of pieces exceeds the limit
    if(depth == 167 && x == 1 && n->f == 0 && !(n->nb + n->nw + n->uk == 15 && n->nb <= 8)) return true;
    if(depth == 167 && x == 0 && n->f == 1 && !(n->nb + n->nw + n->uk == 16 && n->nw <= 8)) return true;
  }
  return false;
}

void validate_zdd(Node_vali* cur, vector<char>& array_objid, const ZDD_base& zdd, int depth, unsigned long long int& leaf_id, int nmove, int vision) {
  if(cur == nullptr) return;

  if(depth == MAX_DEPTH) {
    assert(array_objid.size() == 32);
    unsigned char array_objid_zdd[32], array_objid_copy[32];

    if(leaf_id % 100000000 == 0) {
      cout << "ok! id = " << leaf_id << ": ";
      for(char x : array_objid) {
	cout << +x;
      }
      cout << endl;
    }
    
    zdd.compute_array(leaf_id, array_objid_zdd);
    for(size_t i = 0; i < array_objid.size(); i++) {
      if(array_objid[i] != array_objid_zdd[i]) {
	std::cerr << "zdd error, id = " << leaf_id << ", locid = " << i << endl;
	std::cerr << "dfp = " << +array_objid[i] << ", zdd = " << +array_objid_zdd[i] << endl;
	std::terminate();
      }
      array_objid_copy[i] = array_objid[i];
    }

    unsigned long long int array_id = zdd.compute_id(array_objid_copy);
    if(leaf_id != array_id ) {
      std::cerr << "id error, id = " << leaf_id << endl;
      std::terminate();
    }
    
    leaf_id++;
    return;
  }

  for(int x = 0; x < 2; x++) { // for each branch (0 and 1)
    if(IsNextLeaf0(cur, depth, x, nmove, vision)) { // if the x-branch leads to the 0-terminal node
      continue;
      // cout << depth << ", " << x << "false" << endl;
    } else { // if current node has children
      // cout << depth << ", " << x << "clear" << endl;
      Node_vali* c = new Node_vali;
      if(cur->locid < 8) {
	assert(depth >= 0 && depth < 24);
	if(cur->loc_stateid == 2) {
	  c->locid = cur->locid + 1;
	  c->loc_stateid = 0;
	  c->hand_id1 = cur->hand_id1 - 2;
	} else {
	  c->locid = cur->locid;
	  c->loc_stateid = cur->loc_stateid + 1;
	  c->hand_id1 = cur->hand_id1;
	}

	// c->f: propagate the parent's state; if x == 1, the parent's object is placed
	if(cur->loc_stateid != 2 && (cur->f == 1 || x == 1)) c->f = 1;
	else c->f = 0;

	// f1 = 1 when the leftmost piece in opponent's hand is placed
	if(cur->loc_stateid != 0 && x == 1) c->f1 = 1;
	else c->f1 = cur->f1;

	c->f2 = cur->f2;
	c->f3 = cur->f3;
	c->hand_id2 = cur->hand_id2;

	if(x == 1) {
	  if(cur->locid < 4) {
	    c->nw_set1 = cur->nw_set1 + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb_set1 = cur->nb_set1 + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->nw_set2 = cur->nw_set2;
	    c->nb_set2 = cur->nb_set2;
	    c->nw = cur->nw + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb = cur->nb + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	  } else {
	    c->nw_set1 = cur->nw_set1;
	    c->nb_set1 = cur->nb_set1;
	    c->nw_set2 = cur->nw_set2 + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb_set2 = cur->nb_set2 + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->nw = cur->nw + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb = cur->nb + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	  }
	} else {
	  c->nw_set1 = cur->nw_set1;
	  c->nb_set1 = cur->nb_set1;
	  c->nw_set2 = cur->nw_set2;
	  c->nb_set2 = cur->nb_set2;
	  c->nw = cur->nw;
	  c->nb = cur->nb;
	}

	c->uk = cur->uk;
	c->nw_set3 = cur->nw_set3;
	c->nb_set3 = cur->nb_set3;
	c->nw_set4 = cur->nw_set4;
	c->nb_set4 = cur->nb_set4;
	c->nw_board = cur->nw_board;
	c->nb_board = cur->nb_board;
	
      } else if(cur->locid < 24) {
	assert(depth >= 24 && depth < 136);
	if(cur->loc_stateid == 6) {
	  c->locid = cur->locid + 1;
	  c->loc_stateid = 0;
	} else {
	  c->locid = cur->locid;
	  c->loc_stateid = cur->loc_stateid + 1;
	}

	// c->f: propagate the parent's state; if x == 1, the parent's object is placed
	if(cur->loc_stateid != 6 && (cur->f == 1 || x == 1)) c->f = 1;
	else c->f = 0;

	c->f1 = cur->f1;
	c->f2 = cur->f2;
	c->f3 = cur->f3;
	c->hand_id1 = cur->hand_id1;
	c->hand_id2 = cur->hand_id2;
	c->uk = cur->uk;

	if(x == 1) {
	  c->nw = cur->nw + tbl_objid2locinfo[cur->loc_stateid].get_nw();
	  c->nb = cur->nb + tbl_objid2locinfo[cur->loc_stateid].get_nb();
	  c->nw_board = cur->nw_board + tbl_objid2locinfo[cur->loc_stateid].get_nw();
	  c->nb_board = cur->nb_board + tbl_objid2locinfo[cur->loc_stateid].get_nb();
	} else {
	  c->nw = cur->nw;
	  c->nb = cur->nb;
	  c->nw_board = cur->nw_board;
	  c->nb_board = cur->nb_board;
	}

	c->nw_set1 = cur->nw_set1;
	c->nb_set1 = cur->nb_set1;
	c->nw_set2 = cur->nw_set2;
	c->nb_set2 = cur->nb_set2;
	c->nw_set3 = cur->nw_set3;
	c->nb_set3 = cur->nb_set3;
	c->nw_set4 = cur->nw_set4;
	c->nb_set4 = cur->nb_set4;
        
      } else {
	if(cur->loc_stateid == 3) {
	  c->locid = cur->locid + 1;
	  c->loc_stateid = 0;
	  c->hand_id2 = cur->hand_id2 - 2;
	} else {
	  c->locid = cur->locid;
	  c->loc_stateid = cur->loc_stateid + 1;
	  c->hand_id2 = cur->hand_id2;
	}

	// c->f: propagate the parent's state; if x == 1, the parent's object is placed
	if(cur->loc_stateid != 3 && (cur->f == 1 || x == 1)) c->f = 1;
	else c->f = 0;

	// f2 = 1 when the leftmost piece in player's hand is placed
	if(cur->loc_stateid != 0 && x == 1) c->f2 = 1;
	else c->f2 = cur->f2;

	// once a white or black piece is placed in the player's hand, only unknown pieces may follow
	if(x == 1 && (cur->loc_stateid == 1 || cur->loc_stateid == 2)) c->f3 = 1;
	else c->f3 = cur->f3;

	c->f1 = cur->f1;
	c->hand_id1 = cur->hand_id1;

	if(x == 1) {
	  if(cur->locid < 28) {
	    c->nw_set3 = cur->nw_set3 + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb_set3 = cur->nb_set3 + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->nw_set4 = cur->nw_set4;
	    c->nb_set4 = cur->nb_set4;
	    c->nw = cur->nw + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb = cur->nb + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->uk = cur->uk + tbl_objid2locinfo2[cur->loc_stateid].get_uk();
	  } else {
	    c->nw_set3 = cur->nw_set3;
	    c->nb_set3 = cur->nb_set3;
	    c->nw_set4 = cur->nw_set4 + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb_set4 = cur->nb_set4 + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->nw = cur->nw + tbl_objid2locinfo2[cur->loc_stateid].get_nw();
	    c->nb = cur->nb + tbl_objid2locinfo2[cur->loc_stateid].get_nb();
	    c->uk = cur->uk + tbl_objid2locinfo2[cur->loc_stateid].get_uk();
	  }
	} else {
	  c->nw_set3 = cur->nw_set3;
	  c->nb_set3 = cur->nb_set3;
	  c->nw_set4 = cur->nw_set4;
	  c->nb_set4 = cur->nb_set4;
	  c->nw = cur->nw;
	  c->nb = cur->nb;
	  c->uk = cur->uk;
	}

	c->nw_set1 = cur->nw_set1;
	c->nb_set1 = cur->nb_set1;
	c->nw_set2 = cur->nw_set2;
	c->nb_set2 = cur->nb_set2;
	c->nw_board = cur->nw_board;
	c->nb_board = cur->nb_board;
      }

      if(x == 0) {
	validate_zdd(c, array_objid, zdd, depth + 1, leaf_id, nmove, vision); // left
	delete c;
      } else {
	array_objid.push_back(cur->loc_stateid);
	validate_zdd(c, array_objid, zdd, depth + 1, leaf_id, nmove, vision); // right
	delete c;
	array_objid.pop_back();
      }
    }
  }
  return;
}

bool detect_invalid_transition_player(int parent_val, int child_val[1000][12], int nchild, int num_of_un, int iteration, int vision) {
  assert(parent_val == v_win || parent_val == v_draw || parent_val == v_notlose);

  assert(num_of_un > 0 && num_of_un <= 12);
  assert(nchild > 0 && nchild < 1000);
  
  if(parent_val == v_win) { // win
    if(iteration % 2 == vision) {
      for(int j = 0; j < nchild; j++) {
	int num_win = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_win) num_win++;
	}
	if(num_win == num_of_un) return false;
      }
      return true;
    } else {
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] != v_win) return true;
	}
      }
      return false;
    }
  } else { // not lose or draw
    assert(parent_val == v_notlose || parent_val == v_draw);
    if(iteration % 2 == vision) {
      for(int j = 0; j < nchild; j++) {
	int num_win = 0, num_notlose = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_win) num_win++, num_notlose++;
	  else if(child_val[j][m] == v_notlose || child_val[j][m] == v_draw) num_notlose++;
	}
	if(num_win != num_of_un && num_notlose == num_of_un) return false;
      }
      return true;
    } else {
      int num_win = 0;
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] != v_win && child_val[j][m] != v_notlose && child_val[j][m] != v_draw) {
	    return true;
	  }
	  if(child_val[j][m] == v_win) num_win++;
	}
      }
      if(num_win == nchild * num_of_un) {
	return true;
      }
      return false;
    }
  }
}

bool detect_invalid_transition_opponent(int parent_val, int child_val[12][1000][12], int num_of_un, int nchild_opp, int num_of_un_opp, int iteration, int vision) {
  assert(parent_val == v_lose || parent_val == v_draw || parent_val == v_notwin);

  assert(num_of_un > 0 && num_of_un <= 12);
  assert(nchild_opp > 0 && nchild_opp < 1000);
  assert(num_of_un_opp > 0 && num_of_un_opp <= 12);
  
  if(parent_val == v_lose) { // lose
    int num_lose = 0;
    for(int j = 0; j < num_of_un; j++) {
      bool is_lose = true;
      for(int k = 0; k < nchild_opp; k++) {
	for(int m = 0; m < num_of_un_opp; m++) {
	  if(child_val[j][k][m] != v_lose){
	    is_lose = false;
	    break;
	  }
	}
	if(iteration % 2 != vision) {
	  if(is_lose) {
	    num_lose++;
	    break;
	  }
	  is_lose = true;
	} else {
	  if(!is_lose) return true;
	}
      }
    }
    if(iteration % 2 != vision) {
      if(num_lose == num_of_un) return false;
      else return true;
    }
    return false;
  } else { // not win or draw
    assert(parent_val == v_notwin || parent_val == v_draw);
    int num_notwin = 0;
    for(int j = 0; j < num_of_un; j++) {
      int count_notwin = 0, count_lose = 0;
      for(int k = 0; k < nchild_opp; k++) {
	bool is_notwin = true, is_lose = true;
	for(int m = 0; m < num_of_un_opp; m++) {
	  int x = child_val[j][k][m];
	  if(x != v_lose) {
	    is_lose = false;
	    if(x != v_draw && x != v_notwin) {
	      is_notwin = false;
	      break;
	    }
	  }
	}
	if(is_lose) count_lose++;
	if(is_notwin) count_notwin++;
      }
      if(iteration % 2 != vision) {
	if(count_notwin > 0 && count_lose == 0) num_notwin++;
      } else {
	if(count_notwin == nchild_opp && count_lose != nchild_opp) num_notwin++;
      }
    }
    if(num_notwin == num_of_un) return false;
    return true;
  }
}

int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]); // number of moves
  int vision = atoi(argv[2]); // player perspective (0: white, 1: black)
  string base_filename = base[vision];
  string base_filename_opp = base[1-vision];
  string read_filename_str = base_filename + "_" + std::to_string(iteration) + ".bin";
  string read_filename_str_child = base_filename + "_" + std::to_string(iteration+1) + ".bin";
  string read_filename_str_child_opp = base_filename_opp + "_" + std::to_string(iteration+1) + ".bin";
  const char* read_filename_c_str = read_filename_str.c_str();
  const char* read_filename_c_str_child = read_filename_str_child.c_str();
  const char* read_filename_c_str_child_opp = read_filename_str_child_opp.c_str();
  unsigned long long int parent_table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL; // parent table size
  unsigned long long int child_table_size64 = (placement_count[vision][15 - iteration] + 15ULL) / 16ULL; // child table size
  unsigned long long int opp_child_table_size64 = (placement_count[1 - vision][15 - iteration] + 15ULL) / 16ULL; // child table size (opponent's perspective)
  unsigned long long int id = 0;

  unique_ptr<ZDD_base> zdd_parent;
  unique_ptr<ZDD_base> zdd_child;
  unique_ptr<ZDD_base> zdd_child_opp;
  if(vision == 0) {
    zdd_parent = make_unique<ZDD_White>(iteration);
    if(iteration == 15) {
      zdd_child = make_unique<ZDD_White>(iteration);
      zdd_child_opp = make_unique<ZDD_Black>(iteration);
    } else {
      zdd_child = make_unique<ZDD_White>(iteration + 1);
      zdd_child_opp = make_unique<ZDD_Black>(iteration + 1);
    }
  } else {
    zdd_parent = make_unique<ZDD_Black>(iteration);
    if(iteration == 15) {
      zdd_child = make_unique<ZDD_Black>(iteration);
      zdd_child_opp = make_unique<ZDD_White>(iteration);
    } else {
      zdd_child = make_unique<ZDD_Black>(iteration + 1);
      zdd_child_opp = make_unique<ZDD_White>(iteration + 1);
    }
  }
  
  Node_vali* root = new Node_vali;
  root->nb = 0;
  root->nw = 0;
  root->uk = 0;
  root->f = 0, root->f1 = 0, root->f2 = 0, root->f3 = 0;
  root->nw_set1 = 0, root->nb_set1 = 0, root->nw_set2 = 0, root->nb_set2 = 0;
  root->nw_set3 = 0, root->nb_set3 = 0, root->nw_set4 = 0, root->nb_set4 = 0;
  root->nw_board = 0, root->nb_board = 0;
  root->locid = 0;
  root->loc_stateid = 0;
  if(vision == 0) {
    root->hand_id1 = 15;
    root->hand_id2 = 16;
  } else {
    root->hand_id1 = 16;
    root->hand_id2 = 15;
  }
  
  vector<char> array_objid;
  
  //validate_zdd(root, array_objid, *zdd_parent, 0, id, iteration, vision);

  cout << "leaf num: " << id << endl;

  if(vision == 0) cout << "zdd is ok. (iteration : " << iteration << ", vision : white)" << endl;
  else cout << "zdd is ok. (iteration : " << iteration << ", vision: black)" << endl;

  // from here, verify parent-child relationships
  Table parent_table(16 - iteration, read_filename_c_str, "db", parent_table_size64, placement_count[vision][16 - iteration]); // parent talbe
  Table child_table(15 - iteration, read_filename_c_str_child, "db", child_table_size64, placement_count[vision][15 - iteration]); // child table
  Table opp_child_table(15 - iteration, read_filename_c_str_child_opp, "db", opp_child_table_size64, placement_count[1 - vision][15 - iteration]); // child table (opponent's perspective)
  Posi p;
  bool is_error = false;

  for(unsigned long long int i = 0ULL; i < placement_count[vision][16 - iteration]; i++) {
    p.make_posi(i, *zdd_parent);
    Action actions[1000] = {};
    unsigned char board_belief[12][46] = {};
    int nchild = p.compute_actions(actions, vision, iteration);
    int num_of_un = p.getunknowninfo(board_belief, vision, iteration);
    int nchild_opp = 0, num_of_un_opp = 0;
    
    int parent_val = parent_table.get(i);
    if(parent_val <= 0 || parent_val > 5) std::cout << "parent = " << parent_val << endl;
    assert(parent_val > 0 && parent_val <= 5);

    if(nchild > 0) {
      if(parent_val == v_win || parent_val == v_notlose || parent_val == v_draw) {
	int child_val[nchild][12] = {};
	for(int j = 0; j < nchild; j++) {
	  for(int m = 0; m < num_of_un; m++) {
	    p.make_posi_n(board_belief, m);
	    int board_check = p.make_action(actions[j], vision, iteration);
	    if(board_check == 0) { // Non-terminal node
	      p.make_posi_myself();
	      child_val[j][m] = child_table.get(p.getzddnum(*zdd_child)); // retrieve the value from the child's table
	    } else if(board_check == 2) child_val[j][m] = v_win; // win (player's perspective)
	    else if(board_check == 3) child_val[j][m] = v_lose; // lose (player's perspective)
	    else {
	      assert(board_check == 10); // draw
	      child_val[j][m] = v_draw;
	    }
	  }
	  p.make_posi(i, *zdd_parent); //undo
	}
	if(detect_invalid_transition_player(parent_val, child_val, nchild, num_of_un, iteration, vision)) {
	  is_error = true;
	  std::cout << "label error1, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  for(int j = 0; j < nchild; j++) {
	    for(int m = 0; m < num_of_un; m++) {
	      std::cout << child_val[j][m];
	    }
	    if(j != nchild - 1) std::cerr << " ";
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd_parent);
	  p.print();
	  break;
	}
      }
      if(parent_val == v_lose || parent_val == v_notwin || parent_val == v_draw) {
	int child_val_opp[num_of_un][1000][12] = {};
	for(int j = 0; j < num_of_un; j++) {
	  Action actions_opp[1000] = {};
	  unsigned char board_belief_opp[12][46] = {};
	  p.make_posi_n(board_belief, j);
	  p.make_posi_opponent();
	  nchild_opp = p.compute_actions(actions_opp, 1 - vision, iteration);
	  num_of_un_opp = p.getunknowninfo(board_belief_opp, 1 - vision, iteration);
	  assert(nchild_opp < 1000 && num_of_un_opp <= 12);
	  for(int k = 0; k < nchild_opp; k++) {
	    for(int m = 0; m < num_of_un_opp; m++) {
	      p.make_posi_n(board_belief_opp, m);
	      int board_check = p.make_action(actions_opp[k], 1 - vision, iteration);
	      if(board_check == 0) { // Non-terminal node
		p.make_posi_opponent();
		child_val_opp[j][k][m] = opp_child_table.get(p.getzddnum(*zdd_child_opp)); // retrieve the value from the child's table
	      } else if(board_check == 2) child_val_opp[j][k][m] = v_lose; // lose(opponent's perspective)
	      else if(board_check == 3) child_val_opp[j][k][m] = v_win; // win(opponent's perspective)
	      else {
		assert(board_check == 10); // draw
		child_val_opp[j][k][m] = v_draw;
	      }
	    }
	    p.make_posi(i, *zdd_parent);
	  }
	}
	if(detect_invalid_transition_opponent(parent_val, child_val_opp, num_of_un, nchild_opp, num_of_un_opp, iteration, vision)) {
	  is_error = true;
	  std::cout << "label error2, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  for(int j = 0; j < num_of_un; j++) {
	    for(int k = 0; k < nchild_opp; k++) {
	      for(int m = 0; m < num_of_un_opp; m++) {
		std::cout << child_val_opp[j][k][m];
	      }
	      if(k != nchild_opp - 1) std::cerr << " ";	      
	    }
	    if(j != num_of_un - 1) std::cerr << ": ";
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd_parent);
	  p.print();
	  break;
	}
      }
    } else if(nchild == -1) {
      if(parent_val != 1) {
	is_error = true;
	std::cout << "terminal label error1, id = " << i << ", parent = " << parent_val << ", nchild = "<< nchild << endl;
	p.make_posi(i, *zdd_parent);
	p.print();
	std::cout << "vision = " << vision << ", iteration = " << iteration << endl;
	break;
      }
    } else if(nchild == -2) {
      if(parent_val != 2) {
	is_error = true;
	std::cout << "terminal label error2, id = " << i << ", parent = " << parent_val << ", nchild = " << nchild << endl;
	p.make_posi(i, *zdd_parent);
	p.print();
	break;
      }
    } else {
      is_error = true;
      std::cout << "value error: exception, id =" << i << endl;
      std::cout << "nchild = " << nchild << ", parent = " << parent_val << endl;
      p.make_posi(i, *zdd_parent);
      p.print();
      break;
    }
    if(i % 100000000 == 0) std::cout << "label ok! id = " << i << endl;
  }
  if(is_error) std::terminate();
  return 0;
}
