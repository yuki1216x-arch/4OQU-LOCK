#include <memory>
#include <chrono>
#include <random>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <thread>
#include <map>
#include <stdint.h>
#include <climits>
#include <exception>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::mt19937_64;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::to_string;
using std::max;
using std::min;

using namespace std;

constexpr int worker_count = 7;
constexpr int input_queue_capacity = 1024;
constexpr int output_queue_capacity = 256;
constexpr int max_legal_action_count = 1000;
constexpr int max_belief_state_size = 12;

condition_variable coordinator_condition;
condition_variable worker_condition;
mutex queue_mutex;
mutex zdd_mutex;

bool workers_should_stop = false;   // Protected by queue_mutex.

// Transfer a configuration and its child IDs between workers and the coodinator.
// Negative action counts encode results: -1/-4 win, -2/-5 loss,
// -6 no-loss, -7 no-win, and -8 draw. Child IDs use -2 win, -3 loss, -10 draw
// from the viewpoint used by make_action(), before any perspective inversion.
class WorkItem {
  unsigned long long int m_configuration_id;
  int m_action_count_or_result;
  int m_belief_state_size;
  int m_opponent_action_counts[2][max_belief_state_size];
  int m_opponent_belief_state_sizes[2][max_belief_state_size];
  long long int m_child_configuration_ids[2][max_legal_action_count][max_belief_state_size];
  long long int m_opponent_child_configuration_ids[2][max_belief_state_size][max_legal_action_count][max_belief_state_size];
public:
  WorkItem() noexcept {}
  WorkItem(unsigned long long int configuration_id) noexcept : m_configuration_id(configuration_id) {}
  void set_id(unsigned long long int configuration_id) noexcept { m_configuration_id = configuration_id; }
  void set(int action_count_or_result, int belief_state_size) noexcept {
    m_action_count_or_result = action_count_or_result;
    m_belief_state_size = belief_state_size;
    assert(action_count_or_result < max_legal_action_count);
    assert(belief_state_size <= max_belief_state_size);
  }
  void set(int opponent_action_counts[][max_belief_state_size], int opponent_belief_state_sizes[][max_belief_state_size],
	   long long int child_configuration_ids[][max_legal_action_count][max_belief_state_size],
	   long long int opponent_child_configuration_ids[][max_belief_state_size][max_legal_action_count][max_belief_state_size]) noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_action_count_or_result; j++) {
	for(int k = 0; k < m_belief_state_size; k++) {
	  m_child_configuration_ids[i][j][k] = child_configuration_ids[i][j][k];
	}
      }
      for(int j = 0; j < m_belief_state_size; j++) {
        m_opponent_action_counts[i][j] = opponent_action_counts[i][j];
	m_opponent_belief_state_sizes[i][j] = opponent_belief_state_sizes[i][j];
	assert(opponent_action_counts[i][j] > 0 && opponent_action_counts[i][j] < max_legal_action_count);
	assert(opponent_belief_state_sizes[i][j] > 0 && opponent_belief_state_sizes[i][j] < max_belief_state_size);
	for(int k = 0; k < opponent_action_counts[i][j]; k++) {
	  for(int l = 0; l < opponent_belief_state_sizes[i][j]; l++) {
	    m_opponent_child_configuration_ids[i][j][k][l] = opponent_child_configuration_ids[i][j][k][l];
	  }
	}
      }
    }
  }
  unsigned long long int get_id() const noexcept { return m_configuration_id; }
  void get_counts(int& action_count_or_result, int& belief_state_size) const noexcept {
    action_count_or_result = m_action_count_or_result;
    belief_state_size = m_belief_state_size;
  }
  void get(long long int child_configuration_ids[][max_legal_action_count][max_belief_state_size]) const noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_action_count_or_result; j++) {
	for(int k = 0; k < m_belief_state_size; k++) {
	  child_configuration_ids[i][j][k] = m_child_configuration_ids[i][j][k];
	}
      }
    }
  }
  void get_opponent_children(int opponent_action_counts[][max_belief_state_size], int opponent_belief_state_sizes[][max_belief_state_size],
	       long long int opponent_child_configuration_ids[][max_belief_state_size][max_legal_action_count][max_belief_state_size]) const noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_belief_state_size; j++) {
	opponent_action_counts[i][j] = m_opponent_action_counts[i][j];
	opponent_belief_state_sizes[i][j] = m_opponent_belief_state_sizes[i][j];
	for(int k = 0; k < m_opponent_action_counts[i][j]; k++) {
	  for(int l = 0; l < m_opponent_belief_state_sizes[i][j]; l++) {
	    opponent_child_configuration_ids[i][j][k][l] = m_opponent_child_configuration_ids[i][j][k][l];
	  }
	}
      }
    }
  }
};

// Indexed by viewpoint (0 white, 1 black), then by remaining placements (16 - iteration).
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

unsigned long long int win_count = 0;
unsigned long long int loss_count = 0;
unsigned long long int draw_count = 0;
unsigned long long int unknown_count = 0;
unsigned long long int last_unknown_id = 0;
unsigned long long int no_win_count = 0;
unsigned long long int no_loss_count = 0;

