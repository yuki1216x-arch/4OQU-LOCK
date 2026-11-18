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

// Piece counts and stack contents for a single object state.
class NodeObjectInfo {
  char m_pieces[2];
  char m_height;    // height
  char m_white_count;   // Number of white pieces.
  char m_black_count;   // Number of black pieces.
  char m_unknown_count;   // Number of pieces with hidden colors.
  char m_top_piece;   // type of the top piece

public:
  NodeObjectInfo(char piece0, char piece1, char height, char white_count, char black_count,
	       char unknown_count, char top_piece) noexcept
    : m_pieces{piece0, piece1}, m_height(height), m_white_count(white_count), m_black_count(black_count), m_unknown_count(unknown_count), m_top_piece(top_piece) {
      assert(is_valid()); }
  bool is_outside_board() const noexcept { return m_pieces[0] == 99; }
  int get_piece(int i) const noexcept {
    assert(i == 0 && i == 1);
    return m_pieces[i];
  }
  int get_white_count() const noexcept { return m_white_count; }
  int get_black_count() const noexcept { return m_black_count; }
  int get_unknown_count() const noexcept { return m_unknown_count; }
  bool is_valid() const noexcept;
  bool operator==(const NodeObjectInfo &o) const noexcept {
    assert(o.is_valid());
    return (m_pieces[0] == o.m_pieces[0] && m_pieces[1] == o.m_pieces[1]);
  }
};

// Track the constraints and cached subtree statistics for a ZDD node.
class ZddNode {
  char m_white_count;   // Number of white pieces up to the current node
  char m_black_count;   // Number of black pieces up to the current node
  char m_unknown_count;   // Number of unknown pieces up to the current node
  char m_state_code;   // Object selection state, terminal code, or redundant marker.
  char m_opponent_hand_started;   // A nonempty opponent hand slot has been selected.
  char m_player_hand_started;   // A nonempty player hand slot has been selected.
  char m_player_hand_color_selected;   // A known color has been selected in the player hand.

  // Each hand has two groups of four slots; track the board separately.
  char m_opponent_hand_first_white_count, m_opponent_hand_first_black_count, m_opponent_hand_second_white_count, m_opponent_hand_second_black_count,
    m_player_hand_first_white_count, m_player_hand_first_black_count, m_player_hand_second_white_count, m_player_hand_second_black_count,
    m_board_white_count, m_board_black_count;
  char m_opponent_hand_threshold, m_player_hand_threshold;   // Thresholds that control which hand slots must be empty.
  unsigned char m_location_index;
  unsigned char m_object_state_id;
  unsigned long long int m_configuration_count;   // number of reachable leaf nodes with value 1 (number of possible board configurations)
  bool m_statistics_computed;   // for compute_subtree_statistics to compute path length
  int m_max_path_length;   // holds the maximum path length value
  int m_min_path_length;   // holds the minimum path length value
  unsigned long long int m_total_path_length;   // total path length for computing the average path length

