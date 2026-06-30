#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <random>
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using namespace std;

constexpr unsigned char tbl_loc2locid[46] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    99, 99, 99, 99, 99,
    99, 8, 9, 10, 11,
    99, 12, 13, 14, 15,
    99, 16, 17, 18, 19,
    99, 20, 21, 22, 23,
    99, 99, 99, 99, 99,
    24, 25, 26, 27, 28, 29, 30, 31
};

constexpr LocInfo tbl_objid2locinfo[7] = {
     { 0, {'.', '.'} },
     { 1, {'W', '.'} },
     { 1, {'B', '.'} },
     { 2, {'W', 'W'} },
     { 2, {'W', 'B'} },
     { 2, {'B', 'W'} },
     { 2, {'B', 'B'} }
};

constexpr LocInfo tbl_objid2locinfo2[4] = {
     { 0, {'.', '.'} },
     { 1, {'W', '.'} },
     { 1, {'B', '.'} },
     { 1, {'U', '.'} }
};



Posi::Posi() noexcept {
  for(int loc = 0; loc < 46; loc++){
    if(tbl_loc2locid[loc] < 32){
      m_locs_info[loc] = tbl_objid2locinfo[0];
    }
  }
  for(int loc = 0; loc < 16; loc++) m_already[loc] = 0;
}



Posi::Posi(unsigned long long int x, const ZDD& zdd) noexcept {
  unsigned char array_objid[32];
  zdd.compute_array(x, array_objid, 32);
  for(int loc = 0; loc < 46; loc++){
    unsigned int locid = tbl_loc2locid[loc];
    assert(locid < 32 || locid == 99);
    if(loc >= 38) {
      assert(array_objid[locid] < 4);
      m_locs_info[loc] = tbl_objid2locinfo2[array_objid[locid]];
    } else {
      assert(array_objid[locid] < 7);
      m_locs_info[loc] = tbl_objid2locinfo[array_objid[locid]];
    }
  }
  for(int loc = 0; loc < 16; loc++) m_already[loc] = 0;
}

void Posi::make_posi(unsigned long long int x, const ZDD& zdd) noexcept {
  unsigned char array_objid[32];
  zdd.compute_array(x, array_objid, 32);
  
  for(int loc = 0; loc < 46; loc++){
    unsigned int locid = tbl_loc2locid[loc];
    if(locid < 32) {
      if(loc >= 38) {
	assert(array_objid[locid] < 4);
	m_locs_info[loc] = tbl_objid2locinfo2[array_objid[locid]];
      } else {
	assert(array_objid[locid] < 7);
	m_locs_info[loc] = tbl_objid2locinfo[array_objid[locid]];
      }
    }
  }
}

void Posi::make_posi_n(unsigned char zdd_code[12][46], int n) noexcept {
  for(int i = 0; i < 46; i++) {
    assert(zdd_code[n][i] < 7);
    m_locs_info[i] = tbl_objid2locinfo[zdd_code[n][i]];
  }
}

bool Posi::is_opp_uninfo(unsigned char board_belief[12][46], int vision, int iteration) noexcept {
  unsigned char board_belief_opp[12][46] = {};
  make_posi_n(board_belief, 0);
  make_posi_opponent();
  int num_of_un_opp = getunknowninfo(board_belief_opp, 1 - vision, iteration);
  if(num_of_un_opp > 1) {
    return true; //true if this is an imperfect-information state
  } else {
    return false;
  }
}

void Posi::make_posi_opponent() noexcept {
  LocInfo a = tbl_objid2locinfo[0];
  LocInfo b = tbl_objid2locinfo[0];
  int flg_unk = 0;
  for(int i = 0; i < 8; i++) { //create a configuration with the player's hand pieces swapped
    a = m_locs_info[i];
    b = m_locs_info[38 + i];
    m_locs_info[i] = b;
    m_locs_info[38 + i] = a;
  }
  for(int i = 0; i < 8; i++) { //hide all hand pieces on the current player's side except the rightmost piece
    if(m_locs_info[38 + i].cube[0] == '.') continue;
    if(flg_unk == 1) {
      m_locs_info[38 + i] = tbl_objid2locinfo2[3];
    }
    if(flg_unk == 0 && m_locs_info[38 + i].cube[0] != '.') {
      flg_unk = 1;
    }
  }
}