// Diagnostic counters distinguish the parent turn and the child database viewpoint.
// Self/opponent refer to the parent viewpoint player.
unsigned long long int self_turn_self_view_win_count = 0;   // (mm_me_win)
unsigned long long int self_turn_opponent_view_win_count = 0;   // (mm_ee_win)
unsigned long long int opponent_turn_self_view_win_count = 0;   // (me_mm_win)
unsigned long long int opponent_turn_opponent_view_win_count = 0;   // (me_em_win)
unsigned long long int self_turn_self_view_loss_count = 0;   // (mm_me_loss)
unsigned long long int self_turn_opponent_view_loss_count = 0;   // (mm_ee_loss)
unsigned long long int opponent_turn_self_view_loss_count = 0;   // (me_mm_loss)
unsigned long long int opponent_turn_opponent_view_loss_count = 0;   // (me_em_loss)
unsigned long long int self_turn_self_view_no_win_count = 0;   // (mm_me_no_win)
unsigned long long int self_turn_opponent_view_no_win_count = 0;   // (mm_ee_no_win)
unsigned long long int opponent_turn_self_view_no_win_count = 0;   // (me_mm_no_win)
unsigned long long int opponent_turn_opponent_view_no_win_count = 0;   // (me_em_no_win)
unsigned long long int self_turn_self_view_no_loss_count = 0;   // (mm_me_no_loss)
unsigned long long int self_turn_opponent_view_no_loss_count = 0;   // (mm_ee_no_loss)
unsigned long long int opponent_turn_self_view_no_loss_count = 0;   // (me_mm_no_loss)
unsigned long long int opponent_turn_opponent_view_no_loss_count = 0;   // (me_em_no_loss)
unsigned long long int confirmed_draw_count = 0;
unsigned long long int win_length_counts[17] = {}, loss_length_counts[17] = {};

unsigned long long int terminal_win_count = 0;
unsigned long long int terminal_loss_count = 0;
unsigned long long int terminal_unknown_count = 0;
unsigned long long int singleton_belief_count = 0;
unsigned long long int multiple_configuration_belief_count = 0;

deque<WorkItem *> input_queue;
deque<WorkItem *> output_queue;