  virtual int opponent_hand_white_limit() const noexcept { return -1; }
  virtual int opponent_hand_black_limit() const noexcept { return -1; }
  virtual bool ok_opp_set1_num_gw() const noexcept { return true; }
  virtual bool ok_opp_set1_num_gb() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gw() const noexcept { return true; }
  virtual bool ok_opp_set2_num_gb() const noexcept { return true; }
  virtual int opponent_color_count_for_one_branch() const noexcept { return -1; }
  virtual int opponent_color_count_before_branch() const noexcept { return -1; }
  virtual int player_color_count_with_object(int object_state_id) const noexcept { return -1; }
  virtual int opponent_color_count_with_object(int object_state_id) const noexcept { return -1; }
  virtual int hand_difference_threshold() const noexcept { return -1; }
  virtual int opponent_minus_player_board_count() const noexcept { return -1; }
  virtual int opponent_color_code() const noexcept { return -1; }
  virtual int initial_opponent_hand_threshold() const noexcept { return -1; }
  virtual int initial_player_hand_threshold() const noexcept { return -1; }
  virtual void refresh_opponent_hand_threshold() noexcept {}
  virtual void refresh_player_hand_threshold() noexcept {}

protected:
  ZddNode()
    : m_white_count(0), m_black_count(0), m_unknown_count(0), m_state_code(0), m_opponent_hand_started(0), m_player_hand_started(0), m_player_hand_color_selected(0),
      m_opponent_hand_first_white_count(0), m_opponent_hand_first_black_count(0), m_opponent_hand_second_white_count(0), m_opponent_hand_second_black_count(0),
      m_player_hand_first_white_count(0), m_player_hand_first_black_count(0), m_player_hand_second_white_count(0), m_player_hand_second_black_count(0),
      m_board_white_count(0), m_board_black_count(0), m_opponent_hand_threshold(0), m_player_hand_threshold(0),
      m_location_index(0), m_object_state_id(0), m_configuration_count(0), m_statistics_computed(false) {} 
  ZddNode(ZddNode& parent, int branch);
  int get_opponent_hand_first_white_count() const noexcept { return m_opponent_hand_first_white_count; }
  int get_opponent_hand_first_black_count() const noexcept { return m_opponent_hand_first_black_count; }
  int get_opponent_hand_second_white_count() const noexcept { return m_opponent_hand_second_white_count; }
  int get_opponent_hand_second_black_count() const noexcept { return m_opponent_hand_second_black_count; }
  int get_board_white_count() const noexcept { return m_board_white_count; }
  int get_board_black_count() const noexcept { return m_board_black_count; }
  int get_board_and_opponent_hand_white_count() const noexcept { return m_board_white_count + m_opponent_hand_first_white_count + m_opponent_hand_second_white_count; }
  int get_board_and_opponent_hand_black_count() const noexcept { return m_board_black_count + m_opponent_hand_first_black_count + m_opponent_hand_second_black_count; }
  int get_current_opponent_hand_threshold() const noexcept { return m_opponent_hand_threshold; }
  int get_current_player_hand_threshold() const noexcept { return m_player_hand_threshold; }

public:
  // The ZDD owns the nodes; these pointers link the two outgoing branches.
  ZddNode* m_zero_child;
  ZddNode* m_one_child;
  ZddNode(char terminal_code, unsigned long long int configuration_count, bool statistics_computed, int max_path_length, int min_path_length, unsigned long long int total_path_length)
    : m_white_count(terminal_code), m_black_count(terminal_code), m_unknown_count(terminal_code),
      m_state_code(terminal_code), m_opponent_hand_started(terminal_code), m_player_hand_started(terminal_code), m_player_hand_color_selected(terminal_code),
      m_opponent_hand_first_white_count(terminal_code), m_opponent_hand_first_black_count(terminal_code),
      m_opponent_hand_second_white_count(terminal_code), m_opponent_hand_second_black_count(terminal_code),
      m_player_hand_first_white_count(terminal_code), m_player_hand_first_black_count(terminal_code),
      m_player_hand_second_white_count(terminal_code), m_player_hand_second_black_count(terminal_code),
      m_board_white_count(terminal_code), m_board_black_count(terminal_code),
      m_opponent_hand_threshold(terminal_code), m_player_hand_threshold(terminal_code), m_location_index(0), m_object_state_id(0),
      m_configuration_count(configuration_count), m_statistics_computed(statistics_computed),
      m_max_path_length(max_path_length), m_min_path_length(min_path_length), m_total_path_length(total_path_length),
      m_zero_child(nullptr), m_one_child(nullptr) {}

