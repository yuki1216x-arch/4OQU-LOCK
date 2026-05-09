#include "node.hpp"

bool LocInfo_vali::ok() const noexcept {
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
  
  return true;
}

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

Node_base::Node_base(Node_base& cur, int x) {
  if(cur.m_locid < 8) {
    if(cur.m_loc_stateid == 2) {
      m_locid = cur.m_locid + 1, m_loc_stateid = 0;
    } else {
      m_locid = cur.m_locid, m_loc_stateid = cur.m_loc_stateid + 1;
    }
    
    // f: propagate the parent's state; if x == 1, the parent's object is placed
    if(cur.m_loc_stateid != 2 && (cur.m_f == 1 || x == 1)) m_f = 1;
    else m_f = 0;
    
    // f1 = 1 when the leftmost piece in opponent's hand is placed
    if(cur.m_loc_stateid != 0 && x == 1) m_f1 = 1;
    else m_f1 = cur.m_f1;
    
    m_f2 = cur.m_f2, m_f3 = cur.m_f3, m_uk = cur.m_uk;
    
    if(x == 1) {
      if(cur.m_locid < 4) {
	m_nw_set1 = cur.m_nw_set1 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
	m_nb_set1 = cur.m_nb_set1 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
	m_nw_set2 = cur.m_nw_set2, m_nb_set2 = cur.m_nb_set2;
      } else {
	m_nw_set1 = cur.m_nw_set1, m_nb_set1 = cur.m_nb_set1;
	m_nw_set2 = cur.m_nw_set2 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
	m_nb_set2 = cur.m_nb_set2 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
      }
      m_nw = cur.m_nw + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
      m_nb = cur.m_nb + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
    } else {
      m_nw_set1 = cur.m_nw_set1, m_nb_set1 = cur.m_nb_set1, m_nw_set2 = cur.m_nw_set2, m_nb_set2 = cur.m_nb_set2;
      m_nw = cur.m_nw, m_nb = cur.m_nb;
    }
    
    m_nw_set3 = cur.m_nw_set3, m_nb_set3 = cur.m_nb_set3, m_nw_set4 = cur.m_nw_set4, m_nb_set4 = cur.m_nb_set4;
    m_nw_board = cur.m_nw_board, m_nb_board = cur.m_nb_board;
  } else if(cur.m_locid < 24) {
    if(cur.m_loc_stateid == 6) {
      m_locid = cur.m_locid + 1, m_loc_stateid = 0;
    } else {
      m_locid = cur.m_locid, m_loc_stateid = cur.m_loc_stateid + 1;
    }
    
    // f: propagate the parent's state; if x == 1, the parent's object is placed
    if(cur.m_loc_stateid != 6 && (cur.m_f == 1 || x == 1)) m_f = 1;
    else m_f = 0;
    
    m_f1 = cur.m_f1, m_f2 = cur.m_f2, m_f3 = cur.m_f3, m_uk = cur.m_uk;
    
    if(x == 1) {
      m_nw = cur.m_nw + tbl_objid2locinfo[cur.m_loc_stateid].get_nw();
      m_nb = cur.m_nb + tbl_objid2locinfo[cur.m_loc_stateid].get_nb();
      m_nw_board = cur.m_nw_board + tbl_objid2locinfo[cur.m_loc_stateid].get_nw();
      m_nb_board = cur.m_nb_board + tbl_objid2locinfo[cur.m_loc_stateid].get_nb();
    } else {
      m_nw = cur.m_nw, m_nb = cur.m_nb, m_nw_board = cur.m_nw_board, m_nb_board = cur.m_nb_board;
    }
    
    m_nw_set1 = cur.m_nw_set1, m_nb_set1 = cur.m_nb_set1, m_nw_set2 = cur.m_nw_set2, m_nb_set2 = cur.m_nb_set2;
    m_nw_set3 = cur.m_nw_set3, m_nb_set3 = cur.m_nb_set3, m_nw_set4 = cur.m_nw_set4, m_nb_set4 = cur.m_nb_set4;
  } else {
    if(cur.m_loc_stateid == 3) {
      m_locid = cur.m_locid + 1, m_loc_stateid = 0;
    } else {
      m_locid = cur.m_locid, m_loc_stateid = cur.m_loc_stateid + 1;
    }
    
    // f: propagate the parent's state; if x == 1, the parent's object is placed
    if(cur.m_loc_stateid != 3 && (cur.m_f == 1 || x == 1)) m_f = 1;
    else m_f = 0;
    
    // f2 = 1 when the leftmost piece in player's hand is placed
    if(cur.m_loc_stateid != 0 && x == 1) m_f2 = 1;
    else m_f2 = cur.m_f2;
    
    // once a white or black piece is placed in the player's hand, only unknown pieces may follow
    if(x == 1 && (cur.m_loc_stateid == 1 || cur.m_loc_stateid == 2)) m_f3 = 1;
    else m_f3 = cur.m_f3;
    
    m_f1 = cur.m_f1, m_hand_id2 = cur.m_hand_id2;
    
    if(x == 1) {
      if(cur.m_locid < 28) {
	m_nw_set3 = cur.m_nw_set3 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
	m_nb_set3 = cur.m_nb_set3 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
	m_nw_set4 = cur.m_nw_set4, m_nb_set4 = cur.m_nb_set4;
      } else {
	m_nw_set3 = cur.m_nw_set3, m_nb_set3 = cur.m_nb_set3;
	m_nw_set4 = cur.m_nw_set4 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
	m_nb_set4 = cur.m_nb_set4 + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
      }
      m_nw = cur.m_nw + tbl_objid2locinfo2[cur.m_loc_stateid].get_nw();
      m_nb = cur.m_nb + tbl_objid2locinfo2[cur.m_loc_stateid].get_nb();
      m_uk = cur.m_uk + tbl_objid2locinfo2[cur.m_loc_stateid].get_uk();
    } else {
      m_nw_set3 = cur.m_nw_set3, m_nb_set3 = cur.m_nb_set3, m_nw_set4 = cur.m_nw_set4, m_nb_set4 = cur.m_nb_set4;
      m_nw = cur.m_nw, m_nb = cur.m_nb, m_uk = cur.m_uk;
    }
    
    m_nw_set1 = cur.m_nw_set1, m_nb_set1 = cur.m_nb_set1, m_nw_set2 = cur.m_nw_set2, m_nb_set2 = cur.m_nb_set2;
    m_nw_board = cur.m_nw_board, m_nb_board = cur.m_nb_board;
  }

  m_num = 0, m_visited = false;
}

