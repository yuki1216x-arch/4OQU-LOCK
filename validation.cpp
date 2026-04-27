#include <memory>
#include <iostream>
#include <vector>
#include <exception>
#include <cassert>
#include "node.hpp"
#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::vector;

constexpr int MAX_DEPTH = 168; // 168

const string base[2] = {"white_table", "black_table"};

constexpr unsigned long long int placement_count[2][17] {
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL}
};

void search(Node_base& cur, vector<char>& array_objid, const ZDD& zdd, int depth, unsigned long long int& leaf_id, int nmove, int vision) {
  if(depth == MAX_DEPTH) {
    assert(array_objid.size() == 32);
    unsigned char array_objid_zdd[32];

    zdd.compute_array(leaf_id, array_objid_zdd);
    
    for(size_t i = 0; i < array_objid.size(); i++) {
      if(array_objid[i] != array_objid_zdd[i]) {
	std::cerr << "zdd error, id = " << leaf_id << ", locid = " << i << endl;
	std::cerr << "dfp = " << +array_objid[i] << ", zdd = " << +array_objid_zdd[i] << endl;
	std::terminate();
      }
    }

    unsigned long long int array_id = zdd.compute_id(array_objid);
    if(leaf_id != array_id ) {
      std::cerr << "id error, id = " << leaf_id << endl;
      std::terminate();
    }

    if(leaf_id % 100000000 == 0) {
      cout << "ok! id = " << leaf_id << ": ";
      for(char x : array_objid) {
	cout << +x;
      }
      cout << endl;
    }
    
    leaf_id++;
    return;
  }

  for(int x = 0; x < 2; x++) { // for each branch (0 and 1)
    if(cur.IsNextLeaf0(depth, x, nmove)) { // if the x-branch leads to the 0-terminal node
      /*cout << depth << ", " << x << "false: ";
      for(size_t i = 0; i < array_objid.size(); i++) {
	cout << array_objid[i];
      }
      cout << endl;*/
      continue;
    } else { // if current node has children
      std::unique_ptr<Node_base> c;
      if(vision == 0) c = std::make_unique<Node_White>(cur, x);
      else c = std::make_unique<Node_Black>(cur, x);

      if(x == 0) {
	search(*c, array_objid, zdd, depth + 1, leaf_id, nmove, vision); // left
      } else {
	array_objid.push_back(cur.get_loc_stateid());
	search(*c, array_objid, zdd, depth + 1, leaf_id, nmove, vision); // right
	array_objid.pop_back();
      }
    }
  }
  return;
}

bool detect_invalid_transition_player(int parent_val, int child_val[1000][12], int nchild, int num_of_un, int iteration, int vision) {
  assert(parent_val == v_win || parent_val == v_draw || parent_val == v_notlose);

  assert(num_of_un > 0 && num_of_un <= 12);
  assert(nchild > 0 && nchild < 1000);
  
  if(parent_val == v_win) { // win
    if(iteration % 2 == vision) {
      for(int j = 0; j < nchild; j++) {
	int num_win = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_win) num_win++;
	}
	if(num_win == num_of_un) return false;
      }
      return true;
    } else {
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] != v_win) return true;
	}
      }
      return false;
    }
  } else { // not lose or draw
    assert(parent_val == v_notlose || parent_val == v_draw);
    if(iteration % 2 == vision) {
      for(int j = 0; j < nchild; j++) {
	int num_win = 0, num_notlose = 0;
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] == v_win) num_win++, num_notlose++;
	  else if(child_val[j][m] == v_notlose || child_val[j][m] == v_draw) num_notlose++;
	}
	if(num_win != num_of_un && num_notlose == num_of_un) return false;
      }
      return true;
    } else {
      int num_win = 0;
      for(int j = 0; j < nchild; j++) {
	for(int m = 0; m < num_of_un; m++) {
	  if(child_val[j][m] != v_win && child_val[j][m] != v_notlose && child_val[j][m] != v_draw) {
	    return true;
	  }
	  if(child_val[j][m] == v_win) num_win++;
	}
      }
      if(num_win == nchild * num_of_un) {
	return true;
      }
      return false;
    }
  }
}

