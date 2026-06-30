#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::to_string;
using std::max;
using std::min;

constexpr unsigned long long int placement_count[2][17] {
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
    144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
    666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
    144799200ULL, 30925440ULL, 11571840ULL, 2932160ULL,
    666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

const string base[2] = {"white_table", "black_table"};

int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]);
  int vision = atoi(argv[2]); //0なら白視点, 1なら黒視点
  unsigned long long int id = atoi(argv[3]);
  string base_filename = base[vision];
  string base_filename_opp = base[1 - vision];
  string read_filename_str = "data/db/" + base_filename + "_" + to_string(iteration) + ".bin";
  string read_filename_str_child = "data/db/" + base_filename + "_" + to_string(iteration+1) + ".bin";
  string read_filename_str_child_opp = "data/db/" + base_filename_opp + "_" + to_string(iteration+1) + ".bin";
  unique_ptr<ZDD> zdd_parent = make_unique<ZDD>(vision, iteration);
  unique_ptr<ZDD> zdd_child;
  unique_ptr<ZDD> zdd_child_opp;
  if(iteration == 15) {
    zdd_child = make_unique<ZDD>(vision, iteration);
    zdd_child_opp = make_unique<ZDD>(1 - vision, iteration);
  } else {
    zdd_child = make_unique<ZDD>(vision, iteration + 1);
    zdd_child_opp = make_unique<ZDD>(1 - vision, iteration + 1);
  }
  
  size_t bits_per_entry, child_bits_per_entry;
  if(iteration > 12) bits_per_entry = 4, child_bits_per_entry = 4;
  else if(iteration > 11) bits_per_entry = 8, child_bits_per_entry = 4;
  else bits_per_entry = 8, child_bits_per_entry = 8;
  Table parent_table(16 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);
  Table child_table(15 -iteration, read_filename_str_child.c_str(), child_bits_per_entry, placement_count[vision][15 - iteration]);
  Table opp_child_table(15 - iteration, read_filename_str_child_opp.c_str(), child_bits_per_entry, placement_count[1 - vision][15 - iteration]);

  if(parent_table.get_value(id) == 1) {
    std::cout << "this configuration is win" << endl;
  }

  Posi p;

  assert(id < placement_count[vision][16 - iteration]);
  p.make_posi(id, *zdd_parent);
  std::cout << "ID of this configuration: " << id << endl;
  p.print();

  Action actions[1000] = {};
  unsigned char board_belief[12][46] = {};
  int nchild = p.compute_actions(actions, vision, iteration);
  int num_of_un = p.getunknowninfo(board_belief, vision, iteration);

  int notlose_flg = 0;

  int win_game_length, lose_game_length;
  lose_game_length = -1;
  if(iteration % 2 == vision) win_game_length = 99;
  else win_game_length = -1;
  
  if(nchild == -1) {
    win_game_length = 0;
    std::cout << "already win" << endl;
  } else if(nchild == -2) {
    lose_game_length = 0;
    std::cout << "already lose" << endl;
    std::cout << lose_game_length << endl;
  } else {
    // assert(nchild > 0);
  }
  
  int game_val = 0;
  int already_decided = 0;
  int num_of_action = nchild;
  
  if(nchild > 0) {
    for(int which_table = 0; which_table < 2; which_table++) {
      int win_plan_num = 0; //必勝戦略の数
      int lose_plan_num = 0; //必敗戦略の数
      int nlose_plan_num = 0; //負無し戦略の数
      int nwin_plan_num = 0; //勝無し戦略の数
      int draw_plan_num = 0; //引分戦略の数
      int unknown_plan_num = 0; //不明戦略の数
      for(int j = 0; j < num_of_action; j++) {
	int kati = 0; //勝ち
	int make = 0; //負け
	int makenasi = 0; //負無し
	int katinasi = 0; //勝無し
	int hikiwake = 0; //引分
	int kati_game_length = -1;
	unsigned long long int num_of_haiti;
	for(int m = 0; m < num_of_un; m++) {
	  p.make_posi_n(board_belief, m); // 第3者視点の配置を作る
	  int board_check = p.make_action(actions[j], vision, iteration);
	  if(board_check == 0) { // まだ終わってない
	    if(which_table == 0) { // 子が自分視点
	      p.make_posi_myself();
	      num_of_haiti = p.getzddnum(*zdd_child); // 子のZDDから子の配置の番号を取得
	      int child_table_val = child_table.get_value(num_of_haiti); //子のtableから値を取得
	      assert(child_table_val >= 0 && child_table_val <= 5);
	      if(child_table_val == 1) {
		//視点プレイヤ勝ち
		kati++;
		kati_game_length = max((int)child_table.get_game_length(num_of_haiti) + 1, kati_game_length);
	      } else if(child_table_val == 2) {
		//視点プレイヤ負け
		make++;
	      } else if(child_table_val == 4) {
		makenasi++;
	      } else if(child_table_val == 5) {
		katinasi++;
	      } else if(child_table_val == 3) {
		hikiwake++;
	      }
	    } else { // 子が相手視点
	      p.make_posi_opponent();
	      num_of_haiti = p.getzddnum(*zdd_child_opp); // 子のZDDから子の配置の番号を取得
	      int child_table_val = opp_child_table.get_value(num_of_haiti); //子のtableから値を取得
	      assert(child_table_val >= 0 && child_table_val <= 5);
	      if(child_table_val == 1) {
		//視点プレイヤ負け
		make++;
	      } else if(child_table_val == 2) {
		//視点プレイヤ勝ち
		kati++;
		kati_game_length = max((int)opp_child_table.get_game_length(num_of_haiti) + 1, kati_game_length);
	      } else if(child_table_val == 4) {
		katinasi++;
	      } else if(child_table_val == 5) {
		makenasi++;
	      } else if(child_table_val == 3) {
		hikiwake++;
	      }
	    }
	  } else {
	    if(board_check == 3) {
	      //視点プレイヤ負け
	      make++;
	    } else if(board_check == 2) {
	      //視点プレイヤ勝ち
	      kati++;
	      kati_game_length = max(1, kati_game_length);
	    } else if(board_check == 10) {
	      //引分
	      hikiwake++;
	    }
	  }
	}
	
	if(kati == num_of_un) { //必勝
	  std::cout << "child number " << j << " is win(table" << which_table << ")" << endl;
	  std::cout << "Action is (" << actions[j].po << ", " << actions[j].loc1 << ", " << actions[j].loc2 << ", " << actions[j].loc3 << ")" << endl;
	  std::cout << "after id is " << num_of_haiti << endl;
	  std::cout << "game_length = " << kati_game_length << endl;
	  std::cout << endl;
	  if(iteration % 2 == vision) { //手番プレイヤが視点プレイヤのとき
	    if(already_decided != 1) {
	      game_val = -4;
	    }
	    already_decided = 1;
	    win_game_length = min(kati_game_length, win_game_length);
	  } else {
	    win_plan_num++;
	    win_game_length = max(kati_game_length, win_game_length);
	  }
	} else if(kati + makenasi + hikiwake == num_of_un) { //負無し
	  if(hikiwake == num_of_un) { //引分
	    draw_plan_num++;
	  } else {
	    nlose_plan_num++;
	  }
	} else if(make == num_of_un) { //必敗
	  lose_plan_num++;                    
	} else if(make + katinasi + hikiwake == num_of_un) { //勝無し
	  nwin_plan_num++;
	} else {
	  unknown_plan_num++;
	}
	if(j == num_of_action - 1 && iteration % 2 == vision) {
	  if(nlose_plan_num > 0 || draw_plan_num > 0) {
	    if(already_decided != 1) notlose_flg = 1;
	    already_decided = 1;
	  }
	}
      }
      //すべての合法手を確認した後
      if(already_decided == 0) {
	if(win_plan_num == num_of_action) {
	  game_val = -4; //必勝を与える
	} else if(nlose_plan_num + win_plan_num + draw_plan_num == num_of_action) {
	  notlose_flg = 1;
	}
      }      
      if(game_val < 0) {
	nchild = game_val;
	break;
      } else if(notlose_flg == 1) {
	break;
	// p.make_posi(id, zdd_parent);
      }
    }
  }
  if(nchild == -2 || nchild == -4) {
    std::cout << "result is win" << endl;
    std::cout << "win game length is " << win_game_length << endl;
  }

  return 0;
}
