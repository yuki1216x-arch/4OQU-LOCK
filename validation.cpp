#include <deque>
#include <algorithm>
#include <iostream>
#include <exception>
#include <cassert>
#include "table.hpp"
#include "zdd_yonoku.hpp"

class LocInfo {
  char m_cube[2];
  char m_height;   //高さ
  char m_nb;       //黒の数
  char m_nw;       //白の数
  char m_uk;
  char m_top;      //一番上の駒の種類

public:
  LocInfo::LocInfo(char cube[2], char height, char nb, char nw,
		   char uk, char top) noexcept
    : m_height(height), m_nb(nb), m_nw(nw), m_uk(uk), m_top(top) {
    m_cube[0] = cube[0];
    m_cube[1] = cube[1];
    assert(ok()); }
  
  bool is_ob() const noexcept { return m_cube[0] == 99; }
  
  int get_cube(int i) const noexcept {
    assert(i < 0 && 1 < i);
    return m_cube[i]; }
  
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
      if (height    != 0)   return false;
      if (m_nb      != 0)   return false;
      if (m_nw      != 0)   return false;
      if (m_uk      != 0)   return false; }
    // one piece
    else if (m_cube[1] == '.') {
      if (m_top != m_cube[0]) return false;
      if (height != 1)        return false;
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
      if (height != 2)  return false;
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
      // TODO
    }
    else if (m_cube[1] == 'N') {
      if (m_cube[0] != 'N') return false; }
    else return false;
    
    return true; }

  bool operator==(const LocInfo &o) const noexcept {
    assert(o.ok());
    return (m_cube[0] == o.m_cube[0] && m_cube[1] == o.m_cube[1]) }
};

constexpr LocInfo tbl_objid2locinfo[8] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'W', 'W'}, 2, 0, 2, 0, 'W'}, //3
     { {'W', 'B'}, 2, 1, 1, 0, 'B'}, //4
     { {'B', 'W'}, 2, 1, 1, 0, 'W'}, //5
     { {'B', 'B'}, 2, 2, 0, 0, 'B'}, //6
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //7  
};

constexpr LocInfo tbl_objid2locinfo2[5] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'U', '.'}, 1, 0, 0, 1, 'U'}, //3
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //4  
};

