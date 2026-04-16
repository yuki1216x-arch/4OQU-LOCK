#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include <exception>

class LocInfo_vali {
  char m_cube[2];
  char m_height; // height
  char m_nb; // number of black piece
  char m_nw; // number of white piece
  char m_uk;
  char m_top; // type of the top piece

public:
  LocInfo_vali(char c0, char c1, char height, char nb, char nw,
	       char uk, char top) noexcept
    : m_cube{c0, c1}, m_height(height), m_nb(nb), m_nw(nw), m_uk(uk), m_top(top) {
      assert(ok()); }
  
  bool is_ob() const noexcept { return m_cube[0] == 99; }
  
  int get_cube(int i) const noexcept {
    assert(i == 0 && i == 1);
    return m_cube[i]; }
  int get_nb() const noexcept { return m_nb; }
  int get_nw() const noexcept { return m_nw; }
  int get_uk() const noexcept { return m_uk; }
  
  bool ok() const noexcept {
    // out of bounce
    if(m_cube[0] == 'N') {
      if(m_cube[1] != 'N') return false;
      if(m_top     != 'E') return false;
      if(m_height  != 99)  return false;
      if(m_nb      != 99)  return false;
      if(m_nw      != 99)  return false;
      if(m_uk      != 99)  return false; }
    // zero piece
    else if(m_cube[0] == '.') {
      if(m_cube[1] != '.') return false;
      if(m_top     != '.') return false;
      if(m_height  != 0)   return false;
      if(m_nb      != 0)   return false;
      if(m_nw      != 0)   return false;
      if(m_uk      != 0)   return false; }
    // one piece
    else if(m_cube[1] == '.') {
      if(m_top    != m_cube[0]) return false;
      if(m_height != 1)         return false;
      if(m_cube[0] == 'W') {
	if(m_nb != 0) return false;
	if(m_nw != 1) return false;
	if(m_uk != 0) return false; }
      else if(m_cube[0] == 'B') {
	if(m_nb != 1) return false;
	if(m_nw != 0) return false;
	if(m_uk != 0) return false; }
      else if(m_cube[0] == 'U') {
	if(m_nb != 0) return false;
	if(m_nw != 0) return false;
	if(m_uk != 1) return false; }
      else return false; }
    // two piece
    else if(m_cube[1] == 'W') {
      if(m_top    != 'W') return false;
      if(m_height != 2)   return false;
      if(m_cube[0] == 'W') {
	if(m_nb != 0) return false;
	if(m_nw != 2) return false;
	if(m_uk != 0) return false; }
      else if(m_cube[0] == 'B') {
	if(m_nb != 1) return false;
	if(m_nw != 1) return false;
	if(m_uk != 0) return false; }
      else return false; }
    else if(m_cube[1] == 'B') {
      if(m_top    != 'B') return false;
      if(m_height != 2)   return false;
      if(m_cube[0] == 'W') {
	if(m_nb != 1) return false;
	if(m_nw != 1) return false;
	if(m_uk != 0) return false; }
      else if(m_cube[0] == 'B') {
	if(m_nb != 2) return false;
	if(m_nw != 0) return false;
	if(m_uk != 0) return false; }
      else return false; }
    else if(m_cube[1] == 'N') {
      if(m_cube[0] != 'N') return false; }
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
    LocInfo_vali('N', 'N', 99, 99, 99, 99, 'E') // 7
};

const LocInfo_vali tbl_objid2locinfo2[5] =
  { LocInfo_vali('.', '.', 0, 0, 0, 0, '.'), //0
    LocInfo_vali('W', '.', 1, 0, 1, 0, 'W'), //1
    LocInfo_vali('B', '.', 1, 1, 0, 0, 'B'), //2
    LocInfo_vali('U', '.', 1, 0, 0, 1, 'U'), //3
    LocInfo_vali('N', 'N', 99, 99, 99, 99, 'E') //4
};


class Node {
  char nb; // Number of black pieces up to the current node
  char nw; // Number of white pieces up to the current node
  char uk; // Number of unknown pieces up to the current node
  // whether an object is placed on this square (0/1)
  // at most one type of object can occupy a square
  char f;
  // whether a piece has been placed in the opponent's hand (0/1)
  // once placed, it will not become empty again
  char f1;
  // whether a piece has been placed in the player's hand (0/1)
  char f2;
  // whether an unknown piece has been placed in the player's hand (0/1)
  // after this, neither a white nor a black piece will be placed there
  char f3;
  // Opponent's hand: left (set1), right (set2)
  // Player's hand: left (set3), right (set4)
  char nb_set1, nw_set1, nb_set2, nw_set2,
    nb_set3, nw_set3, nb_set4, nw_set4,
    nb_board, nw_board;
  char hand_id1, hand_id2; // variable that determines how many empty slots are allowed in the opponent/player's hand
  unsigned char locid;
  unsigned char loc_stateid;