void Posi::make_posi_myself() noexcept {
  int flg_unk = 0;
  for(int i = 0; i < 8; i++) {
    if(m_locs_info[38 + i].cube[0] == '.') continue;
    if(flg_unk == 1) {
      m_locs_info[38 + i] = tbl_objid2locinfo2[3];
    }
    if(flg_unk == 0 && m_locs_info[38 + i].cube[0] != '.') {
      flg_unk = 1;
    }
  }
}

void Posi::print() const noexcept {
  for(int i = 0; i < 8; i++) {
    //if(i % 8 == 0 && i != 0) cout << "\n";
    cout << m_locs_info[i].cube[0];
  }
  cout << "\n";
  for(int i = 45; i > 37; i--) {
    cout << m_locs_info[i].cube[0];
  }
  cout << "\n";
  cout << "   _______________\n";
  for(int i = 0; i < 4; i++){
    cout << "   " << "|" << " ";
    for(int j = 0; j < 4; j++){
      cout << m_locs_info[14 + 5 * i + j].cube[0] << m_locs_info[14 + 5 * i + j].cube[1]
                << " ";
    }
        cout << "|\n";
  }
  cout << "   ^^^^^^^^^^^^^^^" << endl;
}

constexpr int tbl_dirid2dir[5] = {-5, -1, 0, 1, 5};

int Posi::compute_actions(Action actions[1000], int vision, int turn) noexcept { //vi = 0: configuration from white's perspective; vi = 1: configuration from black's perspective
  assert(vision == 0 || vision == 1);
  assert(turn >= 0 && turn <= 15);
  
  int bc = TerminalTest(vision, turn); // terminal check

  if(bc == 2) {
    //cout << "-1" << endl;
    return -1; //current player wins
  } else if(bc == 3) {
    //cout << "-2" << endl;
    return -2; //current player loses
  } else if(bc == 1) {
    //cout << "-3" << endl;
    return -3; //unknown
  }
  
  //cout << "aaaaa" << endl;
  
  int naction = 0; //number of legal moves
  int po = -1; //square index from which to place
  int f; //avoid duplicating the action of not moving a piece
  
  if(turn % 2 == vision) { //scan the hand pieces from right to left and store in po the index of the non-empty square containing the next piece to use
    for(int position = 38; position < 46; position++) { //player's hand pieces
      if(m_locs_info[position].height == 1) {
	po = position;
	assert(po >= 38 && 46 > po);
	break;
      }
    }
  } else {
    for(int position = 0; position < 8; position++) { //opponent's hand pieces
      if(m_locs_info[position].height == 1) {
	po = position;
	assert(po >= 0 && 8 > po);
	break;
      }
    }
  }

  int co1 = 0;

  for(int loc = 14; loc < 33; loc++){ //loc: board square index where the hand piece is placed
    if (tbl_loc2locid[loc] == 99) continue; //cannot place a piece out of bounds
    if (m_locs_info[loc].height >= 1) continue; //cannot place a piece on an occupied square
    f = 0;
    
    for(int loc2 = 14; loc2 < 33; loc2++) { //loc2: board square index containing the piece to move
      if (tbl_loc2locid[loc2] == 99) continue; //cannot move a piece from out of bounds
      if (m_locs_info[loc2].height == 2) continue; //cannot move a locked piece
      if (m_locs_info[loc2].height == 0) continue; //cannot move a piece from an empty square
      if (loc2 == loc) continue; //cannot move a piece placed on the same turn
      
      assert(m_locs_info[loc2].height == 1); //squares other than those with exactly one piece should have been excluded
      //char color2 = m_locs_info[loc2].cube[m_locs_info[loc2].height-1];
      for(int dirid = 0; dirid < 5; dirid++) { //loop over movement patterns
	int dir = tbl_dirid2dir[dirid]; //In ascending index order: up, left, stay, right, down
	assert(loc2 + dir >= 0 && loc2 + dir <= 45);
	
	//destination must not be out of bounds
	//cannot move to a square with a locked piece
	//neither player can move pieces on their first turn
	if(tbl_loc2locid[loc2 + dir] == 99 || m_locs_info[loc2 + dir].height == 2 || po == 0 || po == 38 || (f == 1 && dir == 0)) continue;
	co1++;
	if(dir == 0) f = 1;
	assert(naction < 1000);
	actions[naction++] = { po, loc, loc2, loc2 + dir };
      }
    }
    assert(naction < 1000);
    if(co1 == 0) actions[naction++] = { po, loc, 100, 100 };
  }
  assert(naction < 1000);
  return naction;
}

