#ifndef INCLUDE_ZDD_YONOKU_H
#define INCLUDE_ZDD_YONOKU_H

#include <cstddef>
#include <deque>
#include <vector>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <exception>
#include <memory>
#include "node.hpp"

using std::deque;
using std::cout;
using std::endl;
using std::cerr;
using std::terminate;
using std::unique_ptr;
using std::make_unique;
using std::vector;

class ZDD { //使いまわせるやつをこの中に入れる
private:
  unique_ptr<Node_base> l0;
  unique_ptr<Node_base> l1;
  deque<std::unique_ptr<Node_base>> m_N[168];        //N[i]:深さiの節点集合
  void construct_zdd(int vision, int nmove) noexcept;

public:
  ZDD(int vision, int nmove) {
    l0 = std::make_unique<Node_base>(20, 0, true, -500, 500, 0);
    l1 = std::make_unique<Node_base>(30, 1, true, 0, 0, 0);
    construct_zdd(vision, nmove);
  }
  ~ZDD() { /* destruct_zdd() */; }
  //3種類の経路長を求める関数
  void out_info() const noexcept {
    cout << "root->num = " << m_N[0][0]->get_num() << endl;
    cout << "root->lengthmax = " << m_N[0][0]->get_lengthmax() << endl;
    cout << "root->lengthmin = " << m_N[0][0]->get_lengthmin() << endl;
    cout << "root->lengthave = " << (double)m_N[0][0]->get_lengthsum() / (double)m_N[0][0]->get_num() << endl;
  }

  void compute_array(unsigned long long int x, unsigned char* array_objid, size_t size) const noexcept {
    assert(size == 32);
    const Node_base *r = m_N[0][0].get();
    for(std::size_t i = 0; i < size; i++) {
      array_objid[i] = 7;
    }
    while (true) {
      assert(r);
      if(r->get_f() == 30) break; // もし１葉なら
      assert(r->get_f() != 20);
      assert(r->get_f() == 0 || r->get_f() == 1);
      
      if(r->m_left->get_num() <= x) {
	x -= r->m_left->get_num();
	assert(r->get_locid() >= 0 && r->get_locid() <= static_cast<int>(size));
	array_objid[static_cast<std::size_t>(r->get_locid())] = static_cast<unsigned char>(r->get_loc_stateid());
	r = r->m_right;
      } else {
	r = r->m_left;
      }
    }
  }
  
  unsigned long long int compute_id(unsigned char* array_objid, std::size_t size) const noexcept {
    assert(size == 32);
    const Node_base* r = m_N[0][0].get();
    unsigned long long int index = 0;
    while(r->get_f() < 20){
      assert(r->get_locid() >= 0 && r->get_locid() < static_cast<int>(size));
      if(array_objid[static_cast<std::size_t>(r->get_locid())] == static_cast<unsigned char>(r->get_loc_stateid())) {
	index += r->m_left->get_num();
	r = r->m_right;
      } else {
	r = r->m_left;
      }
    }
    return index;
  }
};

#endif