bool detect_invalid_transition_opponent(int parent_val, int child_val[12][1000][12], int num_of_un, int nchild_opp, int num_of_un_opp, int iteration, int vision) {
  assert(parent_val == v_lose || parent_val == v_draw || parent_val == v_notwin);

  assert(num_of_un > 0 && num_of_un <= 12);
  assert(nchild_opp > 0 && nchild_opp < 1000);
  assert(num_of_un_opp > 0 && num_of_un_opp <= 12);
  
  if(parent_val == v_lose) { // lose
    int num_lose = 0;
    for(int j = 0; j < num_of_un; j++) {
      bool is_lose = true;
      for(int k = 0; k < nchild_opp; k++) {
	for(int m = 0; m < num_of_un_opp; m++) {
	  if(child_val[j][k][m] != v_lose){
	    is_lose = false;
	    break;
	  }
	}
	if(iteration % 2 != vision) {
	  if(is_lose) {
	    num_lose++;
	    break;
	  }
	  is_lose = true;
	} else {
	  if(!is_lose) return true;
	}
      }
    }
    if(iteration % 2 != vision) {
      if(num_lose == num_of_un) return false;
      else return true;
    }
    return false;
  } else { // not win or draw
    assert(parent_val == v_notwin || parent_val == v_draw);
    int num_notwin = 0;
    for(int j = 0; j < num_of_un; j++) {
      int count_notwin = 0, count_lose = 0;
      for(int k = 0; k < nchild_opp; k++) {
	bool is_notwin = true, is_lose = true;
	for(int m = 0; m < num_of_un_opp; m++) {
	  int x = child_val[j][k][m];
	  if(x != v_lose) {
	    is_lose = false;
	    if(x != v_draw && x != v_notwin) {
	      is_notwin = false;
	      break;
	    }
	  }
	}
	if(is_lose) count_lose++;
	if(is_notwin) count_notwin++;
      }
      if(iteration % 2 != vision) {
	if(count_notwin > 0 && count_lose == 0) num_notwin++;
      } else {
	if(count_notwin == nchild_opp && count_lose != nchild_opp) num_notwin++;
      }
    }
    if(num_notwin == num_of_un) return false;
    return true;
  }
}