// return the object index of the piece
int Posi::getobjnum(const LocInfo& a) noexcept {
  int ret = -1;
  if(a.cube[0] == '.' && a.cube[1] == '.') {
    ret = 0;
  } else if(a.cube[0] == 'W' && a.cube[1] == '.') {
    ret = 1;
  } else if(a.cube[0] == 'B' && a.cube[1] == '.') {
    ret = 2;
  } else if(a.cube[0] == 'W' && a.cube[1] == 'W') {
    ret = 3;
  } else if(a.cube[0] == 'W' && a.cube[1] == 'B') {
    ret = 4;
  } else if(a.cube[0] == 'B' && a.cube[1] == 'W') {
    ret = 5;
  } else if(a.cube[0] == 'B' && a.cube[1] == 'B') {
    ret = 6;
    } else if(a.cube[0] == 'U' && a.cube[1] == '.') {
    ret = 7;
  }
  return ret;
}

int Posi::getunknowninfo(unsigned char zdd_code[12][46], int vision, int turn) noexcept {
  assert(0 <= turn && turn < 16);
  assert(0 <= vision && vision <= 1);
  int num_unknown = 0;
  int black_num = 0;
  int white_num = 0;
  int flg_un = 0;
  int unknownposi = -1;
  int retiter = 0;
  for(int i = 0; i < 46; i++) {
    //count the number of pieces
    if(getobjnum(m_locs_info[i]) == 0 || tbl_loc2locid[i] > 31) continue;
    if(getobjnum(m_locs_info[i]) == 1) {
      white_num += 1;
    } else if(getobjnum(m_locs_info[i]) == 2) {
      black_num += 1;
    } else if(getobjnum(m_locs_info[i]) == 3) {
      white_num += 2;
    } else if(getobjnum(m_locs_info[i]) == 4 || getobjnum(m_locs_info[i]) == 5) {
      black_num += 1;
      white_num += 1;
    } else if(getobjnum(m_locs_info[i]) == 6) {
      black_num += 2;
    } else if(getobjnum(m_locs_info[i]) == 7) {
      num_unknown++;
      if(flg_un == 0) {
	flg_un = 1;
	unknownposi = i;
      }
    }
  }
  black_num = 8 - black_num; //number of unknown black pieces
  white_num = 8 - white_num; //number of unknown white pieces
  assert(black_num >= 0 && black_num <= 8);
  assert(white_num >= 0 && white_num <= 8);
  if(vision == 0) {
    //white player's perspective
    if(turn >= 14) {
      for(int i = 0; i < 46; i++) {
	if(tbl_loc2locid[i] > 31) continue;
	assert(0 <= retiter && retiter < 12);
	assert(0 <= i && i < 46);
	zdd_code[retiter][i] = getobjnum(m_locs_info[i]);
      }
      retiter++;
    } else {
      if(black_num == 1) {
	//one unknown black piece
	if(unknownposi < 42) {
	  //the rightmost unknown piece is among the rightmost four pieces
	  for(int i = 0; i < 4; i++) {
	    for(int j = 0; j < 42; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      if(getobjnum(m_locs_info[j]) == 7) {
		assert(0 <= retiter && retiter < 12);
		zdd_code[retiter][j] = 1;
	      } else {
		assert(0 <= retiter && retiter < 12);
		zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	      }
	    }
	    for(int j = 0; j < 4; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 2;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 1;
	      }
	    }
	    retiter++;
	  }
	} else {
	  //the rightmost unknown piece is among the leftmost four pieces
	  for(int i = 0; i < num_unknown; i++) {
	    for(int j = 0; j < unknownposi; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      assert(0 <= retiter && retiter < 12);
	      zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	    }
	    for(int j = 0; j < num_unknown; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 2;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 1;
	      }
	    }
	    retiter++;
	  }
	}
      } else if(black_num == 2) {
	//two unknown black pieces
	//the rightmost unknown piece is among the rightmost four pieces
	for(int k = 0; k < 42 - unknownposi; k++) {
	  for(int i = 0; i < 4; i++) {
	    for(int j = 0; j < unknownposi; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      assert(0 <= retiter && retiter < 12);
	      zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	    }
	    for(int j = 0; j < 42 - unknownposi; j++) {
	      if(j == k) {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 2;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 1;
	      }
	    }
	    for(int j = 0; j < 4; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 2;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 1;
	      }
	    }
	    retiter++;
	  }
	}
      } else {
	//no unknown black pieces
	for(int i = 0; i < 46; i++) {
	  if(tbl_loc2locid[i] > 31) continue;
	  if(getobjnum(m_locs_info[i]) == 7) {
	    assert(0 <= retiter && retiter < 12);
	    zdd_code[retiter][i] = 1;
	  } else {
	    assert(0 <= retiter && retiter < 12);
	    zdd_code[retiter][i] = getobjnum(m_locs_info[i]);
	  }
	}
	retiter++;
      }
    }
  } else {
    //black player's perspective
    if(turn >= 14) {
      for(int i = 0; i < 46; i++) {
	if(tbl_loc2locid[i] > 31) continue;
	assert(0 <= retiter && retiter < 12);
	zdd_code[retiter][i] = getobjnum(m_locs_info[i]);
      }
      retiter++;
    } else {
      if(white_num == 1) {
	//one unknown white piece
	if(unknownposi < 42) {
	  //the rightmost unknown piece is among the rightmost four pieces
	  for(int i = 0; i < 4; i++) {
	    for(int j = 0; j < 42; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      if(getobjnum(m_locs_info[j]) == 7) {
		assert(0 <= retiter && retiter < 12);
		zdd_code[retiter][j] = 2;
	      } else {
		assert(0 <= retiter && retiter < 12);
		zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	      }
	    }
	    for(int j = 0; j < 4; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 1;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 2;
	      }
	    }
	    retiter++;
	  }
	} else {
	  //the rightmost unknown piece is among the leftmost four pieces
	  for(int i = 0; i < num_unknown; i++) {
	    for(int j = 0; j < unknownposi; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      assert(0 <= retiter && retiter < 12);
	      zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	    }
	    for(int j = 0; j < num_unknown; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 1;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 2;
	      }
	    }
	    retiter++;
	  }
	}
      } else if(white_num == 2) {
	//two unknown white pieces
	//the rightmost unknown piece is among the rightmost four pieces
	for(int k = 0; k < 42 - unknownposi; k++) {
	  for(int i = 0; i < 4; i++) {
	    for(int j = 0; j < unknownposi; j++) {
	      if(tbl_loc2locid[j] > 31) continue;
	      assert(0 <= retiter && retiter < 12);
	      zdd_code[retiter][j] = getobjnum(m_locs_info[j]);
	    }
	    for(int j = 0; j < 42 - unknownposi; j++) {
	      if(j == k) {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 1;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + unknownposi >= 0 && j + unknownposi <= 45);
		zdd_code[retiter][j + unknownposi] = 2;
	      }
	    }
	    for(int j = 0; j < 4; j++) {
	      if(j == i) {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 1;
	      } else {
		assert(0 <= retiter && retiter < 12);
		assert(j + 42 >= 0 && j + 42 <= 45);
		zdd_code[retiter][j + 42] = 2;
	      }
	    }
	    retiter++;
	  }
	}
      } else {
	//no unknown white piece
	for(int i = 0; i < 46; i++) {
	  if(tbl_loc2locid[i] > 31) continue;
	  if(getobjnum(m_locs_info[i]) == 7) {
	    assert(0 <= retiter && retiter < 12);
	    zdd_code[retiter][i] = 2;
	  } else {
	    assert(0 <= retiter && retiter < 12);
	    zdd_code[retiter][i] = getobjnum(m_locs_info[i]);
	  }
	}
	retiter++;
      }
    }
  }
  return retiter;
}