bool Node_base::IsNextLeaf0(int depth, int x, int nmove) const noexcept {
  assert(0 <= depth && depth <= 167);
  assert(x == 0 || x == 1);
  assert(0 <= nmove && nmove < 16);
  int nhand = 16 - nmove;
  
  if(x == 1 && m_f == 1) return true; // Invalid: attempting to place an object on an already occupied square
  if(depth < 24) { // opponent's pieces in hand
    // Invalid: placing a non-empty value on squares beyond the current square number(remaining moves)
    if(x == 1 && m_loc_stateid != 0 && m_hand_id1 > nhand) return true;
    if(x == 1 && m_loc_stateid == 0 && m_hand_id1 <= nhand) return true;
    if(x == 0 && m_loc_stateid == 0 && m_hand_id1 > nhand) return true;
    
    if(x == 1 && m_loc_stateid == 0 && m_f1 == 1) return true; // Invalid: placing an empty value after a piece has been placed in hand
    if(x == 0 && depth % 3 == 2 && m_f == 0) return true; // Invalid: value remains unset
    if(depth < 12) { // set1
      // Invalid: exceeding the number of white pieces in set1
      if(x == 1 && m_loc_stateid == 1 && m_nw_set1 >= nw_set12_min()) return true;
      // Invalid: exceeding the number of black pieces in set1
      if(x == 1 && m_loc_stateid == 2 && m_nb_set1 >= nb_set12_min()) return true;
    } else { // set2
      // Invalid: exceeding the number of white pieces in set2
      if(x == 1 && m_loc_stateid == 1 && m_nw_set2 >= nw_set12_min()) return true;
      // Invalid: exceeding the number of black pieces in set2
      if(x == 1 && m_loc_stateid == 2 && m_nb_set2 >= nb_set12_min()) return true;
    }
    return false;
  }
  if(depth < 136) { // on the board
    if(x == 0 && (depth - 24) % 7 == 6 && m_f == 0) return true; // Invalid: value remains unset
    
    // Invalid: placing more pieces than the limit
    if(x == 1 && m_f == 0) {
      int objid = (depth - 24) % 7;
      if(nhand > opp_col() + 11) {
	if(8 - set12_board_num_player(objid) < 6 && 8 - set12_board_num_opponent(objid) < 2) return true;
	if(nhand <= opp_col() + 13) {
	  if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 7) return true;
	}
      } else if(nhand > opp_col() + 9) {
	if(8 - set12_board_num_player(objid) < 5 && 8 - set12_board_num_opponent(objid) < 2) return true;
	if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 6) return true;
      } else if(nhand > opp_col() + 7) {
	if(8 - set12_board_num_player(objid) < 4 && 8 - set12_board_num_opponent(objid) < 2) return true;
	if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 5) return true;
      } else if(nhand > opp_col() + 3) {
	if(8 - set12_board_num_player(objid) < 3 && 8 - set12_board_num_opponent(objid) < 1) return true;
	if(nhand <= opp_col() + 5) {
	  if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 3) return true;
	}
      } else if(nhand > opp_col() + 1) {
	if(8 - set12_board_num_player(objid) < 2 && 8 - set12_board_num_opponent(objid) < 1) return true;
	if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 2) return true;
      } else if(nhand > opp_col() - 1) {
	if(8 - set12_board_num_player(objid) < 1 && 8 - set12_board_num_opponent(objid) < 1) return true;
	if(16 - set12_board_num_player(objid) - set12_board_num_opponent(objid) < 1) return true;
      } else {
	assert(nhand >= 0);
	if(8 - set12_board_num_player(objid) < 0 || 8 - set12_board_num_opponent(objid) < 0) return true;
      }
    }

    if(depth == 135) {
      if(x == 1 && m_f == 0) {
	// Invalid: the number of black pieces exceeds that of white by two
	if(nhand > 8 && (m_nb_board + 2) - m_nw_board > 1 && ok_opp_set1_num_gb()) return true;
	if(nhand <= 8 && (m_nb_board + 2) - m_nw_board > 1 && ok_opp_set2_num_gb()) return true;

	// Invalid: the number of white pieces exceeds that of black by two
	if(nhand > 8 && m_nw_board - (m_nb_board + 2) > 1 && ok_opp_set1_num_gw()) return true;
	if(nhand <= 8 && m_nw_board - (m_nb_board + 2) > 1 && ok_opp_set2_num_gw()) return true;

	// Invalid: incorrect number of opponent-colored pieces in the player's hand
	// (2 for remaining moves > 9, otherwise 1)

	// Invalid: pieces on the board that do not exist in hand
	if(nhand > 8 && nw_set12_min() - m_nw_set1 > m_nw_board) return true;
	if(nhand > 8 && nb_set12_min() - m_nb_set1 > m_nb_board + 2) return true;
	if(nhand <= 8 && 4 + (nw_set12_min() - m_nw_set2) > m_nw_board) return true;
	if(nhand <= 8 && 4 + (nb_set12_min() - m_nb_set2) > m_nb_board + 2) return true;

	// Invalid: (white - black) piece count on the board is out of bounds
	if(nhand - opp_col() > 13) {
	  if(8 - set12_board_num_player(6) != 6 || 8 - set12_board_num_opponent(6) != 2) return true;
	} else if(nhand - opp_col() > 7) {
	  if(8 - set12_board_num_player(6) > (nhand - opp_col()) / 2 || 8 - set12_board_num_opponent(6) > 2) return true;
	} else if(nhand - opp_col() > 5) {
	  if(8 - set12_board_num_player(6) != 3 || 8 - set12_board_num_opponent(6) != 1) return true;
	} else if(nhand - opp_col() >= 0) {
	  if(8 - set12_board_num_player(6) > (nhand - opp_col() + 2) / 2 || 8 - set12_board_num_opponent(6) > 1) return true;
	} else {
	  assert(nhand >= 0);
	  if(8 - set12_board_num_player(6) != 0 || 8 - set12_board_num_opponent(6) != 0) return true;
	}
	
      } else if(x == 0 && m_f == 1) {
	// Invalid: the number of black pieces exceeds that of white by two
	if(nhand > 8 && m_nb_board - m_nw_board > 1 && ok_opp_set1_num_gb()) return true;
	if(nhand <= 8 && m_nb_board - m_nw_board > 1 && ok_opp_set2_num_gb()) return true;

	// Invalid: the number of white pieces exceeds that of black by two
	if(nhand > 8 && m_nw_board - m_nb_board > 1 && ok_opp_set1_num_gw()) return true;
	if(nhand <= 8 && m_nw_board - m_nb_board > 1 && ok_opp_set2_num_gw()) return true;

	// Invalid: incorrect number of opponent-colored pieces in the player's hand
	// (2 for remaining moves > 9, otherwise 1)

	// Invalid: pieces on the board that do not exist in hand
	if(nhand > 8 && nw_set12_min() - m_nw_set1 > m_nw_board) return true;
	if(nhand > 8 && nb_set12_min() - m_nb_set1 > m_nb_board) return true;
	if(nhand <= 8 && 4 + (nw_set12_min() - m_nw_set2) > m_nw_board) return true;
	if(nhand <= 8 && 4 + (nb_set12_min() - m_nb_set2) > m_nb_board) return true;

	if(nhand - opp_col() > 13) {
	  if(8 - set12_board_num_player(0) != 6 || 8 - set12_board_num_opponent(0) != 2) return true;
	} else if(nhand - opp_col() > 7) {
	  if(8 - set12_board_num_player(0) > (nhand - opp_col()) / 2 || 8 - set12_board_num_opponent(0) > 2) return true;
	} else if(nhand - opp_col() > 5) {
	  if(8 - set12_board_num_player(0) != 3 || 8 - set12_board_num_opponent(0) != 1) return true;
	} else if(nhand - opp_col() >= 0) {
	  if(8 - set12_board_num_player(0) > (nhand - opp_col() + 2) / 2 || 8 - set12_board_num_opponent(0) > 1) return true;
	} else {
	  assert(nhand >= 0);
	  if(8 - set12_board_num_player(0) != 0 || 8 - set12_board_num_opponent(0) != 0) return true;
	}
      }
    }
    return false;
  }
  
  // player's pieces in hand
  // Invalid: placing a non-empty value on squares beyond the current square number (remaining moves)
  if(x == 1 && m_loc_stateid != 0 && m_hand_id2 > nhand) return true;
  if(x == 1 && m_loc_stateid == 0 && m_hand_id2 <= nhand) return true;

  if(x == 1 && m_loc_stateid == 0 && m_f2 == 1) return true; // Invalid: placing an empty value after a piece has been placed in hand
  if(x == 1 && (m_loc_stateid == 1 || m_loc_stateid == 2) && m_f3 == 1) return true; // Invalid: placing a white or black piece after an unknown piece
  if(x == 1 && m_loc_stateid == 3 && m_f3 == 0) return true; // Invalid: placing an unknown piece before any white or black piece
  if(x == 0 && (depth - 136) % 4 == 3 && m_f == 0) return true; // Invalid: value remains unset

  if(depth < 152) { // set3
    if(x == 1) {
      if(m_loc_stateid == opp_col() && sub_wb() > 1) return true;
      if(m_loc_stateid == opp_col() && set12_board_num_x0() == 7) return true;
      
      // Invalid: exceeding the number of white pieces in set3
      if(m_loc_stateid == 1 && m_nw_set3 >= nb_set12_min()) return true;
      // Invalid: exceeding the number of black pieces in set3
      if(m_loc_stateid == 2 && m_nb_set3 >= nw_set12_min()) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(m_loc_stateid == 1 && m_nw_board - (nw_set12_min() - m_nw_set1) - (nw_set12_min() - m_nw_set2) - nb_set12_min() >= nb_set12_min()) return true;
      if(m_loc_stateid == 2 && m_nb_board - (nb_set12_min() - m_nb_set1) - (nb_set12_min() - m_nb_set2) - nw_set12_min() >= nw_set12_min()) return true;
    }
  } else { // set4
    if(x == 1) {
      if(m_loc_stateid == opp_col() && sub_wb() > 1) return true;
      if(m_loc_stateid == opp_col() && set12_board_num_x0() == 8) return true;
      
      // Invalid: exceeding the number of white pieces in set4
      if(m_loc_stateid == 1 && m_nw_set4 >= nb_set12_min()) return true;
      // Invalid: exceeding the number of black pieces in set4
      if(m_loc_stateid == 2 && m_nb_set4 >= nw_set12_min()) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(m_loc_stateid == 1 && m_nw_board - (nw_set12_min() - m_nw_set1) - (nw_set12_min() - m_nw_set2) - nb_set12_min() >= nb_set12_min()) return true;
      if(m_loc_stateid == 2 && m_nb_board - (nb_set12_min() - m_nb_set1) - (nb_set12_min() - m_nb_set2) - nw_set12_min() >= nw_set12_min()) return true;
    }

    // Invalid: total number of pieces exceeds the limit
    if(depth == 167 && x == 1 && m_f == 0 && !(m_nb + m_nw + m_uk == 15 && m_nb <= 8 && m_nw <= 8)) return true;
    if(depth == 167 && x == 0 && m_f == 1 && !(m_nb + m_nw + m_uk == 16 && m_nb <= 8 && m_nw <= 8)) return true;
  }
  return false;
}

int Node_Black::set12_board_num_player(int objid) const noexcept { return get_nb_set12_board() + tbl_objid2locinfo[objid].get_nb(); }
int Node_Black::set12_board_num_opponent(int objid) const noexcept { return get_nw_set12_board() + tbl_objid2locinfo[objid].get_nw(); }
int Node_White::set12_board_num_player(int objid) const noexcept { return get_nw_set12_board() + tbl_objid2locinfo[objid].get_nw(); }
int Node_White::set12_board_num_opponent(int objid) const noexcept { return get_nb_set12_board() + tbl_objid2locinfo[objid].get_nb(); }
