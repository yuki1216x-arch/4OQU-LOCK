#ifndef INCLUDE_NODE_H
#define INCLUDE_NODE_H

#include <exception>
#include <memory>
#include <algorithm>
#include <iostream>
#include <cassert>

using std::cout;
using std::endl;
using std::max;
using std::min;
using std::unique_ptr;

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
  
  bool ok() const noexcept;
  
  bool operator==(const LocInfo_vali &o) const noexcept {
    assert(o.ok());
    return (m_cube[0] == o.m_cube[0] && m_cube[1] == o.m_cube[1]); }
};

class Node_base {
  char m_nb; // Number of black pieces up to the current node
  char m_nw; // Number of white pieces up to the current node
  char m_uk; // Number of unknown pieces up to the current node
  // whether an object is placed on this square (0/1)
  // at most one type of object can occupy a square
  char m_f;
  // whether a piece has been placed in the opponent's hand (0/1)
  // once placed, it will not become empty again
  char m_f1;
  // whether a piece has been placed in the player's hand (0/1)
  char m_f2;
  // whether an unknown piece has been placed in the player's hand (0/1)
  // after this, neither a white nor a black piece will be placed there
  char m_f3;
  // Opponent's hand: left (set1), right (set2)
  // Player's hand: left (set3), right (set4)
  char m_nb_set1, m_nw_set1, m_nb_set2, m_nw_set2,
    m_nb_set3, m_nw_set3, m_nb_set4, m_nw_set4,
    m_nb_board, m_nw_board;
  char m_hand_id1, m_hand_id2; // variable that determines how many empty slots are allowed in the opponent/player's hand
  unsigned char m_locid;
  unsigned char m_loc_stateid;
  unsigned long long int m_num; // number of reachable leaf nodes with value 1 (number of possible board configurations)
  bool m_visited; // for dfs to compute path length
  int m_lengthmax; // holds the maximum path length value
  int m_lengthmin; // holds the minimum path length value
  unsigned long long int m_lengthsum; // total path length for computing the average path length

  virtual int nw_set12_min() const noexcept { return -1; }
  virtual int nb_set12_min() const noexcept { return -1; }
  virtual bool ok_opp_set1_num_gb() const noexcept { return true; }
  virtual bool ok_opp_set1_num_gw() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gb() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gw() const noexcept { return true; }
  virtual int set12_board_num_x1() const noexcept { return -1; }
  virtual int set12_board_num_x0() const noexcept { return -1; }
  virtual int set12_board_num_player(int objid) const noexcept { return -1; }
  virtual int set12_board_num_opponent(int objid) const noexcept { return -1; }
  virtual int sub_wb_two_nhand() const noexcept { return -1; }
  virtual int sub_wb() const noexcept { return -1; }
  virtual int opp_col() const noexcept { return -1; }
  virtual int get_hand_id1() const noexcept { return -1; }
  virtual int get_hand_id2() const noexcept { return -1; }
  virtual void update_hand_id1() noexcept {}
  virtual void update_hand_id2() noexcept {}

protected:
  Node_base()
    : m_nb(0), m_nw(0), m_uk(0), m_f(0), m_f1(0), m_f2(0), m_f3(0),
      m_nb_set1(0), m_nw_set1(0), m_nb_set2(0), m_nw_set2(0),
      m_nb_set3(0), m_nw_set3(0), m_nb_set4(0), m_nw_set4(0),
      m_nb_board(0), m_nw_board(0), m_hand_id1(0), m_hand_id2(0),
      m_locid(0), m_loc_stateid(0), m_num(0), m_visited(false) {} 
  Node_base(Node_base& cur, int x);
  int get_nb_set1() const noexcept { return m_nb_set1; }
  int get_nw_set1() const noexcept { return m_nw_set1; }
  int get_nb_set2() const noexcept { return m_nb_set2; }
  int get_nw_set2() const noexcept { return m_nw_set2; }
  int get_nb_board() const noexcept { return m_nb_board; }
  int get_nw_board() const noexcept { return m_nw_board; }
  int get_nw_set12_board() const noexcept { return m_nw_board + m_nw_set1 + m_nw_set2; }
  int get_nb_set12_board() const noexcept { return m_nb_board + m_nb_set1 + m_nb_set2; }
  int get_cur_hand_id1() const noexcept { return m_hand_id1; }
  int get_cur_hand_id2() const noexcept { return m_hand_id2; }

public:
  Node_base* m_left;
  Node_base* m_right;