unsigned long long int Posi::getzddnum(const ZDD& zdd) const noexcept {
  unsigned char before_array[32] = {};
  int before_array_iter = 0;
  for(int j = 0; j < 46; j++) {
    if(tbl_loc2locid[j] > 31) continue;
    if(j < 38) {
      //excluding the player's hand pieces
      if(m_locs_info[j].cube[0] == tbl_objid2locinfo[0].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[0].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[0].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 0;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[1].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[1].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[1].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 1;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[2].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[2].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[2].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 2;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[3].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[3].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[3].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 3;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[4].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[4].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[4].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 4;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[5].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[5].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[5].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 5;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo[6].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo[6].cube[1] && m_locs_info[j].height == tbl_objid2locinfo[6].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 6;
      }
      before_array_iter++;
    } else {
      //the player's hand pieces
      if(m_locs_info[j].cube[0] == tbl_objid2locinfo2[0].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo2[0].cube[1] && m_locs_info[j].height == tbl_objid2locinfo2[0].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 0;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo2[1].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo2[1].cube[1] && m_locs_info[j].height == tbl_objid2locinfo2[1].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 1;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo2[2].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo2[2].cube[1] && m_locs_info[j].height == tbl_objid2locinfo2[2].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 2;
      } else if(m_locs_info[j].cube[0] == tbl_objid2locinfo2[3].cube[0] && m_locs_info[j].cube[1] == tbl_objid2locinfo2[3].cube[1] && m_locs_info[j].height == tbl_objid2locinfo2[3].height) {
	assert(before_array_iter >= 0 && before_array_iter <= 31);
	before_array[before_array_iter] = 3;
      }
      before_array_iter++;
    }
  }
  return zdd.compute_id(before_array, 32);
}