  virtual ~ZddNode() {}
  void initialize_hand_thresholds() noexcept {
    m_opponent_hand_threshold = initial_opponent_hand_threshold();
    m_player_hand_threshold = initial_player_hand_threshold();
  }
  void update_hand_thresholds(ZddNode& parent) noexcept {
    if(parent.m_location_index < 8) {
      if(parent.m_object_state_id == 2) m_opponent_hand_threshold = parent.m_opponent_hand_threshold - 2;
      else m_opponent_hand_threshold = parent.m_opponent_hand_threshold;
      m_player_hand_threshold = parent.m_player_hand_threshold;
    } else if(parent.m_location_index < 24) {
      m_opponent_hand_threshold = parent.m_opponent_hand_threshold;
      m_player_hand_threshold = parent.m_player_hand_threshold;
    } else {
      if(parent.m_object_state_id == 3) m_player_hand_threshold = parent.m_player_hand_threshold - 2;
      else m_player_hand_threshold = parent.m_player_hand_threshold;
      m_opponent_hand_threshold = parent.m_opponent_hand_threshold;
    }
  }
  // Test whether choosing this branch violates a placement constraint.
  bool branch_leads_to_zero_terminal(int depth, int branch, int iteration) const noexcept;
  int get_location_index() const noexcept { return m_location_index; }
  int get_object_state_id() const noexcept { return m_object_state_id; }
  unsigned long long int get_configuration_count() const noexcept { return m_configuration_count; }
  int get_state_code() const noexcept { return m_state_code; }
  int get_max_path_length() const noexcept { return m_max_path_length; }
  int get_min_path_length() const noexcept { return m_min_path_length; }
  unsigned long long int get_total_path_length() const noexcept { return m_total_path_length; }
  // Cache the configuration count and path lengths for this subtree.
  void compute_subtree_statistics() noexcept {
    assert(m_zero_child != nullptr && m_one_child != nullptr);
    if(!m_zero_child->m_statistics_computed) m_zero_child->compute_subtree_statistics();
    if(!m_one_child->m_statistics_computed) m_one_child->compute_subtree_statistics();
    m_configuration_count = m_zero_child->m_configuration_count + m_one_child->m_configuration_count;
    m_max_path_length = max(m_zero_child->m_max_path_length, m_one_child->m_max_path_length) + 1;
    m_min_path_length = min(m_zero_child->m_min_path_length, m_one_child->m_min_path_length) + 1;
    m_total_path_length = m_zero_child->m_total_path_length + m_one_child->m_total_path_length + m_configuration_count;
    m_statistics_computed = true;
  }
  void mark_redundant() noexcept { m_state_code = 40; }
  bool is_redundant() const noexcept { return m_state_code == 40; }
  bool is_zero_terminal() const noexcept { return m_state_code == 20; }
  bool is_one_terminal() const noexcept { return m_state_code == 30; }
  bool has_equivalent_opponent_hand_state(ZddNode& c) const noexcept {
    return m_state_code == c.m_state_code && m_opponent_hand_first_white_count == c.m_opponent_hand_first_white_count
      && m_opponent_hand_second_white_count == c.m_opponent_hand_second_white_count
      && m_player_hand_first_white_count == c.m_player_hand_first_white_count
      && m_player_hand_second_white_count == c.m_player_hand_second_white_count
      && m_opponent_hand_first_black_count == c.m_opponent_hand_first_black_count
      && m_opponent_hand_second_black_count == c.m_opponent_hand_second_black_count
      && m_player_hand_first_black_count == c.m_player_hand_first_black_count
      && m_player_hand_second_black_count == c.m_player_hand_second_black_count
      && m_white_count == c.m_white_count && m_black_count == c.m_black_count;
  }
  bool has_equivalent_board_state(ZddNode& c) const noexcept {
    return m_state_code == c.m_state_code && m_white_count == c.m_white_count && m_black_count == c.m_black_count
      && m_white_count - m_opponent_hand_first_white_count - m_opponent_hand_second_white_count - m_player_hand_first_white_count - m_player_hand_second_white_count
      == c.m_white_count - c.m_opponent_hand_first_white_count - c.m_opponent_hand_second_white_count - c.m_player_hand_first_white_count - c.m_player_hand_second_white_count
      && m_black_count - m_opponent_hand_first_black_count - m_opponent_hand_second_black_count - m_player_hand_first_black_count - m_player_hand_second_black_count
      == c.m_black_count - c.m_opponent_hand_first_black_count - c.m_opponent_hand_second_black_count - c.m_player_hand_first_black_count - c.m_player_hand_second_black_count;
  }
  bool has_equivalent_player_hand_state(ZddNode& c) const noexcept {
    return m_state_code == c.m_state_code && m_white_count == c.m_white_count && m_black_count == c.m_black_count && m_unknown_count == c.m_unknown_count
      && m_white_count - m_opponent_hand_first_white_count - m_opponent_hand_second_white_count - m_player_hand_first_white_count - m_player_hand_second_white_count
      == c.m_white_count - c.m_opponent_hand_first_white_count - c.m_opponent_hand_second_white_count - c.m_player_hand_first_white_count - c.m_player_hand_second_white_count
      && m_black_count - m_opponent_hand_first_black_count - m_opponent_hand_second_black_count - m_player_hand_first_black_count - m_player_hand_second_black_count
      == c.m_black_count - c.m_opponent_hand_first_black_count - c.m_opponent_hand_second_black_count - c.m_player_hand_first_black_count - c.m_player_hand_second_black_count
      && m_opponent_hand_first_white_count == c.m_opponent_hand_first_white_count && m_opponent_hand_second_white_count == c.m_opponent_hand_second_white_count
      && m_player_hand_first_white_count == c.m_player_hand_first_white_count && m_player_hand_second_white_count == c.m_player_hand_second_white_count
      && m_opponent_hand_first_black_count == c.m_opponent_hand_first_black_count && m_opponent_hand_second_black_count == c.m_opponent_hand_second_black_count
      && m_player_hand_first_black_count == c.m_player_hand_first_black_count && m_player_hand_second_black_count == c.m_player_hand_second_black_count;
  }
};