  Node_base(char nleaf, unsigned long long int num, bool visited, int lengthmax, int lengthmin, unsigned long long int lengthsum)
    : m_nb(nleaf), m_nw(nleaf), m_uk(nleaf), m_f(nleaf), m_f1(nleaf), m_f2(nleaf), m_f3(nleaf),
      m_nb_set1(nleaf), m_nw_set1(nleaf), m_nb_set2(nleaf), m_nw_set2(nleaf),
      m_nb_set3(nleaf), m_nw_set3(nleaf), m_nb_set4(nleaf), m_nw_set4(nleaf), m_nb_board(nleaf), m_nw_board(nleaf),
      m_hand_id1(nleaf), m_hand_id2(nleaf), m_locid(0), m_loc_stateid(0),
      m_num(num), m_visited(visited), m_lengthmax(lengthmax), m_lengthmin(lengthmin), m_lengthsum(lengthsum),
      m_left(nullptr), m_right(nullptr) {}

  virtual ~Node_base() {}
  void init_hand() noexcept {
    m_hand_id1 = get_hand_id1();
    m_hand_id2 = get_hand_id2();
  }
  void update_hand(Node_base& cur) noexcept {
    if(cur.m_locid < 8) {
      if(cur.m_loc_stateid == 2) m_hand_id1 = cur.m_hand_id1 - 2;
      else m_hand_id1 = cur.m_hand_id1;

      m_hand_id2 = cur.m_hand_id2;
    } else if(cur.m_locid < 24) {
      m_hand_id1 = cur.m_hand_id1;
      m_hand_id2 = cur.m_hand_id2;
    } else {
      if(cur.m_loc_stateid == 3) m_hand_id2 = cur.m_hand_id2 - 2;
      else m_hand_id2 = cur.m_hand_id2;

      m_hand_id1 = cur.m_hand_id1;
    }
  }
  bool IsNextLeaf0(int depth, int x, int nmove) const noexcept;
  int get_locid() const noexcept { return m_locid; }
  int get_loc_stateid() const noexcept { return m_loc_stateid; }
  unsigned long long int get_num() const noexcept { return m_num; }
  int get_f() const noexcept { return m_f; }
  int get_lengthmax() const noexcept { return m_lengthmax; }
  int get_lengthmin() const noexcept { return m_lengthmin; }
  unsigned long long int get_lengthsum() const noexcept { return m_lengthsum; }
  void dfs() noexcept {
    assert(m_left != nullptr && m_right != nullptr);
    if(!m_left->m_visited) m_left->dfs();
    if(!m_right->m_visited) m_right->dfs();
    m_num = m_left->m_num + m_right->m_num;
    m_lengthmax = max(m_left->m_lengthmax, m_right->m_lengthmax) + 1;
    m_lengthmin = min(m_left->m_lengthmin, m_right->m_lengthmin) + 1;
    m_lengthsum = m_left->m_lengthsum + m_right->m_lengthsum + m_num;
    m_visited = true;
  }
  void setRedundantNode() noexcept { m_f = 40; }
  bool IsRedundantNode() const noexcept { return m_f == 40; }
  bool IsLeaf0() const noexcept { return m_f == 20; }
  bool IsLeaf1() const noexcept { return m_f == 30; }
  bool IsEquivalentVisited_opponent_hand(Node_base& c) const noexcept {
    return m_f == c.m_f && m_nb_set1 == c.m_nb_set1 && m_nb_set2 == c.m_nb_set2 && m_nb_set3 == c.m_nb_set3 && m_nb_set4 == c.m_nb_set4
      && m_nw_set1 == c.m_nw_set1 && m_nw_set2 == c.m_nw_set2 && m_nw_set3 == c.m_nw_set3 && m_nw_set4 == c.m_nw_set4
      && m_nb == c.m_nb && m_nw == c.m_nw;
  }
  bool IsEquivalentVisited_board(Node_base& c) const noexcept {
    return m_f == c.m_f && m_nb == c.m_nb && m_nw == c.m_nw
      && m_nb - m_nb_set1 - m_nb_set2 - m_nb_set3 - m_nb_set4 == c.m_nb - c.m_nb_set1 - c.m_nb_set2 - c.m_nb_set3 - c.m_nb_set4
      && m_nw - m_nw_set1 - m_nw_set2 - m_nw_set3 - m_nw_set4 == c.m_nw - c.m_nw_set1 - c.m_nw_set2 - c.m_nw_set3 - c.m_nw_set4;
  }
  bool IsEquivalentVisited_player_hand(Node_base& c) const noexcept {
    return m_f == c.m_f && m_nb == c.m_nb && m_uk == c.m_uk && m_nw == c.m_nw
      && m_nb - m_nb_set1 - m_nb_set2 - m_nb_set3 - m_nb_set4 == c.m_nb - c.m_nb_set1 - c.m_nb_set2 - c.m_nb_set3 - c.m_nb_set4
      && m_nw - m_nw_set1 - m_nw_set2 - m_nw_set3 - m_nw_set4 == c.m_nw - c.m_nw_set1 - c.m_nw_set2 - c.m_nw_set3 - c.m_nw_set4
      && m_nb_set1 == c.m_nb_set1 && m_nb_set2 == c.m_nb_set2 && m_nb_set3 == c.m_nb_set3 && m_nb_set4 == c.m_nb_set4
      && m_nw_set1 == c.m_nw_set1 && m_nw_set2 == c.m_nw_set2 && m_nw_set3 == c.m_nw_set3 && m_nw_set4 == c.m_nw_set4;
  }
};