bool Posi::exist_action() const noexcept {
  for(int loc = 7; loc < 42; loc++){
    if (tbl_loc2locid[loc] == 99) continue;
    
    // Is the top cube at loc black?
    if(m_locs_info[loc].height >= 1 && m_locs_info[loc].cube[m_locs_info[loc].height-1] == 'W') {
      if(loc >= 37) return true; // win
      for(int dirid = 0; dirid < 8; dirid++) {
	int dir = tbl_dirid2dir[dirid];
	if(tbl_loc2locid[loc + dir] == 99 || m_locs_info[loc + dir].height == 3) continue;
	return true;
	assert(m_locs_info[loc].cube[m_locs_info[loc].height-1] == 'W');
      }
    }
  }
  return false;
}

int Posi::FourCheck(int x, int y, int z) noexcept {
  assert(x >= -1 && x <= 4);
  assert(y >= -1 && y <= 4);
  assert(z == 'W' || z == 'B');
  if(x == -1 || x == 4 || y == -1 || y == 4) return 0;
  int locid = 14 + x + 5*y;
  assert(locid >= 0 && locid <= 45);
  assert(x + 4*y >= 0 && x + 4*y <= 15);
  if(m_locs_info[locid].cube[0] == '.') return 0;
  assert(m_locs_info[locid].height - 1 >= 0 && m_locs_info[locid].height - 1 <= 1);
  if(m_locs_info[locid].cube[m_locs_info[locid].height - 1] != z) return 0;
  if(m_already[x + 4*y] == 1) return 0;
  m_already[x + 4*y] = 1;
  return 1 + FourCheck(x + 1, y, z) + FourCheck(x, y + 1, z) + FourCheck(x - 1, y, z) + FourCheck(x, y - 1, z);
}


