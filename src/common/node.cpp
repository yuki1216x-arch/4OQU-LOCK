#include "node.hpp"

bool NodeObjectInfo::is_valid() const noexcept {
  // Outside-board sentinel.
  if(m_pieces[0] == 'N') {
    if(m_pieces[1]     != 'N') return false;
    if(m_top_piece     != 'E') return false;
    if(m_height        != 99)  return false;
    if(m_white_count   != 99)  return false;
    if(m_black_count   != 99)  return false;
    if(m_unknown_count != 99)  return false; }
  // Empty location.
  else if(m_pieces[0] == '.') {
    if(m_pieces[1]     != '.') return false;
    if(m_top_piece     != '.') return false;
    if(m_height        != 0)   return false;
    if(m_white_count   != 0)   return false;
    if(m_black_count   != 0)   return false;
    if(m_unknown_count != 0)   return false; }
  // Single piece.
  else if(m_pieces[1] == '.') {
    if(m_top_piece != m_pieces[0]) return false;
    if(m_height    != 1)           return false;
    if(m_pieces[0] == 'W') {
      if(m_white_count   != 1) return false;
      if(m_black_count   != 0) return false;
      if(m_unknown_count != 0) return false; }
    else if(m_pieces[0] == 'B') {
      if(m_white_count   != 0) return false;
      if(m_black_count   != 1) return false;
      if(m_unknown_count != 0) return false; }
    else if(m_pieces[0] == 'U') {
      if(m_white_count   != 0) return false;
      if(m_black_count   != 0) return false;
      if(m_unknown_count != 1) return false; }
    else return false; }
  // Stock of two pieces.
  else if(m_pieces[1] == 'W') {
    if(m_top_piece != 'W') return false;
    if(m_height    != 2)   return false;
    if(m_pieces[0] == 'W') {
      if(m_white_count   != 2) return false;
      if(m_black_count   != 0) return false;
      if(m_unknown_count != 0) return false; }
    else if(m_pieces[0] == 'B') {
      if(m_white_count   != 1) return false;
      if(m_black_count   != 1) return false;
      if(m_unknown_count != 0) return false; }
    else return false; }
  else if(m_pieces[1] == 'B') {
    if(m_top_piece != 'B') return false;
    if(m_height    != 2)   return false;
    if(m_pieces[0] == 'W') {
      if(m_white_count   != 1) return false;
      if(m_black_count   != 1) return false;
      if(m_unknown_count != 0) return false; }
    else if(m_pieces[0] == 'B') {
      if(m_white_count   != 0) return false;
      if(m_black_count   != 2) return false;
      if(m_unknown_count != 0) return false; }
    else return false; }
  else if(m_pieces[1] == 'N') {
    if(m_pieces[0] != 'N') return false; }
  else return false;
  
  return true;
}

const NodeObjectInfo board_object_info[8] =
  { NodeObjectInfo('.', '.', 0, 0, 0, 0, '.'), //0
    NodeObjectInfo('W', '.', 1, 1, 0, 0, 'W'), //1
    NodeObjectInfo('B', '.', 1, 0, 1, 0, 'B'), //2
    NodeObjectInfo('W', 'W', 2, 2, 0, 0, 'W'), //3
    NodeObjectInfo('W', 'B', 2, 1, 1, 0, 'B'), //4
    NodeObjectInfo('B', 'W', 2, 1, 1, 0, 'W'), //5
    NodeObjectInfo('B', 'B', 2, 0, 2, 0, 'B'), //6
    NodeObjectInfo('N', 'N', 99, 99, 99, 99, 'E') // 7
};

const NodeObjectInfo hand_object_info[5] =
  { NodeObjectInfo('.', '.', 0, 0, 0, 0, '.'), //0
    NodeObjectInfo('W', '.', 1, 1, 0, 0, 'W'), //1
    NodeObjectInfo('B', '.', 1, 0, 1, 0, 'B'), //2
    NodeObjectInfo('U', '.', 1, 0, 0, 1, 'U'), //3
    NodeObjectInfo('N', 'N', 99, 99, 99, 99, 'E') //4
};