int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]); // number of moves
  int vision = atoi(argv[2]); // player perspective (0: white, 1: black)
  string base_filename = base[vision];
  string base_filename_opp = base[1-vision];
  string read_filename_str = base_filename + "_" + std::to_string(iteration) + ".bin";
  string read_filename_str_child = base_filename + "_" + std::to_string(iteration+1) + ".bin";
  string read_filename_str_child_opp = base_filename_opp + "_" + std::to_string(iteration+1) + ".bin";
  const char* read_filename_c_str = read_filename_str.c_str();
  const char* read_filename_c_str_child = read_filename_str_child.c_str();
  const char* read_filename_c_str_child_opp = read_filename_str_child_opp.c_str();
  unsigned long long int parent_table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL; // parent table size
  unsigned long long int child_table_size64 = (placement_count[vision][15 - iteration] + 15ULL) / 16ULL; // child table size
  unsigned long long int opp_child_table_size64 = (placement_count[1 - vision][15 - iteration] + 15ULL) / 16ULL; // child table size (opponent's perspective)
  unsigned long long int id = 0;
  
  unique_ptr<ZDD> zdd_parent = make_unique<ZDD>(vision, iteration);
  unique_ptr<ZDD> zdd_child, zdd_child_opp;
  if(iteration < 15) {
    zdd_child = make_unique<ZDD>(vision, iteration + 1);
    zdd_child_opp = make_unique<ZDD>(1 - vision, iteration + 1);
  } else {
    zdd_child = make_unique<ZDD>(vision, iteration);
    zdd_child_opp = make_unique<ZDD>(1 - vision, iteration);
  }
  
  {
    unique_ptr<Node_base> root;
    if(vision == 0) root = make_unique<Node_White>();
    else root = make_unique<Node_Black>();
  
    vector<char> array_objid;
  
    search(*root, array_objid, *zdd_parent, 0, id, iteration, vision);
  }

  cout << "leaf num: " << id << endl;

  if(vision == 0) cout << "zdd is ok. (iteration : " << iteration << ", vision : white)" << endl;
  else cout << "zdd is ok. (iteration : " << iteration << ", vision: black)" << endl;

  return 0;

  // from here, verify parent-child relationships
  Table parent_table(16 - iteration, read_filename_c_str, "db", parent_table_size64, placement_count[vision][16 - iteration]); // parent talbe
  Table child_table(15 - iteration, read_filename_c_str_child, "db", child_table_size64, placement_count[vision][15 - iteration]); // child table
  Table opp_child_table(15 - iteration, read_filename_c_str_child_opp, "db", opp_child_table_size64, placement_count[1 - vision][15 - iteration]); // child table (opponent's perspective)
  Posi p;
  bool is_error = false;

  for(unsigned long long int i = 0ULL; i < placement_count[vision][16 - iteration]; i++) {
    p.make_posi(i, *zdd_parent);
    Action actions[1000] = {};
    unsigned char board_belief[12][46] = {};
    int nchild = p.compute_actions(actions, vision, iteration);
    int num_of_un = p.getunknowninfo(board_belief, vision, iteration);
    int nchild_opp = 0, num_of_un_opp = 0;
    
    int parent_val = parent_table.get(i);
    if(parent_val <= 0 || parent_val > 5) std::cout << "parent = " << parent_val << endl;
    assert(parent_val > 0 && parent_val <= 5);

    if(nchild > 0) {
      if(parent_val == v_win || parent_val == v_notlose || parent_val == v_draw) {
	int child_val[nchild][12] = {};
	for(int j = 0; j < nchild; j++) {
	  for(int m = 0; m < num_of_un; m++) {
	    p.make_posi_n(board_belief, m);
	    int board_check = p.make_action(actions[j], vision, iteration);
	    if(board_check == 0) { // Non-terminal node
	      p.make_posi_myself();
	      child_val[j][m] = child_table.get(p.getzddnum(*zdd_child)); // retrieve the value from the child's table
	    } else if(board_check == 2) child_val[j][m] = v_win; // win (player's perspective)
	    else if(board_check == 3) child_val[j][m] = v_lose; // lose (player's perspective)
	    else {
	      assert(board_check == 10); // draw
	      child_val[j][m] = v_draw;
	    }
	  }
	  p.make_posi(i, *zdd_parent); //undo
	}
	if(detect_invalid_transition_player(parent_val, child_val, nchild, num_of_un, iteration, vision)) {
	  is_error = true;
	  std::cout << "label error1, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  for(int j = 0; j < nchild; j++) {
	    for(int m = 0; m < num_of_un; m++) {
	      std::cout << child_val[j][m];
	    }
	    if(j != nchild - 1) std::cerr << " ";
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd_parent);
	  p.print();
	  break;
	}
      }
      if(parent_val == v_lose || parent_val == v_notwin || parent_val == v_draw) {
	int child_val_opp[num_of_un][1000][12] = {};
	for(int j = 0; j < num_of_un; j++) {
	  Action actions_opp[1000] = {};
	  unsigned char board_belief_opp[12][46] = {};
	  p.make_posi_n(board_belief, j);
	  p.make_posi_opponent();
	  nchild_opp = p.compute_actions(actions_opp, 1 - vision, iteration);
	  num_of_un_opp = p.getunknowninfo(board_belief_opp, 1 - vision, iteration);
	  assert(nchild_opp < 1000 && num_of_un_opp <= 12);
	  for(int k = 0; k < nchild_opp; k++) {
	    for(int m = 0; m < num_of_un_opp; m++) {
	      p.make_posi_n(board_belief_opp, m);
	      int board_check = p.make_action(actions_opp[k], 1 - vision, iteration);
	      if(board_check == 0) { // Non-terminal node
		p.make_posi_opponent();
		child_val_opp[j][k][m] = opp_child_table.get(p.getzddnum(*zdd_child_opp)); // retrieve the value from the child's table
	      } else if(board_check == 2) child_val_opp[j][k][m] = v_lose; // lose(opponent's perspective)
	      else if(board_check == 3) child_val_opp[j][k][m] = v_win; // win(opponent's perspective)
	      else {
		assert(board_check == 10); // draw
		child_val_opp[j][k][m] = v_draw;
	      }
	    }
	    p.make_posi(i, *zdd_parent);
	  }
	}
	if(detect_invalid_transition_opponent(parent_val, child_val_opp, num_of_un, nchild_opp, num_of_un_opp, iteration, vision)) {
	  is_error = true;
	  std::cout << "label error2, id = " << i << ", parent = " << parent_val << endl;
	  std::cout << "child = ";
	  for(int j = 0; j < num_of_un; j++) {
	    for(int k = 0; k < nchild_opp; k++) {
	      for(int m = 0; m < num_of_un_opp; m++) {
		std::cout << child_val_opp[j][k][m];
	      }
	      if(k != nchild_opp - 1) std::cerr << " ";	      
	    }
	    if(j != num_of_un - 1) std::cerr << ": ";
	  }
	  std::cout << endl;
	  p.make_posi(i, *zdd_parent);
	  p.print();
	  break;
	}
      }
    } else if(nchild == -1) {
      if(parent_val != 1) {
	is_error = true;
	std::cout << "terminal label error1, id = " << i << ", parent = " << parent_val << ", nchild = "<< nchild << endl;
	p.make_posi(i, *zdd_parent);
	p.print();
	std::cout << "vision = " << vision << ", iteration = " << iteration << endl;
	break;
      }
    } else if(nchild == -2) {
      if(parent_val != 2) {
	is_error = true;
	std::cout << "terminal label error2, id = " << i << ", parent = " << parent_val << ", nchild = " << nchild << endl;
	p.make_posi(i, *zdd_parent);
	p.print();
	break;
      }
    } else {
      is_error = true;
      std::cout << "value error: exception, id =" << i << endl;
      std::cout << "nchild = " << nchild << ", parent = " << parent_val << endl;
      p.make_posi(i, *zdd_parent);
      p.print();
      break;
    }
    if(i % 100000000 == 0) std::cout << "label ok! id = " << i << endl;
  }
  if(is_error) std::terminate();
  return 0;
}