int Posi::BoardCheck(int a, int b) noexcept {
  //int k;
  int result;
  //int flg = 0;
  for(int k = 0; k < 16; k++) m_already[k] = 0;
  if(a == 100 || b == 100) return 0;
  // for(i = 0; i < 4; i++) {
  //     for(j = 0; j < 4; j++) {
  //         if(m_locs_info[22 + j + 5*i].height == 0) continue;
  //         result = FourCheck(i, j, m_locs_info[22 + j + 5*i].cube[m_locs_info[22 + j + 5*i].height - 1], m_already);
  //         if(result >= 4) {
  //             flg = 1;
  //             break;
  //         } else {
  //             for(k = 0; k < 16; k++) m_already[k] = 0;
  //         }
  //     }
  // }
  result = FourCheck((a - 14) % 5, (a - 14) / 5, m_locs_info[a].cube[m_locs_info[a].height - 1]);
  if(result >= 4) {
    // for(int i = 0; i < 4; i++) {
    //     for(int j = 0; j < 4; j++) {
    //         cout << m_already[j + 4*i] << " ";
    //     }
    //     cout << endl;
    // }
    if(m_locs_info[a].cube[m_locs_info[a].height - 1] == 'W') {
      return 1;
    } else if(m_locs_info[a].cube[m_locs_info[a].height - 1] == 'B') {
      return 2;
    }
    //return true;
  } else {
    for(int k = 0; k < 16; k++) m_already[k] = 0;
  }
  result = FourCheck((b - 14) % 5, (b - 14) / 5, m_locs_info[b].cube[m_locs_info[b].height - 1]);
  if(result >= 4) {
    // for(int i = 0; i < 4; i++) {
    //     for(int j = 0; j < 4; j++) {
    //         cout << m_already[j + 4*i] << " ";
    //     }
    //     cout << endl;
    // }
    if(m_locs_info[b].cube[m_locs_info[b].height - 1] == 'W') {
      //for(k = 0; k < 16; k++) m_already[k] = 0;
      return 1;
    } else if(m_locs_info[b].cube[m_locs_info[b].height - 1] == 'B') {
      //for(k = 0; k < 16; k++) m_already[k] = 0;
      return 2;
    }
    //return true;
  }
  
  return 0;
  // if(flg == 1) return true;
  // else return false;
}

int Posi::TerminalTest(int vision, int turn) noexcept { //初期判定
  int i, j;
  int result1, result2;
  int flg1 = 0;
  int flg2 = 0;
  //cout << "----------------------" << endl;
  for(int k = 0; k < 16; k++) m_already[k] = 0;
  for(i = 0; i < 4; i++) {
    for(j = 0; j < 4; j++) {
      // cout << "----------------------" << endl;
      assert(14 + j + 5*i >= 0 && 14 + j + 5*i <= 45);
      if(m_locs_info[14 + j + 5*i].height == 0) {
	//print();
	continue;
      }
      if(m_already[j + 4*i] == 1) continue;
      // cout << "----------------------" << endl;
      assert(14 + j + 5*i >= 0 && 14 + j + 5*i <= 45);
      result1 = FourCheck(j, i, m_locs_info[14 + j + 5*i].cube[m_locs_info[14 + j + 5*i].height - 1]);
      if(result1 >= 4 && m_locs_info[14 + j + 5*i].cube[m_locs_info[14 + j + 5*i].height - 1] == 'W') {
	flg1 = 1; //a white tetromino has been formed
      } else if(result1 >= 4 && m_locs_info[14 + j + 5*i].cube[m_locs_info[14 + j + 5*i].height - 1] == 'B') {
	flg2 = 1; //a black tetromino has been formed
      }
      if(flg1 == 1 && flg2 == 1) break; //both tetrominoes have been formed
    }
  }
  if(vision == 0) {
    if(flg1 == 1 && flg2 == 1) {
      if(turn % 2 == 0) result2 = 2;
      else result2 = 3;
    } else if(flg1 == 1 && flg2 == 0) {
      result2 = 2; //white player wins
    } else if(flg1 == 0 && flg2 == 1) {
      result2 = 3; //white player loses
    } else {
      return 0; //the result has not been decided
    }
  } else {
    if(flg1 == 1 && flg2 == 1) {
      if(turn % 2 == 0) result2 = 3;
      else result2 = 2;
    } else if(flg1 == 1 && flg2 == 0) {
      result2 = 3; //black player wins
    } else if(flg1 == 0 && flg2 == 1) {
      result2 = 2; //black player loses
    } else {
      result2 = 0; //the result has not been decided
    }
  }
  
  return result2;
}

bool Posi::TegomaCheck() noexcept {
  if(m_locs_info[7].cube[0] == '.' && m_locs_info[45].cube[0] == '.') return true;
  else return false;
}