// Schedule work, classify returned children, and write the parent database.
static void run_coordinator(int iteration, int viewpoint, const char* parent_database_path,
			   const char* self_view_child_database_path, const char* opponent_view_child_database_path) noexcept {
  cout << "[analysis] Coordinator started." << endl;

  // Preserve the existing label and game-length storage widths for each layer.
  size_t bits_per_entry, child_bits_per_entry;
  if(iteration > 12) bits_per_entry = 4, child_bits_per_entry = 4;
  else if(iteration > 11) bits_per_entry = 8, child_bits_per_entry = 4;
  else bits_per_entry = 8, child_bits_per_entry = 8;
  Table parent_table(16 - iteration, parent_database_path, bits_per_entry, configuration_counts[viewpoint][16 - iteration]);
  Table self_view_child_table(15 -iteration, self_view_child_database_path, child_bits_per_entry, configuration_counts[viewpoint][15 - iteration]);
  Table opponent_view_child_table(15 - iteration, opponent_view_child_database_path, child_bits_per_entry, configuration_counts[1 - viewpoint][15 - iteration]);

  unsigned long long int configuration_count = configuration_counts[viewpoint][16 - iteration];
  unsigned long long int submitted_work_count = 0ULL;
  unsigned long long int completed_work_count = 0ULL;

  int idle_work_count = input_queue_capacity + output_queue_capacity + worker_count;
  WorkItem* idle_work_items[idle_work_count];
  for(int i = 0; i < idle_work_count; i++) idle_work_items[i] = new WorkItem;

  cout << "[analysis] Scheduling configurations." << endl;

  while(true) {
    unique_lock<mutex> queue_lock(queue_mutex);
    coordinator_condition.wait(queue_lock, [&](){
			return (((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && (submitted_work_count < configuration_count))
				|| (0 < output_queue.size())); });

    if((input_queue.size() < input_queue_capacity) && (output_queue.size() < output_queue_capacity) && submitted_work_count < configuration_count) {
      assert(idle_work_count >= 1);
      WorkItem *work_item = idle_work_items[ --idle_work_count ];
      work_item->set_id(submitted_work_count);
      input_queue.push_front(work_item);
      queue_lock.unlock();
      submitted_work_count++;

      if(submitted_work_count % 100000000ULL == 0ULL) {
	cout << "configuration_id: " << submitted_work_count << ", win_count: " << win_count << ", loss_count: " << loss_count << ", draw_count: " << draw_count
	     << ", no_win_count: " << no_win_count << ", no_loss_count: " << no_loss_count << ", unknown_count: " << unknown_count
	     << ", singleton_belief_count: " << singleton_belief_count << ", multiple_configuration_belief_count: " << multiple_configuration_belief_count << endl;
      }

      if(submitted_work_count % 10000000000ULL == 0ULL) {
	cout << "win positions by shortest win length:" << endl;
	cout << "0 moves: " << std::setw(12) << win_length_counts[0] << " positions" << "          9 moves: " << std::setw(12) << win_length_counts[9] << " positions" << endl;
	cout << "1 move : " << std::setw(12) << win_length_counts[1] << " positions" << "         10 moves: " << std::setw(12) << win_length_counts[10] << " positions" << endl;
	cout << "2 moves: " << std::setw(12) << win_length_counts[2] << " positions" << "         11 moves: " << std::setw(12) << win_length_counts[11] << " positions" << endl;
	cout << "3 moves: " << std::setw(12) << win_length_counts[3] << " positions" << "         12 moves: " << std::setw(12) << win_length_counts[12] << " positions" << endl;
	cout << "4 moves: " << std::setw(12) << win_length_counts[4] << " positions" << "         13 moves: " << std::setw(12) << win_length_counts[13] << " positions" << endl;
	cout << "5 moves: " << std::setw(12) << win_length_counts[5] << " positions" << "         14 moves: " << std::setw(12) << win_length_counts[14] << " positions" << endl;
	cout << "6 moves: " << std::setw(12) << win_length_counts[6] << " positions" << "         15 moves: " << std::setw(12) << win_length_counts[15] << " positions" << endl;
	cout << "7 moves: " << std::setw(12) << win_length_counts[7] << " positions" << "         16 moves: " << std::setw(12) << win_length_counts[16] << " positions" << endl;
	cout << "8 moves: " << std::setw(12) << win_length_counts[8] << " positions" << endl;
      
	cout << endl;
	cout << "lose positions by longest lose length:" << endl;
	cout << "0 moves: " << std::setw(12) << loss_length_counts[0] << " positions" << "          9 moves: " << std::setw(12) << loss_length_counts[9] << " positions" << endl;
	cout << "1 move : " << std::setw(12) << loss_length_counts[1] << " positions" << "         10 moves: " << std::setw(12) << loss_length_counts[10] << " positions" << endl;
	cout << "2 moves: " << std::setw(12) << loss_length_counts[2] << " positions" << "         11 moves: " << std::setw(12) << loss_length_counts[11] << " positions" << endl;
	cout << "3 moves: " << std::setw(12) << loss_length_counts[3] << " positions" << "         12 moves: " << std::setw(12) << loss_length_counts[12] << " positions" << endl;
	cout << "4 moves: " << std::setw(12) << loss_length_counts[4] << " positions" << "         13 moves: " << std::setw(12) << loss_length_counts[13] << " positions" << endl;
	cout << "5 moves: " << std::setw(12) << loss_length_counts[5] << " positions" << "         14 moves: " << std::setw(12) << loss_length_counts[14] << " positions" << endl;
	cout << "6 moves: " << std::setw(12) << loss_length_counts[6] << " positions" << "         15 moves: " << std::setw(12) << loss_length_counts[15] << " positions" << endl;
	cout << "7 moves: " << std::setw(12) << loss_length_counts[7] << " positions" << "         16 moves: " << std::setw(12) << loss_length_counts[16] << " positions" << endl;
	cout << "8 moves: " << std::setw(12) << loss_length_counts[8] << " positions" << endl;
      }
      worker_condition.notify_one();
      
    } else {
      if(0 < output_queue.size()) {
	assert(0 < output_queue.size());
	deque<WorkItem *> completed_work_items;
	swap(completed_work_items, output_queue);

	output_queue.clear();
	queue_lock.unlock();
	completed_work_count += completed_work_items.size();
	for(unsigned int work_index = 0; work_index < completed_work_items.size(); work_index++) {
	  unsigned long long int configuration_id = completed_work_items[work_index]->get_id();
	  int action_count_or_result, belief_state_size;
	  completed_work_items[work_index]->get_counts(action_count_or_result, belief_state_size);

	  assert(self_turn_self_view_win_count + self_turn_opponent_view_win_count + opponent_turn_self_view_win_count
		 + opponent_turn_opponent_view_win_count + terminal_win_count == win_count);
	  assert(self_turn_self_view_loss_count + self_turn_opponent_view_loss_count + opponent_turn_self_view_loss_count
		 + opponent_turn_opponent_view_loss_count + terminal_loss_count == loss_count);
	  assert(self_turn_self_view_no_win_count + self_turn_opponent_view_no_win_count
		 + opponent_turn_self_view_no_win_count + opponent_turn_opponent_view_no_win_count == no_win_count);
	  assert(self_turn_self_view_no_loss_count + self_turn_opponent_view_no_loss_count
		 + opponent_turn_self_view_no_loss_count + opponent_turn_opponent_view_no_loss_count == no_loss_count);
	  assert(confirmed_draw_count == draw_count);
    
	  int no_loss_found = 0;
	  int no_loss_source = 0;

	  int win_length, loss_length;
	  loss_length = -1;
	  if(iteration % 2 == viewpoint) win_length = 99;
	  else win_length = -1;

	  assert(belief_state_size > 0);
	  if(belief_state_size == 1) {
	    singleton_belief_count++;
	  } else {
	    multiple_configuration_belief_count++;
	  }
	  if(action_count_or_result == -1) {
	    terminal_win_count++;
	    win_length = 0;
	  } else if(action_count_or_result == -2) {
	    terminal_loss_count++;
	    loss_length = 0;
	  } else {
	    assert(action_count_or_result > 0);
	  }
	
	  int classification_result = 0;
	  int classification_decided = 0;
	  // First classify wins and no-loss results from the parent belief state.
	  if(action_count_or_result > 0) {
	    long long int child_configuration_ids[2][max_legal_action_count][max_belief_state_size];
	    completed_work_items[work_index]->get(child_configuration_ids);
	    
	    int action_count = action_count_or_result;
	    // Child view index: 0 uses the parent viewpoint; 1 uses its opponent.
	    for(int child_view_index = 0; child_view_index < 2; child_view_index++) {
	      int winning_action_count = 0;
	      int losing_action_count = 0;
	      int no_loss_action_count = 0;
	      int no_win_action_count = 0;
	      int drawing_action_count = 0;
	      int unknown_action_count = 0;
	      for(int j = 0; j < action_count; j++) {
		int winning_child_count = 0;
		int losing_child_count = 0;
		int no_loss_child_count = 0;
		int no_win_child_count = 0;
		int drawing_child_count = 0;
		int action_win_length = -1;
		for(int m = 0; m < belief_state_size; m++) {
		  long long int child_configuration_id = child_configuration_ids[child_view_index][j][m];
		  if(child_configuration_id >= 0) {
		    if(child_view_index == 0) {
		      int child_label = self_view_child_table.get_value((unsigned long long int)child_configuration_id);
		      assert(child_label >= 0 && child_label <= 5);
		      if(child_label == v_win) {
			winning_child_count++;
			action_win_length = max((int)self_view_child_table.get_game_length((unsigned long long int)child_configuration_id) + 1, action_win_length);
		      } else if(child_label == v_lose) {
			losing_child_count++;
		      } else if(child_label == v_notlose) {
			no_loss_child_count++;
		      } else if(child_label == v_notwin) {
			no_win_child_count++;
		      } else if(child_label == v_draw) {
			drawing_child_count++;
		      } else {}
		    } else {
		      int child_label = opponent_view_child_table.get_value((unsigned long long int)child_configuration_id);
		      assert(child_label >= 0 && child_label <= 5);
		      if(child_label == v_win) {
			losing_child_count++;
		      } else if(child_label == v_lose) {
			winning_child_count++;
			action_win_length = max((int)opponent_view_child_table.get_game_length((unsigned long long int)child_configuration_id) + 1, action_win_length);
		      } else if(child_label == v_notlose) {
			no_win_child_count++;
		      } else if(child_label == v_notwin) {
			no_loss_child_count++;
		      } else if(child_label == v_draw) {
			drawing_child_count++;
		      }
		    }
		  } else {
		    if(child_configuration_id == -3) {
		      losing_child_count++;
		    } else if(child_configuration_id == -2) {
		      winning_child_count++;
		      action_win_length = max(1, action_win_length);
		    } else if(child_configuration_id == -10) {
		      drawing_child_count++;
		    }
		  }
		}
		
		if(winning_child_count == belief_state_size) {
		  if(iteration % 2 == viewpoint) {
		    if(classification_decided != 1) {
		      classification_result = -4;
		      if(child_view_index == 0) {
			self_turn_self_view_win_count++;
		      } else {
			self_turn_opponent_view_win_count++;
		      }
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
		if(j == action_count - 1 && iteration % 2 == viewpoint) {
		  if(no_loss_action_count > 0 || drawing_action_count > 0) {
		    if(classification_decided != 1) {
		      no_loss_found = 1;
		      if(child_view_index == 0) {
			self_turn_self_view_no_loss_count++;
			no_loss_source = 1;
		      } else {
			self_turn_opponent_view_no_loss_count++;
			no_loss_source = 2;
		      }
		    }
		    classification_decided = 1;
		  }
		}
	      }
	      if(classification_decided == 0) {
		if(winning_action_count == action_count) {
		  classification_result = -4;
		  if(child_view_index == 0) {
		    opponent_turn_self_view_win_count++;
		  } else {
		    opponent_turn_opponent_view_win_count++;
		  }
		} else if(no_loss_action_count + winning_action_count + drawing_action_count == action_count) {
		  if(child_view_index == 0) {
		    opponent_turn_self_view_no_loss_count++;
		    no_loss_source = 3;
		  } else {
		    opponent_turn_opponent_view_no_loss_count++;
		    no_loss_source = 4;
		  }
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
	  
	  // First classify wins and no-loss results from the parent belief state.
	  if(action_count_or_result > 0) {
	    // Classify loss and no-win results through opponent belief states.
	    int opponent_action_counts[2][max_belief_state_size], opponent_belief_state_sizes[2][max_belief_state_size];
	    long long int opponent_child_configuration_ids[2][max_belief_state_size][max_legal_action_count][max_belief_state_size];
	    completed_work_items[work_index]->get_opponent_children(opponent_action_counts, opponent_belief_state_sizes, opponent_child_configuration_ids);
	    // Child view index: 0 uses the parent viewpoint; 1 uses its opponent.
	    for(int child_view_index = 0; child_view_index < 2; child_view_index++) {
	      int opponent_winning_belief_count = 0;
	      int opponent_no_loss_belief_count = 0;
	      for(int j = 0; j < belief_state_size; j++) {
		int opponent_action_count = opponent_action_counts[child_view_index][j];
		int opponent_belief_state_size = opponent_belief_state_sizes[child_view_index][j];
		int opponent_action_classified = 0;
		int opponent_belief_counted = 0;
		int opponent_winning_action_count = 0;
		int opponent_no_loss_action_count = 0;
		int opponent_drawing_action_count = 0;
		int opponent_unknown_action_count = 0;
		int opponent_plan_loss_length;
		if(iteration % 2 != viewpoint) opponent_plan_loss_length = 99;
		else opponent_plan_loss_length = -1;
		for(int k = 0; k < opponent_action_count; k++) {
		  int self_winning_child_count = 0;
		  int self_losing_child_count = 0;
		  int self_drawing_child_count = 0;
		  int self_no_loss_child_count = 0;
		  int self_no_win_child_count = 0;
		  int action_loss_length = -1;
		  opponent_action_classified = 0;
		  for(int m = 0; m < opponent_belief_state_size; m++) {
		    long long int opponent_child_configuration_id = opponent_child_configuration_ids[child_view_index][j][k][m];
		    if(opponent_child_configuration_id >= 0) {
		      if(child_view_index == 0) {
			int opponent_child_label = self_view_child_table.get_value((unsigned long long int)opponent_child_configuration_id);
			assert(opponent_child_label >= 0 && opponent_child_label <= 5);
			if(opponent_child_label == v_win) {
			  self_winning_child_count++;
			} else if(opponent_child_label == v_lose) {
			  self_losing_child_count++;
			  action_loss_length = max((int)self_view_child_table.get_game_length((unsigned long long int)opponent_child_configuration_id) + 1, action_loss_length);
			} else if(opponent_child_label == v_draw) {
			  self_drawing_child_count++;
			} else if(opponent_child_label == v_notlose) {
			  self_no_loss_child_count++;
			} else if(opponent_child_label == v_notwin) {
			  self_no_win_child_count++;
			}
		      } else {
			int opponent_child_label = opponent_view_child_table.get_value((unsigned long long int)opponent_child_configuration_id);
			assert(opponent_child_label >= 0 && opponent_child_label <= 5);
			if(opponent_child_label == v_win) {
			  self_losing_child_count++;
			  action_loss_length = max((int)opponent_view_child_table.get_game_length((unsigned long long int)opponent_child_configuration_id) + 1, action_loss_length);
			} else if(opponent_child_label == v_lose) {
			  self_winning_child_count++;
			} else if(opponent_child_label == v_draw) {
			  self_drawing_child_count++;
			} else if(opponent_child_label == v_notlose) {
			  self_no_win_child_count++;
			} else if(opponent_child_label == v_notwin) {
			  self_no_loss_child_count++;
			}
		      }
		    } else {
		      if(opponent_child_configuration_id == -3) {
			self_winning_child_count++;
		      } else if(opponent_child_configuration_id == -2) {
			self_losing_child_count++;
			action_loss_length = max(1, action_loss_length);
		      } else if(opponent_child_configuration_id == -10) {
			self_drawing_child_count++;
		      }
		    }
		  }
		  if(self_losing_child_count == opponent_belief_state_size) {
		    if(iteration % 2 != viewpoint) {
		      if(opponent_belief_counted != 1) opponent_winning_belief_count++;
		      opponent_action_classified = 1;
		      opponent_belief_counted = 1;
		      opponent_plan_loss_length = min(action_loss_length, opponent_plan_loss_length);
		      loss_length = max(opponent_plan_loss_length, loss_length);
		    } else {
		      opponent_winning_action_count++;
		      opponent_plan_loss_length = max(action_loss_length, opponent_plan_loss_length);
		    }
		  } else if(self_losing_child_count + self_drawing_child_count + self_no_win_child_count == opponent_belief_state_size) {
		    if(self_losing_child_count == 0 && self_no_win_child_count == 0) {
		      opponent_drawing_action_count++;
		    } else {
		      opponent_no_loss_action_count++;
		    }
		  } else {
		    opponent_unknown_action_count++;
		  }
		  if(k == opponent_action_count - 1 && iteration % 2 != viewpoint) {
		    if(opponent_no_loss_action_count > 0 || opponent_drawing_action_count > 0) {
		      if(opponent_belief_counted != 1) opponent_no_loss_belief_count++;
		      opponent_action_classified = 1;
		      opponent_belief_counted = 1;
		    }
		  }
		}
		if(opponent_action_classified == 0) {
		  if(opponent_winning_action_count == opponent_action_count) {
		    opponent_action_classified = 1;
		    opponent_winning_belief_count++;
		    loss_length = max(opponent_plan_loss_length, loss_length);
		  } else if(opponent_winning_action_count + opponent_drawing_action_count + opponent_no_loss_action_count == opponent_action_count) {
		    opponent_action_classified = 1;
		    opponent_no_loss_belief_count++;
		  }
		}
		if(opponent_action_classified == 0) break;
	      }
	      if(opponent_winning_belief_count == belief_state_size) {
		action_count_or_result = -5;
		if(iteration % 2 != viewpoint) {
		  if(child_view_index == 0) {
		    opponent_turn_self_view_loss_count++;
		  } else {
		    opponent_turn_opponent_view_loss_count++;
		  }
		} else {
		  if(child_view_index == 0) {
		    self_turn_self_view_loss_count++;
		  } else {
		    self_turn_opponent_view_loss_count++;
		  }
		}
		break;
	      } else if(opponent_winning_belief_count + opponent_no_loss_belief_count == belief_state_size) {
		if(no_loss_found == 1) {
		  action_count_or_result = -8;
		  confirmed_draw_count++;
		  if(iteration % 2 == viewpoint) {
		    if(no_loss_source == 1) {
		      self_turn_self_view_no_loss_count--;
		    } else if(no_loss_source == 2) {
		      self_turn_opponent_view_no_loss_count--;
		    }
		  } else {
		    if(no_loss_source == 3) {
		      opponent_turn_self_view_no_loss_count--;
		    } else if(no_loss_source == 4) {
		      opponent_turn_opponent_view_no_loss_count--;
		    }
		  }
		  break;
		} else {
		  action_count_or_result = -7;
		  if(iteration % 2 != viewpoint) {
		    if(child_view_index == 0) {
		      opponent_turn_self_view_no_win_count++;
		    } else {
		      opponent_turn_opponent_view_no_win_count++;
		    }
		  } else {
		    if(child_view_index == 0) {
		      self_turn_self_view_no_win_count++;
		    } else {
		      self_turn_opponent_view_no_win_count++;
		    }
		  }
		  break;
		}
	      }
	    } 
	  }
	  if(action_count_or_result > 0 && no_loss_found == 1) {
	    action_count_or_result = -6;
	  }
	  if(action_count_or_result == -1 || action_count_or_result == -4) {
	    assert(win_length >= 0 && win_length <= 16 - iteration);
	    parent_table.set(configuration_id, v_win, win_length);
	    win_count++;
	    win_length_counts[win_length]++;
	  } else if(action_count_or_result == -2 || action_count_or_result == -5) {
	    assert(loss_length >= 0 && loss_length <= 16 - iteration);
	    parent_table.set(configuration_id, v_lose, loss_length);
	    loss_count++;
	    loss_length_counts[loss_length]++;
	  } else if(action_count_or_result == -8) {
	    parent_table.set(configuration_id, v_draw, 0ULL);
	    draw_count++;
	  } else if(action_count_or_result == -6) {
	    parent_table.set(configuration_id, v_notlose, 0ULL);
	    no_loss_count++;
	  } else if(action_count_or_result == -7) {
	    parent_table.set(configuration_id, v_notwin, 0ULL);
	    no_win_count++;
	  } else {
	    unknown_count++;
	    parent_table.set(configuration_id, v_unknown, 0ULL);
	  }
	}
	
	// Return completed work items to the reusable pool.
	for(size_t i = 0; i < completed_work_items.size(); i++) {
	  assert(idle_work_count < 2048);
	  idle_work_items[idle_work_count++] = completed_work_items[i];
	}
      }
      if(completed_work_count >= configuration_count) break;
    }
  }

  cout << "[analysis] Writing database: " << parent_database_path << endl;
  {
    OutTable database_writer(16 - iteration, parent_database_path, configuration_count, bits_per_entry);
    for(unsigned long long int i = 0; i < configuration_count; i++) {
      database_writer.write(parent_table.get(i));
    }
    database_writer.flush();
  }

  {
    unique_lock<mutex> queue_lock(queue_mutex);
    workers_should_stop = true;
  }
  worker_condition.notify_all();
}

// Generate compatible configurations and child IDs; the coordinator reads labels.
static void run_worker(int iteration, int viewpoint, const ZDD& parent_zdd, const ZDD& self_view_child_zdd, const ZDD& opponent_view_child_zdd) noexcept {
  WorkItem *work_item;
  while(true) {
    unique_lock<mutex> queue_lock(queue_mutex);

    worker_condition.wait(queue_lock, [&](){ if(0 < input_queue.size()) return true;
	return workers_should_stop; });
    if(workers_should_stop) break;
    assert(0 < input_queue.size());
    work_item = input_queue.back();
    input_queue.pop_back();
    queue_lock.unlock();

    unsigned long long int configuration_id = work_item->get_id();
    Posi position;
    position.make_posi(configuration_id, parent_zdd);
    Action actions[1000] = {};
    unsigned char compatible_boards[12][46] = {};
    int action_count_or_result = position.compute_actions(actions, viewpoint, iteration);
    int belief_state_size = position.getunknowninfo(compatible_boards, viewpoint, iteration);
    assert(action_count_or_result == -2 || action_count_or_result == -1 || (action_count_or_result > 0 && action_count_or_result < max_legal_action_count));
    assert(belief_state_size > 0 && belief_state_size <= max_belief_state_size);

    work_item->set(action_count_or_result, belief_state_size);
    if(action_count_or_result > 0) {
      int opponent_action_counts[2][max_belief_state_size], opponent_belief_state_sizes[2][max_belief_state_size];
      long long int child_configuration_ids[2][max_legal_action_count][max_belief_state_size], opponent_child_configuration_ids[2][max_belief_state_size][max_legal_action_count][max_belief_state_size];

      // Child view index: 0 uses the parent viewpoint; 1 uses its opponent.
      for(int child_view_index = 0; child_view_index < 2; child_view_index++) {
	for(int j = 0; j < action_count_or_result; j++) {
	  for(int m = 0; m < belief_state_size; m++) {
	    position.make_posi_n(compatible_boards, m);
	    int action_result = position.make_action(actions[j], viewpoint, iteration);
	    if(action_result == 0) {
	      if(child_view_index == 0) {
		position.make_posi_myself();
		child_configuration_ids[child_view_index][j][m] = position.getzddnum(self_view_child_zdd);
	      } else {
		position.make_posi_opponent();
		child_configuration_ids[child_view_index][j][m] = position.getzddnum(opponent_view_child_zdd);
	      }
	    } else {
	      if(action_result == 3) child_configuration_ids[child_view_index][j][m] = -3;
	      else if(action_result == 2) child_configuration_ids[child_view_index][j][m] = -2;
	      else {
		assert(action_result == 10);
		child_configuration_ids[child_view_index][j][m] = -10;
	      }
	    }
	  }
	}
      }

      position.make_posi(configuration_id, parent_zdd);
      // Child view index: 0 uses the parent viewpoint; 1 uses its opponent.
      for(int child_view_index = 0; child_view_index < 2; child_view_index++) {
	for(int j = 0; j < belief_state_size; j++) {
	  Action opponent_actions[1000] = {};
	  unsigned char opponent_compatible_boards[12][46] = {};
	  position.make_posi_n(compatible_boards, j);
	  position.make_posi_opponent();
	  opponent_action_counts[child_view_index][j] = position.compute_actions(opponent_actions, 1 - viewpoint, iteration);
	  opponent_belief_state_sizes[child_view_index][j] = position.getunknowninfo(opponent_compatible_boards, 1 - viewpoint, iteration);
	  assert(opponent_action_counts[child_view_index][j] > 0 && opponent_action_counts[child_view_index][j] < max_legal_action_count);
	  assert(opponent_belief_state_sizes[child_view_index][j] > 0 && opponent_belief_state_sizes[child_view_index][j] <= max_belief_state_size);
	  for(int k = 0; k < opponent_action_counts[child_view_index][j]; k++) {
	    for(int m = 0; m < opponent_belief_state_sizes[child_view_index][j]; m++) {
	      position.make_posi_n(opponent_compatible_boards, m);
	      int action_result = position.make_action(opponent_actions[k], 1 - viewpoint, iteration);
	      if(action_result == 0) {
		if(child_view_index == 0) {
		  position.make_posi_opponent();
		  opponent_child_configuration_ids[child_view_index][j][k][m] = position.getzddnum(self_view_child_zdd);
		} else {
		  position.make_posi_myself();
		  opponent_child_configuration_ids[child_view_index][j][k][m] = position.getzddnum(opponent_view_child_zdd);
		}
	      } else {
		if(action_result == 3) opponent_child_configuration_ids[child_view_index][j][k][m] = -3;
		else if(action_result == 2) opponent_child_configuration_ids[child_view_index][j][k][m] = -2;
		else {
		  assert(action_result == 10);
		  opponent_child_configuration_ids[child_view_index][j][k][m] = -10;
		}
	      }
	    }
	  }
	}
      }
      work_item->set(opponent_action_counts, opponent_belief_state_sizes, child_configuration_ids, opponent_child_configuration_ids);
    }

    // Publish completed work while holding the shared queue mutex.
    queue_lock.lock();
    output_queue.push_front(work_item);
    queue_lock.unlock();
    coordinator_condition.notify_one();
  }
}

// Arguments retain the existing interface: iteration, then viewpoint (0 white, 1 black).
int main(int argc, char *argv[]) {
  // Iteration is the number of placements already made; even turns are white.
  int iteration = atoi(argv[1]);
  int viewpoint = atoi(argv[2]);   // 0: white; 1: black.
  string self_view_database_prefix = database_prefixes[viewpoint];
  string opponent_view_database_prefix = database_prefixes[1 - viewpoint];
  string parent_database_path = "data/db/" + self_view_database_prefix + "_" + to_string(iteration) + ".bin";
  string self_view_child_database_path = "data/db/" + self_view_database_prefix + "_" + to_string(iteration + 1) + ".bin";
  string opponent_view_child_database_path = "data/db/" + opponent_view_database_prefix + "_" + to_string(iteration + 1) + ".bin";
  unique_ptr<ZDD> parent_zdd = make_unique<ZDD>(viewpoint, iteration);
  unique_ptr<ZDD> self_view_child_zdd;
  unique_ptr<ZDD> opponent_view_child_zdd;
  // At the final layer, child IDs are unused; retain the existing ZDD setup.
  if(iteration == 15) {
    self_view_child_zdd = make_unique<ZDD>(viewpoint, iteration);
    opponent_view_child_zdd = make_unique<ZDD>(1 - viewpoint, iteration);
  } else {
    self_view_child_zdd = make_unique<ZDD>(viewpoint, iteration + 1);
    opponent_view_child_zdd = make_unique<ZDD>(1 - viewpoint, iteration + 1);
  }

  thread coordinator_thread([=]{run_coordinator(iteration, viewpoint, parent_database_path.c_str(), self_view_child_database_path.c_str(), opponent_view_child_database_path.c_str());});

  thread worker_threads[worker_count];
  for(int worker_index = 0; worker_index < worker_count; worker_index++) {
    worker_threads[worker_index] = thread(run_worker, iteration, viewpoint, std::cref(*parent_zdd), std::cref(*self_view_child_zdd), std::cref(*opponent_view_child_zdd));
  }

  // Wait for all database output and worker processing to finish.
  coordinator_thread.join();
  for(int worker_index = 0; worker_index < worker_count; worker_index++) {
    worker_threads[worker_index].join();
  }
  
  cout << "win_count  =  " << win_count << endl;
  cout << "loss_count  =  " << loss_count << endl;
  cout << "draw_count  =  " << draw_count << endl;
  cout << "unknown_count  =  " << unknown_count << endl;
  cout << "no_win_count  =  " << no_win_count << endl;
  cout << "no_loss_count  =  " << no_loss_count << endl;
  cout << "terminal_win_count  =  " << terminal_win_count << endl;
  cout << "terminal_loss_count  =  " << terminal_loss_count << endl;
  cout << "terminal_unknown_count  =  " << terminal_unknown_count << endl;
  cout << "singleton_belief_count  =  " << singleton_belief_count << endl;
  cout << "multiple_configuration_belief_count  =  " << multiple_configuration_belief_count << endl;
  
  cout << "self_turn_self_view_win_count = " << self_turn_self_view_win_count
       << ", self_turn_opponent_view_win_count = " << self_turn_opponent_view_win_count
       << ", opponent_turn_self_view_win_count = " << opponent_turn_self_view_win_count
       << ", opponent_turn_opponent_view_win_count = " << opponent_turn_opponent_view_win_count << endl;
  cout << "self_turn_self_view_loss_count = " << self_turn_self_view_loss_count
       << ", self_turn_opponent_view_loss_count = " << self_turn_opponent_view_loss_count
       << ", opponent_turn_self_view_loss_count = " << opponent_turn_self_view_loss_count
       << ", opponent_turn_opponent_view_loss_count = " << opponent_turn_opponent_view_loss_count << endl;
  cout << "self_turn_self_view_no_win_count = " << self_turn_self_view_no_win_count
       << ", self_turn_opponent_view_no_win_count = " << self_turn_opponent_view_no_win_count
       << ", opponent_turn_self_view_no_win_count = " << opponent_turn_self_view_no_win_count
       << ", opponent_turn_opponent_view_no_win_count = " << opponent_turn_opponent_view_no_win_count << endl;
  cout << "self_turn_self_view_no_loss_count = " << self_turn_self_view_no_loss_count
       << ", self_turn_opponent_view_no_loss_count = " << self_turn_opponent_view_no_loss_count
       << ", opponent_turn_self_view_no_loss_count = " << opponent_turn_self_view_no_loss_count
       << ", opponent_turn_opponent_view_no_loss_count = " << opponent_turn_opponent_view_no_loss_count << endl;
  cout << "confirmed_draw_count = " << confirmed_draw_count << endl;

  cout << endl;
  cout << "Win positions by shortest win length:" << endl;
  cout << "0 moves: " << std::setw(12) << win_length_counts[0] << " positions" << "          9 moves: " << std::setw(12) << win_length_counts[9] << " positions" << endl;
  cout << "1 move : " << std::setw(12) << win_length_counts[1] << " positions" << "         10 moves: " << std::setw(12) << win_length_counts[10] << " positions" << endl;
  cout << "2 moves: " << std::setw(12) << win_length_counts[2] << " positions" << "         11 moves: " << std::setw(12) << win_length_counts[11] << " positions" << endl;
  cout << "3 moves: " << std::setw(12) << win_length_counts[3] << " positions" << "         12 moves: " << std::setw(12) << win_length_counts[12] << " positions" << endl;
  cout << "4 moves: " << std::setw(12) << win_length_counts[4] << " positions" << "         13 moves: " << std::setw(12) << win_length_counts[13] << " positions" << endl;
  cout << "5 moves: " << std::setw(12) << win_length_counts[5] << " positions" << "         14 moves: " << std::setw(12) << win_length_counts[14] << " positions" << endl;
  cout << "6 moves: " << std::setw(12) << win_length_counts[6] << " positions" << "         15 moves: " << std::setw(12) << win_length_counts[15] << " positions" << endl;
  cout << "7 moves: " << std::setw(12) << win_length_counts[7] << " positions" << "         16 moves: " << std::setw(12) << win_length_counts[16] << " positions" << endl;
  cout << "8 moves: " << std::setw(12) << win_length_counts[8] << " positions" << endl;

  cout << endl;
  cout << "Loss positions by longest lose length:" << endl;
  cout << "0 moves: " << std::setw(12) << loss_length_counts[0] << " positions" << "          9 moves: " << std::setw(12) << loss_length_counts[9] << " positions" << endl;
  cout << "1 move : " << std::setw(12) << loss_length_counts[1] << " positions" << "         10 moves: " << std::setw(12) << loss_length_counts[10] << " positions" << endl;
  cout << "2 moves: " << std::setw(12) << loss_length_counts[2] << " positions" << "         11 moves: " << std::setw(12) << loss_length_counts[11] << " positions" << endl;
  cout << "3 moves: " << std::setw(12) << loss_length_counts[3] << " positions" << "         12 moves: " << std::setw(12) << loss_length_counts[12] << " positions" << endl;
  cout << "4 moves: " << std::setw(12) << loss_length_counts[4] << " positions" << "         13 moves: " << std::setw(12) << loss_length_counts[13] << " positions" << endl;
  cout << "5 moves: " << std::setw(12) << loss_length_counts[5] << " positions" << "         14 moves: " << std::setw(12) << loss_length_counts[14] << " positions" << endl;
  cout << "6 moves: " << std::setw(12) << loss_length_counts[6] << " positions" << "         15 moves: " << std::setw(12) << loss_length_counts[15] << " positions" << endl;
  cout << "7 moves: " << std::setw(12) << loss_length_counts[7] << " positions" << "         16 moves: " << std::setw(12) << loss_length_counts[16] << " positions" << endl;
  cout << "8 moves: " << std::setw(12) << loss_length_counts[8] << " positions" << endl;
  
  return 0;
}