// Derive the next node from the parent and its selected branch.
ZddNode::ZddNode(ZddNode& parent, int branch) {
  if(parent.m_location_index < 8) {
    if(parent.m_object_state_id == 2) {
      m_location_index = parent.m_location_index + 1, m_object_state_id = 0;
    } else {
      m_location_index = parent.m_location_index, m_object_state_id = parent.m_object_state_id + 1;
    }

    // Carry the selection state until advancing to the next location.
    if(parent.m_object_state_id != 2 && (parent.m_state_code == 1 || branch == 1)) m_state_code = 1;
    else m_state_code = 0;

    // Record the first nonempty slot in the opponent hand.
    if(parent.m_object_state_id != 0 && branch == 1) m_opponent_hand_started = 1;
    else m_opponent_hand_started = parent.m_opponent_hand_started;
    
    m_player_hand_started = parent.m_player_hand_started, m_player_hand_color_selected = parent.m_player_hand_color_selected, m_unknown_count = parent.m_unknown_count;
    
    if(branch == 1) {
      if(parent.m_location_index < 4) {
	m_opponent_hand_first_white_count = parent.m_opponent_hand_first_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
	m_opponent_hand_first_black_count = parent.m_opponent_hand_first_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
	m_opponent_hand_second_white_count = parent.m_opponent_hand_second_white_count, m_opponent_hand_second_black_count = parent.m_opponent_hand_second_black_count;
      } else {
	m_opponent_hand_first_white_count= parent.m_opponent_hand_first_white_count, m_opponent_hand_first_black_count = parent.m_opponent_hand_first_black_count;
	m_opponent_hand_second_white_count = parent.m_opponent_hand_second_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
	m_opponent_hand_second_black_count = parent.m_opponent_hand_second_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
      }
      m_white_count = parent.m_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
      m_black_count = parent.m_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
    } else {
      m_opponent_hand_first_white_count = parent.m_opponent_hand_first_white_count, m_opponent_hand_first_black_count = parent.m_opponent_hand_first_black_count;
      m_opponent_hand_second_white_count = parent.m_opponent_hand_second_white_count, m_opponent_hand_second_black_count = parent.m_opponent_hand_second_black_count;
      m_white_count = parent.m_white_count, m_black_count = parent.m_black_count;
    }
    
    m_player_hand_first_white_count = parent.m_player_hand_first_white_count, m_player_hand_first_black_count = parent.m_player_hand_first_black_count;
    m_player_hand_second_white_count = parent.m_player_hand_second_white_count, m_player_hand_second_black_count = parent.m_player_hand_second_black_count;
    m_board_white_count = parent.m_board_white_count, m_board_black_count = parent.m_board_black_count;
  } else if(parent.m_location_index < 24) {
    if(parent.m_object_state_id == 6) {
      m_location_index = parent.m_location_index + 1, m_object_state_id = 0;
    } else {
      m_location_index = parent.m_location_index, m_object_state_id = parent.m_object_state_id + 1;
    }

    // Carry the selection state until advancing to the next location.
    if(parent.m_object_state_id != 6 && (parent.m_state_code == 1 || branch == 1)) m_state_code = 1;
    else m_state_code = 0;
    
    m_opponent_hand_started = parent.m_opponent_hand_started, m_player_hand_started = parent.m_player_hand_started;
    m_player_hand_color_selected = parent.m_player_hand_color_selected, m_unknown_count = parent.m_unknown_count;
    
    if(branch == 1) {
      m_white_count = parent.m_white_count + board_object_info[parent.m_object_state_id].get_white_count();
      m_black_count = parent.m_black_count + board_object_info[parent.m_object_state_id].get_black_count();
      m_board_white_count = parent.m_board_white_count + board_object_info[parent.m_object_state_id].get_white_count();
      m_board_black_count = parent.m_board_black_count + board_object_info[parent.m_object_state_id].get_black_count();
    } else {
      m_white_count = parent.m_white_count, m_black_count = parent.m_black_count;
      m_board_white_count = parent.m_board_white_count, m_board_black_count = parent.m_board_black_count;
    }
    
    m_opponent_hand_first_white_count = parent.m_opponent_hand_first_white_count, m_opponent_hand_first_black_count = parent.m_opponent_hand_first_black_count;
    m_opponent_hand_second_white_count = parent.m_opponent_hand_second_white_count, m_opponent_hand_second_black_count = parent.m_opponent_hand_second_black_count;
    m_player_hand_first_white_count = parent.m_player_hand_first_white_count, m_player_hand_first_black_count = parent.m_player_hand_first_black_count;
    m_player_hand_second_white_count = parent.m_player_hand_second_white_count, m_player_hand_second_black_count = parent.m_player_hand_second_black_count;
  } else {
    if(parent.m_object_state_id == 3) {
      m_location_index = parent.m_location_index + 1, m_object_state_id = 0;
    } else {
      m_location_index = parent.m_location_index, m_object_state_id = parent.m_object_state_id + 1;
    }

    // Carry the selection state until advancing to the next location.
    if(parent.m_object_state_id != 3 && (parent.m_state_code == 1 || branch == 1)) m_state_code = 1;
    else m_state_code = 0;

    // Record the first nonempty slot in the player hand.
    if(parent.m_object_state_id != 0 && branch == 1) m_player_hand_started = 1;
    else m_player_hand_started = parent.m_player_hand_started;

    // once a white or black piece is placed in the player's hand, only unknown pieces may follow
    if(branch == 1 && (parent.m_object_state_id == 1 || parent.m_object_state_id == 2)) m_player_hand_color_selected = 1;
    else m_player_hand_color_selected = parent.m_player_hand_color_selected;
    
    m_opponent_hand_started = parent.m_opponent_hand_started, m_player_hand_threshold = parent.m_player_hand_threshold;
    
    if(branch == 1) {
      if(parent.m_location_index < 28) {
	m_player_hand_first_white_count = parent.m_player_hand_first_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
	m_player_hand_first_black_count = parent.m_player_hand_first_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
	m_player_hand_second_white_count = parent.m_player_hand_second_white_count, m_player_hand_second_black_count = parent.m_player_hand_second_black_count;
      } else {
	m_player_hand_first_white_count = parent.m_player_hand_first_white_count, m_player_hand_first_black_count = parent.m_player_hand_first_black_count;
	m_player_hand_second_white_count = parent.m_player_hand_second_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
	m_player_hand_second_black_count = parent.m_player_hand_second_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
      }
      m_white_count = parent.m_white_count + hand_object_info[parent.m_object_state_id].get_white_count();
      m_black_count = parent.m_black_count + hand_object_info[parent.m_object_state_id].get_black_count();
      m_unknown_count = parent.m_unknown_count + hand_object_info[parent.m_object_state_id].get_unknown_count();
    } else {
      m_player_hand_first_white_count = parent.m_player_hand_first_white_count, m_player_hand_first_black_count = parent.m_player_hand_first_black_count;
      m_player_hand_second_white_count = parent.m_player_hand_second_white_count, m_player_hand_second_black_count = parent.m_player_hand_second_black_count;
      m_white_count = parent.m_white_count, m_black_count = parent.m_black_count, m_unknown_count = parent.m_unknown_count;
    } 
    m_opponent_hand_first_white_count = parent.m_opponent_hand_first_white_count, m_opponent_hand_first_black_count = parent.m_opponent_hand_first_black_count;
    m_opponent_hand_second_white_count = parent.m_opponent_hand_second_white_count, m_opponent_hand_second_black_count = parent.m_opponent_hand_second_black_count;
    m_board_white_count = parent.m_board_white_count, m_board_black_count = parent.m_board_black_count;
  }
  m_configuration_count = 0, m_statistics_computed = false;
}