bool Posi::check_ok() noexcept {
  int nw = 0;
  int nb = 0;
  int nw_set1 = 0, nw_set2 = 0, nw_set3 = 0, nw_set4 = 0;
  int nb_set1 = 0, nb_set2 = 0, nb_set3 = 0, nb_set4 = 0;
  int f = 0;
  
  for(int i = 0; i < 8; i++) {
    if(m_locs_info[i].height == 0 && (m_locs_info[i].cube[0] != '.' || m_locs_info[i].cube[1] != '.')) return false;
    if(f == 1 && m_locs_info[i].cube[m_locs_info[i].height - 1] == '.') return false;
    if(m_locs_info[i].height >= 1) {
      f = 1;
      if(m_locs_info[i].height >= 2) return false;
      if(m_locs_info[i].cube[m_locs_info[i].height - 1] == 'W') {
	if(i < 4) nw_set1++;
	else nw_set2++;
	nw++;
	if(nw_set1 >= 2 || nw_set2 >= 2) return false;
      } else if(m_locs_info[i].cube[m_locs_info[i].height - 1] == 'B') {
	if(i < 4) nb_set1++;
	else nb_set2++;
	nb++;
	if(nb_set1 >= 4 || nb_set2 >= 4) return false;
      }
    }
  }
  f = 0;
  for(int i = 8; i < 16; i++) {
    if(m_locs_info[i].height == 0 && (m_locs_info[i].cube[0] != '.' || m_locs_info[i].cube[1] != '.')) return false;
    if(f == 1 && m_locs_info[i].cube[m_locs_info[i].height - 1] == '.') return false;
    if(m_locs_info[i].height >= 1) {
      f = 1;
      if(m_locs_info[i].height >= 2) return false;
      if(m_locs_info[i].cube[m_locs_info[i].height - 1] == 'W') {
	if(i < 12) nw_set3++;
	else nw_set4++;
	nw++;
	if(nw_set3 >= 4 || nw_set4 >= 4) return false;
      } else if(m_locs_info[i].cube[m_locs_info[i].height - 1] == 'B') {
	if(i < 12) nb_set3++;
	else nb_set4++;
	nb++;
	if(nb_set3 >= 2 || nb_set4 >= 2) return false;
      }
    }
  }

  if(nw >= 9 || nb >= 9) return false;
  if(nw_set1 + nw_set2 + nb_set1 + nb_set2 < nw_set3 + nw_set4 + nb_set3 + nb_set4) return false;
  if(nw_set1 + nw_set2 + nb_set1 + nb_set2 > nw_set3 + nw_set4 + nb_set3 + nb_set4 + 1) return false;
  
  for(int i = 22; i < 41; i++) {
    if(tbl_loc2locid[i] == 99) continue;
    if(m_locs_info[i].height == 0 && (m_locs_info[i].cube[0] != '.' || m_locs_info[i].cube[1] != '.')) return false;
    if(m_locs_info[i].height >= 1) {
      if(m_locs_info[i].cube[0] == '.') return false;
      if(m_locs_info[i].height >= 3) return false;
      if(m_locs_info[i].cube[0] == 'W') nw++;
      if(m_locs_info[i].cube[1] == 'W') nw++;
      if(m_locs_info[i].cube[0] == 'B') nb++;
      if(m_locs_info[i].cube[1] == 'B') nb++;
      if(nw >= 9 || nb >= 9) return false;
    }
  }

  if(nw + nb != 16) return false;
  return true;
}

