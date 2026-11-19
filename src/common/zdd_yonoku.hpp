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

// Index configuration with a reduced diagram, preserving their enumeration order.
class ZDD {
private:
  unique_ptr<ZddNode> m_zero_terminal;
  unique_ptr<ZddNode> m_one_terminal;
  // Nodes are owned here; child pointers refer to nodes or the two terminals.
  deque<std::unique_ptr<ZddNode>> m_nodes_by_depth[168];
  void build_diagram(int viewpoint, int iteration) noexcept;

public:
  ZDD(int viewpoint, int iteration) {
    m_zero_terminal = std::make_unique<ZddNode>(20, 0, true, -500, 500, 0);
    m_one_terminal = std::make_unique<ZddNode>(30, 1, true, 0, 0, 0);
    build_diagram(viewpoint, iteration);
  }
  ~ZDD() = default;
  void print_statistics() const noexcept {
    cout << "[zdd] Configurations: " << m_nodes_by_depth[0][0]->get_configuration_count() << endl;
    cout << "[zdd] Maximum path length: " << m_nodes_by_depth[0][0]->get_max_path_length() << endl;
    cout << "[zdd] Minimum path length: " << m_nodes_by_depth[0][0]->get_min_path_length() << endl;
    cout << "[zdd] Mean path length " << (double)m_nodes_by_depth[0][0]->get_total_path_length() / (double)m_nodes_by_depth[0][0]->get_configuration_count() << endl;
    std::size_t node_count = 0;
    for(const auto& nodes : m_nodes_by_depth) node_count += nodes.size();
    cout << "[zdd] Nodes (excluding terminals): " << node_count << endl;
  }

  // Decode an existing ID into 32 object states (reserve, board, reserve)
  void decode_configuration(unsigned long long int configuration_id, unsigned char* object_state_ids, size_t size) const noexcept {
    assert(size == 32);
    const ZddNode *node = m_nodes_by_depth[0][0].get();
    for(std::size_t i = 0; i < size; i++) {
      object_state_ids[i] = 7;
    }
    while (true) {
      assert(node);
      if(node->get_state_code() == 30) break;
      assert(node->get_state_code() != 20);
      assert(node->get_state_code() == 0 || node->get_state_code() == 1);
      
      if(node->m_zero_child->get_configuration_count() <= configuration_id) {
	configuration_id -= node->m_zero_child->get_configuration_count();
	assert(node->get_location_index() >= 0 && node->get_location_index() <= static_cast<int>(size));
	object_state_ids[static_cast<std::size_t>(node->get_location_index())] = static_cast<unsigned char>(node->get_object_state_id());
	node = node->m_one_child;
      } else {
	node = node->m_zero_child;
      }
    }
  }

  // The caller must supply a configuration contained in this diagram
  unsigned long long int encode_configuration(unsigned char* object_state_ids, std::size_t size) const noexcept {
    assert(size == 32);
    const ZddNode* node = m_nodes_by_depth[0][0].get();
    unsigned long long int configuration_id = 0;
    while(node->get_state_code() < 20){
      assert(node->get_location_index() >= 0 && node->get_location_index() < static_cast<int>(size));
      if(object_state_ids[static_cast<std::size_t>(node->get_location_index())] == static_cast<unsigned char>(node->get_object_state_id())) {
	configuration_id += node->m_zero_child->get_configuration_count();
	node = node->m_one_child;
      } else {
	node = node->m_zero_child;
      }
    }
    return configuration_id;
  }
};

#endif