bool ZddNode::branch_leads_to_zero_terminal(int depth, int branch, int iteration) const noexcept {
  assert(0 <= depth && depth <= 167);
  assert(branch == 0 || branch == 1);
  assert(0 <= iteration && iteration < 16);
  int remaining_placements = 16 - iteration;

  // Invalid: attempting to place an object on an already occupied square.
  if(branch == 1 && m_state_code == 1) return true; 
  if(depth < 24) {   // opponent's pieces in hand
    // Invalid: placing a non-empty value on squares beyond the current square number (remaining moves)
    if(branch == 1 && m_object_state_id != 0 && m_opponent_hand_threshold > remaining_placements) return true;
    if(branch == 1 && m_object_state_id == 0 && m_opponent_hand_threshold <= remaining_placements) return true;
    if(branch == 0 && m_object_state_id == 0 && m_opponent_hand_threshold > remaining_placements) return true;

    // Invalid: placing an empty value after a piece has been placed in hand.
    if(branch == 1 && m_object_state_id == 0 && m_opponent_hand_started == 1) return true;
    // Invalid: no object is selected for this location.
    if(branch == 0 && depth % 3 == 2 && m_state_code == 0) return true;
    if(depth < 12) {   // set1
      // Invalid: exceeding the number of white pieces in set1
      if(branch == 1 && m_object_state_id == 1 && m_opponent_hand_first_white_count >= opponent_hand_white_limit()) return true;
      // Invalid: exceeding the number of black pieces in set1
      if(branch == 1 && m_object_state_id == 2 && m_opponent_hand_first_black_count >= opponent_hand_black_limit()) return true;
    } else {   // set2
      // Invalid: exceeding the number of white pieces in set2
      if(branch == 1 && m_object_state_id == 1 && m_opponent_hand_second_white_count >= opponent_hand_white_limit()) return true;
      // Invalid: exceeding the number of black pieces in set2
      if(branch == 1 && m_object_state_id == 2 && m_opponent_hand_second_black_count >= opponent_hand_black_limit()) return true;
    }
    return false;
  }
  if(depth < 136) {   // on the board
    // Invalid: no object is selected for this location.
    if(branch == 0 && (depth - 24) % 7 == 6 && m_state_code == 0) return true;
    
    // Invalid: placing more pieces than the limit
    if(branch == 1 && m_state_code == 0) {
      int object_state_id = (depth - 24) % 7;
      if(remaining_placements > opponent_color_code() + 11) {
	if(8 - player_color_count_with_object(object_state_id) < 6 && 8 - opponent_color_count_with_object(object_state_id) < 2) return true;
	if(remaining_placements <= opponent_color_code() + 13) {
	  if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 7) return true;
	}
      } else if(remaining_placements > opponent_color_code() + 9) {
	if(8 - player_color_count_with_object(object_state_id) < 5 && 8 - opponent_color_count_with_object(object_state_id) < 2) return true;
	if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 6) return true;
      } else if(remaining_placements > opponent_color_code() + 7) {
	if(8 - player_color_count_with_object(object_state_id) < 4 && 8 - opponent_color_count_with_object(object_state_id) < 2) return true;
	if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 5) return true;
      } else if(remaining_placements > opponent_color_code() + 3) {
	if(8 - player_color_count_with_object(object_state_id) < 3 && 8 - opponent_color_count_with_object(object_state_id) < 1) return true;
	if(remaining_placements <= opponent_color_code() + 5) {
	  if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 3) return true;
	}
      } else if(remaining_placements > opponent_color_code() + 1) {
	if(8 - player_color_count_with_object(object_state_id) < 2 && 8 - opponent_color_count_with_object(object_state_id) < 1) return true;
	if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 2) return true;
      } else if(remaining_placements > opponent_color_code() - 1) {
	if(8 - player_color_count_with_object(object_state_id) < 1 && 8 - opponent_color_count_with_object(object_state_id) < 1) return true;
	if(16 - player_color_count_with_object(object_state_id) - opponent_color_count_with_object(object_state_id) < 1) return true;
      } else {
	assert(remaining_placements >= 0);
	if(8 - player_color_count_with_object(object_state_id) < 0 || 8 - opponent_color_count_with_object(object_state_id) < 0) return true;
      }
    }

    if(depth == 135) {
      if(branch == 1 && m_state_code == 0) {
	// Invalid: the number of black pieces exceeds that of white by two
	if(remaining_placements > 8 && (m_board_black_count + 2) - m_board_white_count > 1 && ok_opp_set1_num_gb()) return true;
	if(remaining_placements <= 8 && (m_board_black_count + 2) - m_board_white_count > 1 && ok_opp_set2_num_gb()) return true;

	// Invalid: the number of white pieces exceeds that of black by two
	if(remaining_placements > 8 && m_board_white_count - (m_board_black_count + 2) > 1 && ok_opp_set1_num_gw()) return true;
	if(remaining_placements <= 8 && m_board_white_count - (m_board_black_count + 2) > 1 && ok_opp_set2_num_gw()) return true;

	// Invalid: pieces on the board that do not exist in hand
	if(remaining_placements > 8 && opponent_hand_white_limit() - m_opponent_hand_first_white_count > m_board_white_count) return true;
	if(remaining_placements > 8 && opponent_hand_black_limit() - m_opponent_hand_first_black_count > m_board_black_count + 2) return true;
	if(remaining_placements <= 8 && 4 + (opponent_hand_white_limit() - m_opponent_hand_second_white_count) > m_board_white_count) return true;
	if(remaining_placements <= 8 && 4 + (opponent_hand_black_limit() - m_opponent_hand_second_black_count) > m_board_black_count + 2) return true;

	// Invalid: (white - black) piece count on the board is out of bounds
	if(remaining_placements - opponent_color_code() > 13) {
	  if(8 - player_color_count_with_object(6) != 6 || 8 - opponent_color_count_with_object(6) != 2) return true;
	} else if(remaining_placements - opponent_color_code() > 7) {
	  if(8 - player_color_count_with_object(6) > (remaining_placements - opponent_color_code()) / 2 || 8 - opponent_color_count_with_object(6) > 2) return true;
	} else if(remaining_placements - opponent_color_code() > 5) {
	  if(8 - player_color_count_with_object(6) != 3 || 8 - opponent_color_count_with_object(6) != 1) return true;
	} else if(remaining_placements - opponent_color_code() >= 0) {
	  if(8 - player_color_count_with_object(6) > (remaining_placements - opponent_color_code() + 2) / 2 || 8 - opponent_color_count_with_object(6) > 1) return true;
	} else {
	  assert(remaining_placements >= 0);
	  if(8 - player_color_count_with_object(6) != 0 || 8 - opponent_color_count_with_object(6) != 0) return true;
	}
      } else if(branch == 0 && m_state_code == 1) {
	// Invalid: the number of black pieces exceeds that of white by two
	if(remaining_placements > 8 && m_board_black_count - m_board_white_count > 1 && ok_opp_set1_num_gb()) return true;
	if(remaining_placements <= 8 && m_board_black_count - m_board_white_count > 1 && ok_opp_set2_num_gb()) return true;

	// Invalid: the number of white pieces exceeds that of black by two
	if(remaining_placements > 8 && m_board_white_count - m_board_black_count > 1 && ok_opp_set1_num_gw()) return true;
	if(remaining_placements <= 8 && m_board_white_count - m_board_black_count > 1 && ok_opp_set2_num_gw()) return true;

	// Invalid: pieces on the board that do not exist in hand
	if(remaining_placements > 8 && opponent_hand_white_limit() - m_opponent_hand_first_white_count > m_board_white_count) return true;
	if(remaining_placements > 8 && opponent_hand_black_limit() - m_opponent_hand_first_black_count > m_board_black_count) return true;
	if(remaining_placements <= 8 && 4 + (opponent_hand_white_limit() - m_opponent_hand_second_white_count) > m_board_white_count) return true;
	if(remaining_placements <= 8 && 4 + (opponent_hand_black_limit() - m_opponent_hand_second_black_count) > m_board_black_count) return true;

	if(remaining_placements - opponent_color_code() > 13) {
	  if(8 - player_color_count_with_object(0) != 6 || 8 - opponent_color_count_with_object(0) != 2) return true;
	} else if(remaining_placements - opponent_color_code() > 7) {
	  if(8 - player_color_count_with_object(0) > (remaining_placements - opponent_color_code()) / 2 || 8 - opponent_color_count_with_object(0) > 2) return true;
	} else if(remaining_placements - opponent_color_code() > 5) {
	  if(8 - player_color_count_with_object(0) != 3 || 8 - opponent_color_count_with_object(0) != 1) return true;
	} else if(remaining_placements - opponent_color_code() >= 0) {
	  if(8 - player_color_count_with_object(0) > (remaining_placements - opponent_color_code() + 2) / 2 || 8 - opponent_color_count_with_object(0) > 1) return true;
	} else {
	  assert(remaining_placements >= 0);
	  if(8 - player_color_count_with_object(0) != 0 || 8 - opponent_color_count_with_object(0) != 0) return true;
	}
      }
    }
    return false;
  }
  
  // player's pieces in hand
  // Invalid: placing a non-empty value on squares beyond the current square number (remaining moves)
  if(branch == 1 && m_object_state_id != 0 && m_player_hand_threshold > remaining_placements) return true;
  if(branch == 1 && m_object_state_id == 0 && m_player_hand_threshold <= remaining_placements) return true;

  // Invalid: placing an empty value after a piece has been placed in hand.
  if(branch == 1 && m_object_state_id == 0 && m_player_hand_started == 1) return true;
  // Invalid: placing a white or black piece after a known color has been selected.
  if(branch == 1 && (m_object_state_id == 1 || m_object_state_id == 2) && m_player_hand_color_selected == 1) return true;
  // Invalid: placing an unknown piece before any white or black piece.
  if(branch == 1 && m_object_state_id == 3 && m_player_hand_color_selected == 0) return true;
  // Invalid: no object is selected for this location.
  if(branch == 0 && (depth - 136) % 4 == 3 && m_state_code == 0) return true;

  if(depth < 152) {   // set3
    if(branch == 1) {
      if(m_object_state_id == opponent_color_code() && opponent_minus_player_board_count() > 1) return true;
      if(m_object_state_id == opponent_color_code() && opponent_color_count_before_branch() == 7) return true;
      
      // Invalid: exceeding the number of white pieces in set3
      if(m_object_state_id == 1 && m_player_hand_first_white_count >= opponent_hand_black_limit()) return true;
      // Invalid: exceeding the number of black pieces in set3
      if(m_object_state_id == 2 && m_player_hand_first_black_count >= opponent_hand_white_limit()) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(m_object_state_id == 1 && m_board_white_count - (opponent_hand_white_limit() - m_opponent_hand_first_white_count) - (opponent_hand_white_limit() - m_opponent_hand_second_white_count) >= opponent_hand_black_limit()) return true;
      if(m_object_state_id == 2 && m_board_black_count - (opponent_hand_black_limit() - m_opponent_hand_first_black_count) - (opponent_hand_black_limit() - m_opponent_hand_second_black_count) >= opponent_hand_white_limit()) return true;
    }
  } else {   // set4
    if(branch == 1) {
      if(m_object_state_id == opponent_color_code() && opponent_minus_player_board_count() > 1) return true;
      if(m_object_state_id == opponent_color_code() && opponent_color_count_before_branch() == 8) return true;
      
      // Invalid: exceeding the number of white pieces in set4
      if(m_object_state_id == 1 && m_player_hand_second_white_count >= opponent_hand_black_limit()) return true;
      // Invalid: exceeding the number of black pieces in set4
      if(m_object_state_id == 2 && m_player_hand_second_black_count >= opponent_hand_white_limit()) return true;
      // Invalid: more white/black pieces on the board than used by the player
      if(m_object_state_id == 1 && m_board_white_count - (opponent_hand_white_limit() - m_opponent_hand_first_white_count) - (opponent_hand_white_limit() - m_opponent_hand_second_white_count) - opponent_hand_black_limit() >= opponent_hand_black_limit()) return true;
      if(m_object_state_id == 2 && m_board_black_count - (opponent_hand_black_limit() - m_opponent_hand_first_black_count) - (opponent_hand_black_limit() - m_opponent_hand_second_black_count) - opponent_hand_white_limit() >= opponent_hand_white_limit()) return true;
    }

    // Invalid: total number of pieces exceeds the limit
    if(depth == 167 && branch == 1 && m_state_code == 0 && !(m_white_count + m_black_count + m_unknown_count == 15 && m_white_count <= 8 && m_black_count <= 8)) return true;
    if(depth == 167 && branch == 0 && m_state_code == 1 && !(m_white_count + m_black_count + m_unknown_count == 16 && m_white_count <= 8 && m_black_count <= 8)) return true;
  }
  return false;
}

int BlackViewNode::player_color_count_with_object(int object_state_id) const noexcept { return get_board_and_opponent_hand_black_count() + board_object_info[object_state_id].get_black_count(); }
int BlackViewNode::opponent_color_count_with_object(int object_state_id) const noexcept { return get_board_and_opponent_hand_white_count() + board_object_info[object_state_id].get_white_count(); }
int WhiteViewNode::player_color_count_with_object(int object_state_id) const noexcept { return get_board_and_opponent_hand_white_count() + board_object_info[object_state_id].get_white_count(); }
int WhiteViewNode::opponent_color_count_with_object(int object_state_id) const noexcept { return get_board_and_opponent_hand_black_count() + board_object_info[object_state_id].get_black_count(); }