  virtual int nw_set12_min() const noexcept { return -1; }
  virtual int nb_set12_min() const noexcept { return -1; }
  virtual bool ok_opp_set1_num_gb() const noexcept { return true; }
  virtual bool ok_opp_set1_num_gw() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gb() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gw() const noexcept { return true; }
  virtual int set12_board_num_x1() const noexcept { return -1; }
  virtual int set12_board_num_x0() const noexcept { return -1; }
  virtual int sub_wb_two_nhand() const noexcept { return -1; }
  virtual int sub_wb() const noexcept { return -1; }
  virtual int opp_col() const noexcept { return -1; }
  virtual int get_hand_id1() const noexcept { return -1; }
  virtual int get_hand_id2() const noexcept { return -1; }

public:
  Node() : nb(0), nw(0), uk(0), f(0), f1(0), f2(0), f3(0),
    nw_set1(0), nb_set1(0), nw_set2(0), nb_set2(0),
    nw_set3(0), nb_set3(0), nw_set4(0), nb_set4(0),
    nw_board(0), nb_board(0), locid(0), loc_stateid(0) {}
  Node(const Node& cur, int x) {
    if(cur.locid < 8) {
      if(cur.loc_stateid == 2) {
	locid = cur.locid + 1;
	loc_stateid = 0;
      } else {
	locid = cur.locid;
	loc_stateid = cur.loc_stateid + 1;
      }

      // f: propagate the parent's state; if x == 1, the parent's object is placed
      if(cur.loc_stateid != 2 && (cur.f == 1 || x == 1)) f = 1;
      else f = 0;

      // f1 = 1 when the leftmost piece in opponent's hand is placed
      if(cur.loc_stateid != 0 && x == 1) f1 = 1;
      else f1 = cur.f1;

      f2 = cur.f2;
      f3 = cur.f3;

      if(x == 1) {
	if(cur.locid < 4) {
	  nw_set1 = cur.nw_set1 + tbl_objid2locinfo2[cur.loc_stateid].get_nw();
	  nb_set1 = cur.nb_set1 + tbl_objid2locinfo2[cur.loc_stateid].get_nb();
	  nw_set2 = cur.nw_set2;
	  nb_set2 = cur.nb_set2;
	} else {
	  nw_set1 = cur.nw_set1;
	  nb_set1 = cur.nb_set1;
	  nw_set2 = cur.nw_set2 + tbl_objid2locinfo2[cur.loc_stateid].get_nw();
	  nb_set2 = cur.nb_set2 + tbl_objid2locinfo2[cur.loc_stateid].get_nb();
	}
	nw = cur.nw + tbl_objid2locinfo2[cur.loc_stateid].get_nw();
	nb = cur.nb + tbl_objid2locinfo2[cur.loc_stateid].get_nb();
      } else {
	nw_set1 = cur.nw_set1;
	nb_set1 = cur.nb_set1;
	nw_set2 = cur.nw_set2;
	nb_set2 = cur.nb_set2;
	nw = cur.nw;
	nb = cur.nb;
      }

      uk = cur.uk;
      nw_set3 = cur.nw_set3;
      nb_set3 = cur.nb_set3;
      nw_set4 = cur.nw_set4;
      nb_set4 = cur.nb_set4;
      nw_board = cur.nw_board;
      nb_board = cur.nb_board;
      
    } else if(cur.locid < 24) {
      if(cur.loc_stateid == 6) {
	locid = cur.locid + 1;
	loc_stateid = 0;
      } else {
	locid = cur.locid;
	loc_stateid = cur.loc_stateid + 1;
      }

      // f: propagate the parent's state; if x == 1, the parent's object is placed
      if(cur.loc_stateid != 6 && (cur.f == 1 || x == 1)) f = 1;
      else f = 0;

      f1 = cur.f1;
      f2 = cur.f2;
      f3 = cur.f3;
      uk = cur.uk;

      if(x == 1) {
	nw = cur.nw + tbl_objid2locinfo[cur.loc_stateid].get_nw();
	nb = cur.nb + tbl_objid2locinfo[cur.loc_stateid].get_nb();
	nw_board = cur.nw_board + tbl_objid2locinfo[cur.loc_stateid].get_nw();
	nb_board = cur.nb_board + tbl_objid2locinfo[cur.loc_stateid].get_nb();
      } else {
	nw = cur.nw;
	nb = cur.nb;
	nw_board = cur.nw_board;
	nb_board = cur.nb_board;
      }

      nw_set1 = cur.nw_set1;
      nb_set1 = cur.nb_set1;
      nw_set2 = cur.nw_set2;
      nb_set2 = cur.nb_set2;
      nw_set3 = cur.nw_set3;
      nb_set3 = cur.nb_set3;
      nw_set4 = cur.nw_set4;
      nb_set4 = cur.nb_set4;
      
    } else {
      if(cur.loc_stateid == 3) {
	locid = cur.locid + 1;
	loc_stateid = 0;
      } else {
	locid = cur.locid;
	loc_stateid = cur.loc_stateid + 1;
      }

      // f: propagate the parent's state; if x == 1, the parent's object is placed
      if(cur.loc_stateid != 3 && (cur.f == 1 || x == 1)) f = 1;
      else f = 0;

      // f2 = 1 when the leftmost piece in player's hand is placed
      if(cur.loc_stateid != 0 && x == 1) f2 = 1;
      else f2 = cur.f2;

      // once a white or black piece is placed in the player's hand, only unknown pieces may follow
      if(x == 1 && (cur.loc_stateid == 1 || cur.loc_stateid == 2)) f3 = 1;
      else f3 = cur.f3;

      f1 = cur.f1;

      if(x == 1) {
	if(cur->locid < 28) {
	  nw_set3 = cur.nw_set3 + tbl_objid2locinfo2[cur.loc_stateid].get_nw();
	  nb_set3 = cur.nb_set3 + tbl_objid2locinfo2[cur.loc_stateid].get_nb();
	  nw_set4 = cur.nw_set4;
	  nb_set4 = cur.nb_set4;
	} else {
	  nw_set3 = cur.nw_set3;
	  nb_set3 = cur.nb_set3;
	  nw_set4 = cur.nw_set4 + tbl.objid2locinfo2[cur.loc_stateid].get_nw();
	  nb_set4 = cur.nb_set4 + tbl.objid2locinfo2[cur.loc_stateid].get_nb();
	}
	nw = cur.nw + tbl_objid2locinfo2[cur.loc_stateid].get_nw();
	nb = cur.nb + tbl_objid2locinfo2[cur.loc_stateid].get_nb();
	uk = cur.uk + tbl_objid2locinfo2[cur.loc_stateid].get_uk();
      } else {
	nw_set3 = cur.nw_set3;
	nb_set3 = cur.nb_set3;
	nw_set4 = cur.nw_set4;
	nb_set4 = cur.nb_set4;
	nw = cur.nw;
	nb = cur.nb;
	uk = cur.uk;
      }

      nw_set1 = cur.nw_set1;
      nb_set1 = cur.nb_set1;
      nw_set2 = cur.nw_set2;
      nb_set2 = cur.nb_set2;
      nw_board = cur.nw_board;
      nb_board = cur.nb_board;
    }
  }
  virtual ~Node() {}
  bool IsNextLeaf0(int d, int x, int nmove) const noexcept;
};

class Node_Black : public Node {
  char hand_id1, hand_id2; // variable that determines how many empty slots are allowed in the opponent/player's hand
  int nw_set12_min() const noexcept override { return 3; }
  int nb_set12_min() const noexcept override { return 1; }
  bool ok_opp_set1_num_gb() const noexcept override { return nb_set1 == 1; }
  bool ok_opp_set1_num_gw() const noexcept override { return nb_set1 == 0; }
  bool ok_opp_set2_num_gb() const noexcept override { return nb_set2 == 1; }
  bool ok_opp_set2_num_gw() const noexcept override { return nb_set2 == 0; }
  int set12_board_num_x1() const noexcept override { return nw_board + nw_set1 + nw_set2; }
  int set12_board_num_x0() const noexcept override { return nw_board + nw_set1 + nw_set2; }
  int sub_wb_two_nhand() const noexcept override { return 9; }
  int sub_wb() const noexcept override { return nw_board - nb_board; }
  int opp_col() const noexcept override { return 1; }
  int get_hand_id1() const noexcept override { return 16; }
  int get_hand_id2() const noexcept override { return 15; }

public:
  Node_Black() : Node(), hand_id1(16), hand_id2(15) {}
  Node_Black(const Node_Black& cur, int x) : Node(cur, x) {
    if(cur.locid < 8) {
      if(cur.loc_stateid == 2) hand_id1 = cur.hand_id1 - 2;
      else hand_id1 = cur.hand_id1;

      hand_id2 = cur.hand_id2;
      
    } else if(cur.locid < 24) {
      hand_id1 = cur.hand_id1;
      hand_id2 = cur.hand_id2;
      
    } else {
      if(cur.loc_stateid == 3) hand_id2 = cur.hand_id2 - 2;
      else hand_id2 = cur.hand_id2;

      hand_id1 = cur.hand_id1;
    }
  }
    virtual ~Node_Black() noexcept {}
};

class Node_White : public Node {
  char hand_id1, hand_id2; // variable that determines how many empty slots are allowed in the opponent/player's hand
  int nw_set12_min() const noexcept override { return 1; }
  int nb_set12_min() const noexcept override { return 3; }
  bool ok_opp_set1_num_gb() const noexcept override { return nw_set1 == 0; }
  bool ok_opp_set1_num_gw() const noexcept override { return nw_set1 == 1; }
  bool ok_opp_set2_num_gb() const noexcept override { return nw_set2 == 0; }
  bool ok_opp_set2_num_gw() const noexcept override { return nw_set2 == 1; }
  int set12_board_num_x1() const noexcept override { return nb_board + nb_set1 + nb_set2 + 2; }
  int set12_board_num_x0() const noexcept override { return nb_board + nb_set1 + nb_set2; }
  int sub_wb_two_nhand() const noexcept override { return 10; }
  int sub_wb() const noexcept override { return nb_board - nw_board; }
  int opp_col() const noexcept override { return 2; }
  int get_hand_id1() const noexcept override { return 15; }
  int get_hand_id2() const noexcept override { return 16; }

public:
  Node_White() : Node(), hand_id1(15), hand_id(16) {}
  Node_White(const Node_White& cur, int x) : Node(cur, x) {
    if(cur.locid < 8) {
      if(cur.loc_stateid == 2) hand_id1 = cur.hand_id1 - 2;
      else hand_id1 = cur.hand_id1;

      hand_id2 = cur.hand_id2;
      
    } else if(cur.locid < 24) {
      hand_id1 = cur.hand_id1;
      hand_id2 = cur.hand_id2;

    } else {
      if(cur.loc_stateid == 3) hand_id2 = cur.hand_id2 - 2;
      else hand_id2 = cur.hand_id2;

      hand_id1 = cur.hand_id1;
    }
  }
  virtual ~Node_White() noexcept {}
};

#endif
