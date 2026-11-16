#include <cerrno>
#include <cstdlib>

#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::to_string;
using std::max;
using std::min;

constexpr unsigned long long int configuration_counts[2][17] {
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
	19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
	144799200ULL, 30925440ULL, 11571840ULL, 2932160ULL,
	666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

const string database_prefixes[2] = {"white_table", "black_table"};

int main(int argc, char *argv[]) {
  if(argc != 4 || (string(argv[2]) != "0" && string(argv[2]) != "1")) {
    cerr << "Usage: " << argv[0] << "ITERATION VIEWPOINT CONFIGURATION_ID (0: White; 1: Black)" << endl;
    return 1;
  }
  char* end = nullptr;
  const long iteration_arg = std::strtol(argv[1], &end, 10);
  if(end == argv[1] || *end || iteration_arg < 0 || iteration_arg > 15) {
    cerr << "Iteration must be between 0 and 15." << endl;
    return 1;
  }
  const int iteration = static_cast<int>(iteration_arg);
  const int viewpoint = argv[2][0] - '0';
  errno = 0;
  const auto configuration_id = std::strtoull(argv[3], &end, 10);
  if(argv[3][0] < '0' || argv[3][0] > '9' || *end || errno == ERANGE ||
     configuration_id >= configuration_counts[viewpoint][16 - iteration]) {
    cerr << "Configuration ID is outside the selected table." << endl;
    return 1;
  }
  string database_prefix = database_prefixes[viewpoint];
  string opponent_database_prefix = database_prefixes[1 - viewpoint];
  string parent_database_path = "data/db/" + database_prefix + "_" + to_string(iteration) + ".bin";
  string child_database_path = "data/db/" + database_prefix + "_" + to_string(iteration + 1) + ".bin";
  string opponent_child_database_path = "data/db/" + opponent_database_prefix + "_" + to_string(iteration + 1) + ".bin";
  unique_ptr<ZDD> parent_zdd = make_unique<ZDD>(viewpoint, iteration);
  unique_ptr<ZDD> child_zdd;
  unique_ptr<ZDD> opponent_child_zdd;
  if(iteration == 15) {
    child_zdd = make_unique<ZDD>(viewpoint, iteration);
    opponent_child_zdd = make_unique<ZDD>(1 - viewpoint, iteration);
  } else {
    child_zdd = make_unique<ZDD>(viewpoint, iteration + 1);
    opponent_child_zdd = make_unique<ZDD>(1 - viewpoint, iteration + 1);
  }
  
  size_t bits_per_entry, child_bits_per_entry;
  if(iteration > 12) bits_per_entry = 4, child_bits_per_entry = 4;
  else if(iteration > 11) bits_per_entry = 8, child_bits_per_entry = 4;
  else bits_per_entry = 8, child_bits_per_entry = 8;
  Table parent_table(16 - iteration, parent_database_path.c_str(), bits_per_entry, configuration_counts[viewpoint][16 - iteration]);
  Table child_table(15 - iteration, child_database_path.c_str(), child_bits_per_entry, configuration_counts[viewpoint][15 - iteration]);
  Table opponent_child_table(15 - iteration, opponent_child_database_path.c_str(), child_bits_per_entry, configuration_counts[1 - viewpoint][15 - iteration]);

  if(parent_table.get_value(configuration_id) == v_win) {
    std::cout << "this configuration is win" << endl;
  }

  Posi position;

  assert(configuration_id < configuration_counts[viewpoint][16 - iteration]);
  position.make_posi(configuration_id, *parent_zdd);
  std::cout << "ID of this configuration: " << configuration_id << endl;
  position.print();

  Action actions[1000] = {};
  unsigned char compatible_boards[12][46] = {};
  int action_count_or_result = position.compute_actions(actions, viewpoint, iteration);
  int belief_state_size = position.getunknowninfo(compatible_boards, viewpoint, iteration);

  int no_loss_found = 0;

  int win_length, loss_length;
  loss_length = -1;
  if(iteration % 2 == viewpoint) win_length = 99;
  else win_length = -1;
  
  if(action_count_or_result == -1) {
    win_length = 0;
    std::cout << "already win" << endl;
  } else if(action_count_or_result == -2) {
    loss_length = 0;
    std::cout << "already loss" << endl;
    std::cout << loss_length << endl;
  }
  
  int classification_result = 0;
  int classification_decided = 0;
  int action_count = action_count_or_result;

  // Evaluate each action across all compatible configurations.
  if(action_count_or_result > 0) {
    for(int child_view_index = 0; child_view_index < 2; child_view_index++) {
      int winning_action_count = 0;
      int losing_action_count = 0;
      int no_loss_action_count = 0;
      int no_win_action_count = 0;
      int drawing_action_count = 0;
      int unknown_action_count = 0;
      for(int action_index = 0; action_index < action_count; action_index++) {
	int winning_child_count = 0;
	int losing_child_count = 0;
	int no_loss_child_count = 0;
	int no_win_child_count = 0;
	int drawing_child_count = 0;
	int action_win_length = -1;
	unsigned long long int child_configuration_id = 0;
	bool has_child_configuration = false;
	for(int compatible_index = 0; compatible_index < belief_state_size; compatible_index++) {
	  position.make_posi_n(compatible_boards, compatible_index);
	  int action_result = position.make_action(actions[action_index], viewpoint, iteration);
	  if(action_result == 0) {
	    has_child_configuration = true;
	    if(child_view_index == 0) {
	      position.make_posi_myself();
	      child_configuration_id = position.getzddnum(*child_zdd);
	      int child_label = child_table.get_value(child_configuration_id);
	      assert(child_label >= 0 && child_label <= 5);
	      if(child_label == v_win) {
		winning_child_count++;
		action_win_length = max((int)child_table.get_game_length(child_configuration_id) + 1, action_win_length);
	      } else if(child_label == v_lose) {
		losing_child_count++;
	      } else if(child_label == v_notlose) {
		no_loss_child_count++;
	      } else if(child_label == v_notwin) {
		no_win_child_count++;
	      } else if(child_label == v_draw) {
		drawing_child_count++;
	      }
	    } else {
	      position.make_posi_opponent();
	      child_configuration_id = position.getzddnum(*opponent_child_zdd);
	      int child_label = opponent_child_table.get_value(child_configuration_id);
	      assert(child_label >= 0 && child_label <= 5);
	      if(child_label == v_win) {
		losing_child_count++;
	      } else if(child_label == v_lose) {
		winning_child_count++;
		action_win_length = max((int)opponent_child_table.get_game_length(child_configuration_id) + 1, action_win_length);
	      } else if(child_label == v_notlose) {
		no_win_child_count++;
	      } else if(child_label == v_notwin) {
		no_loss_child_count++;
	      } else if(child_label == v_draw) {
		drawing_child_count++;
	      }
	    }
	  } else {
	    if(action_result == 3) {
	      losing_child_count++;
	    } else if(action_result == 2) {
	      winning_child_count++;
	      action_win_length = max(1, action_win_length);
	    } else if(action_result == 10) {
	      drawing_child_count++;
	    }
	  }
	}
	
	if(winning_child_count == belief_state_size) {
	  std::cout << "child number " << action_index << " is win(table" << child_view_index << ")" << endl;
	  std::cout << "Action is (" << actions[action_index].po << ", " << actions[action_index].loc1 << ", " << actions[action_index].loc2 << ", " << actions[action_index].loc3 << ")" << endl;
	  if(has_child_configuration) std::cout << "after id is " << child_configuration_id << endl;
	  std::cout << "game_length = " << action_win_length << endl;
	  std::cout << endl;
	  if(iteration % 2 == viewpoint) {
	    if(classification_decided != 1) {
	      classification_result = -4;
	    }
	    classification_decided = 1;
	    win_length = min(action_win_length, win_length);
	  } else {
	    winning_action_count++;
	    win_length = max(action_win_length, win_length);
	  }
	} else if(winning_child_count + no_loss_child_count + drawing_child_count == belief_state_size) {
	  if(drawing_child_count == belief_state_size) {
	    drawing_action_count++;
	  } else {
	    no_loss_action_count++;
	  }
	} else if(losing_child_count == belief_state_size) {
	  losing_action_count++;
	} else if(losing_child_count + no_win_child_count + drawing_child_count == belief_state_size) {
	  no_win_action_count++;
	} else {
	  unknown_action_count++;
	}
	if(action_index == action_count - 1 && iteration % 2 == viewpoint) {
	  if(no_loss_action_count > 0 || drawing_action_count > 0) {
	    if(classification_decided != 1) no_loss_found = 1;
	    classification_decided = 1;
	  }
	}
      }
      
      if(classification_decided == 0) {
	if(winning_action_count == action_count) {
	  classification_result = -4;
	} else if(no_loss_action_count + winning_action_count + drawing_action_count == action_count) {
	  no_loss_found = 1;
	}
      }      
      if(classification_result < 0) {
	action_count_or_result = classification_result;
	break;
      } else if(no_loss_found == 1) {
	break;
      }
    }
  }
  if(action_count_or_result == -1 || action_count_or_result == -4) {
    std::cout << "result is win" << endl;
    std::cout << "win game length is " << win_length << endl;
  }

  return 0;
}