class Node_Black : public Node_base {
  int nw_set12_min() const noexcept override { return 3; }
  int nb_set12_min() const noexcept override { return 1; }
  bool ok_opp_set1_num_gb() const noexcept override { return get_nb_set1() == 1; }
  bool ok_opp_set1_num_gw() const noexcept override { return get_nb_set1() == 0; }
  bool ok_opp_set2_num_gb() const noexcept override { return get_nb_set2() == 1; }
  bool ok_opp_set2_num_gw() const noexcept override { return get_nb_set2() == 0; }
  int set12_board_num_x1() const noexcept override { return get_nw_set12_board(); }
  int set12_board_num_x0() const noexcept override { return get_nw_set12_board(); }
  int set12_board_num_player(int objid) const noexcept override;
  int set12_board_num_opponent(int objid) const noexcept override;
  int sub_wb_two_nhand() const noexcept override { return 9; }
  int sub_wb() const noexcept override { return get_nw_board() - get_nb_board(); }
  int opp_col() const noexcept override { return 1; }
  int get_hand_id1() const noexcept override { return 16; }
  int get_hand_id2() const noexcept override { return 15; }

public:
  Node_Black() : Node_base() { init_hand(); }
  Node_Black(Node_base& cur, int x) : Node_base(cur, x) { update_hand(cur); }
  virtual ~Node_Black() noexcept {}
};

class Node_White : public Node_base {
  int nw_set12_min() const noexcept override { return 1; }
  int nb_set12_min() const noexcept override { return 3; }
  bool ok_opp_set1_num_gb() const noexcept override { return get_nw_set1() == 0; }
  bool ok_opp_set1_num_gw() const noexcept override { return get_nw_set1() == 1; }
  bool ok_opp_set2_num_gb() const noexcept override { return get_nw_set2() == 0; }
  bool ok_opp_set2_num_gw() const noexcept override { return get_nw_set2() == 1; }
  int set12_board_num_x1() const noexcept override { return get_nb_set12_board() + 2; }
  int set12_board_num_x0() const noexcept override { return get_nb_set12_board(); }
  int set12_board_num_player(int objid) const noexcept override;
  int set12_board_num_opponent(int objid) const noexcept override;
  int sub_wb_two_nhand() const noexcept override { return 10; }
  int sub_wb() const noexcept override { return get_nb_board() - get_nw_board(); }
  int opp_col() const noexcept override { return 2; }
  int get_hand_id1() const noexcept override { return 15; }
  int get_hand_id2() const noexcept override { return 16; }

public:
  Node_White() : Node_base() { init_hand(); }
  Node_White(Node_base& cur, int x) : Node_base(cur, x) { update_hand(cur); }
  virtual ~Node_White() noexcept {}
};

#endif