int Posi::make_action(const Action & action, int vision, int turn) noexcept {
  int fourres = -100;
  int fourafter = -100;
  int blackflag = 0;
  int whiteflag = 0;
  char cl = 'a';
  
  for(int k = 0; k < 16; k++) m_already[k] = 0;    
  
  if(action.po != 100 && action.loc1 != 100) {
    assert(action.loc1 >= 0 && action.loc1 <= 45);
    assert(action.po >= 0 && action.po <= 45);
    m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height] = m_locs_info[action.po].cube[m_locs_info[action.po].height - 1];
    m_locs_info[action.loc1].height++;
    m_locs_info[action.po].cube[m_locs_info[action.po].height - 1] = '.';
    m_locs_info[action.po].height--;
    if(turn > 0) {
      fourres = FourCheck((action.loc1 - 14) % 5, (action.loc1 - 14) / 5, m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1]);
      if(fourres >= 4) {
	if(m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1] == 'W') {
	  whiteflag = 1;
	} else if(m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1] == 'B') {
	  blackflag = 1;
	}
	assert(action.loc1 >= 0 && action.loc1 <= 45);
	cl = m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1];
      }
      for(int k = 0; k < 16; k++) m_already[k] = 0;
    }
  }
  
  
  
  if(action.loc2 != 100 && action.loc3 != 100) {
    assert(action.loc3 >= 0 && action.loc3 <= 45);
    assert(action.loc2 >= 0 && action.loc2 <= 45);
    m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height] = m_locs_info[action.loc2].cube[m_locs_info[action.loc2].height - 1];
    m_locs_info[action.loc3].height++;
    m_locs_info[action.loc2].cube[m_locs_info[action.loc2].height - 1] = '.';
    m_locs_info[action.loc2].height--;
    if(turn > 0) {
      if(whiteflag == 0 && blackflag == 0) {
	fourafter = FourCheck((action.loc3 - 14) % 5, (action.loc3 - 14) / 5, m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]);
	if(fourafter >= 4) {
	  assert(action.loc3 >= 0 && action.loc3 <= 45);
	  if(m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1] == 'W') {
	    whiteflag = 1;
	  } else if(m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1] == 'B') {
	    blackflag = 1;
	  }
	}
      } else { //if a tetromino is completed when the piece is placed
	assert(action.loc1 >= 0 && action.loc1 <= 45);
	fourafter = FourCheck((action.loc1 - 14) % 5, (action.loc1 - 14) / 5, m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1]);
	if(fourafter >= 4 && cl == m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1]) {
	  int fourafter2 = FourCheck((action.loc3 - 14) % 5, (action.loc3 - 14) / 5, m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]);
	  if(fourafter2 >= 4 && cl != m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]) {
	    whiteflag = 1;
	    blackflag = 1;
	  }
	}
	if(fourafter >= 4 && cl != m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1]) {
	  //int fourafter2 = FourCheck((action.loc3 - 14) % 5, (action.loc3 - 14) / 5, m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]);
	  //if(fourafter2 < 4 || cl != m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1])  cout << "loc1 = " << fourafter << cl << ", loc2 = " << fourafter2 << m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1] << m_locs_info[action.loc1].cube[m_locs_info[action.loc1].height - 1] << ", " << action.loc1 << action.loc2 << action.loc3 << endl;
	  //assert(fourafter2 >= 4);
	  //assert(cl == m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]);
	  //if(action.loc1 != action.loc3) std::cout << action.loc1 << action.loc2 << action.loc3 << endl;
	  assert(action.loc1 == action.loc3);
	  if(cl == 'W') {
	    whiteflag = 0;
	    blackflag = 1;
	  } else {
	    whiteflag = 1;
	    blackflag = 0;
	  }
	}
	if(fourafter < 4) {
	  whiteflag = 0;
	  blackflag = 0;
	  for(int k = 0; k < 16; k++) m_already[k] = 0;
	  assert(action.loc3 >= 0 && action.loc3 <= 45);
	  fourafter = FourCheck((action.loc3 - 14) % 5, (action.loc3 - 14) / 5, m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1]);
	  if(fourafter >= 4) {
	    assert(action.loc3 >= 0 && action.loc3 <= 45);
	    if(m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1] == 'W') {
	      whiteflag = 1;
	    } else if(m_locs_info[action.loc3].cube[m_locs_info[action.loc3].height - 1] == 'B') {
	      blackflag = 1;
	    }
	  }
	}
      }
    }
  }
  
  if(vision == 0) {
    if(whiteflag == 1 && blackflag == 1) {
      if(turn % 2 == 0) return 3;
      else return 2;
    } else if(whiteflag == 1 && blackflag == 0) {
      return 2; //white player wins
    } else if(whiteflag == 0 && blackflag == 1) {
      return 3; //white player loses
    } else {
      if(turn == 15) {
	return 10;
      } else {
	return 0; //the result has not been decided
      }
    }
  } else {
    if(whiteflag == 1 && blackflag == 1) {
      if(turn % 2 == 0) return 2;
      else return 3;
    } else if(whiteflag == 1 && blackflag == 0) {
      return 3; //black player loses
    } else if(whiteflag == 0 && blackflag == 1) {
      return 2; //black player wins
    } else {
      if(turn == 15) {
	return 10;
      } else {
	return 0; //the result has not been decided
      }
    }
  }
}

// void Posi::unmake_action(const Action & action) noexcept {
//     Posi::make_action({action.loc3, action.loc2, action.loc1, action.po}, -1); //action.loc1, action.po, action.loc3, action.loc2
//     //assert(check_ok());
// }