class BlackViewNode : public ZddNode {
  int opponent_hand_white_limit() const noexcept override { return 3; }
  int opponent_hand_black_limit() const noexcept override { return 1; }
  bool ok_opp_set1_num_gw() const noexcept override { return get_opponent_hand_first_black_count() == 0; }
  bool ok_opp_set1_num_gb() const noexcept override { return get_opponent_hand_first_black_count() == 1; }
  bool ok_opp_set2_num_gw() const noexcept override { return get_opponent_hand_second_black_count() == 0; }
  bool ok_opp_set2_num_gb() const noexcept override { return get_opponent_hand_second_black_count() == 1; }
  int opponent_color_count_for_one_branch() const noexcept override { return get_board_and_opponent_hand_white_count(); }
  int opponent_color_count_before_branch() const noexcept override { return get_board_and_opponent_hand_white_count(); }
  int player_color_count_with_object(int object_state_id) const noexcept override;
  int opponent_color_count_with_object(int object_state_id) const noexcept override;
  int hand_difference_threshold() const noexcept override { return 9; }
  int opponent_minus_player_board_count() const noexcept override { return get_board_white_count() - get_board_black_count(); }
  int opponent_color_code() const noexcept override { return 1; }
  int initial_opponent_hand_threshold() const noexcept override { return 16; }
  int initial_player_hand_threshold() const noexcept override { return 15; }

public:
  BlackViewNode() : ZddNode() { initialize_hand_thresholds(); }
  BlackViewNode(ZddNode& parent, int branch) : ZddNode(parent, branch) { update_hand_thresholds(parent); }
  virtual ~BlackViewNode() noexcept {}
};

class WhiteViewNode : public ZddNode {
  int opponent_hand_white_limit() const noexcept override { return 1; }
  int opponent_hand_black_limit() const noexcept override { return 3; }
  bool ok_opp_set1_num_gw() const noexcept override { return get_opponent_hand_first_white_count() == 1; }
  bool ok_opp_set1_num_gb() const noexcept override { return get_opponent_hand_first_white_count() == 0; }
  bool ok_opp_set2_num_gw() const noexcept override { return get_opponent_hand_second_white_count() == 1; }
  bool ok_opp_set2_num_gb() const noexcept override { return get_opponent_hand_second_white_count() == 0; }
  int opponent_color_count_for_one_branch() const noexcept override { return get_board_and_opponent_hand_black_count() + 2; }
  int opponent_color_count_before_branch() const noexcept override { return get_board_and_opponent_hand_black_count(); }
  int player_color_count_with_object(int object_state_id) const noexcept override;
  int opponent_color_count_with_object(int object_state_id) const noexcept override;
  int hand_difference_threshold() const noexcept override { return 10; }
  int opponent_minus_player_board_count() const noexcept override { return get_board_black_count() - get_board_white_count(); }
  int opponent_color_code() const noexcept override { return 2; }
  int initial_opponent_hand_threshold() const noexcept override { return 15; }
  int initial_player_hand_threshold() const noexcept override { return 16; }

public:
  WhiteViewNode() : ZddNode() { initialize_hand_thresholds(); }
  WhiteViewNode(ZddNode& parent, int branch) : ZddNode(parent, branch) { update_hand_thresholds(parent); }
  virtual ~WhiteViewNode() noexcept {}
};

#endif
