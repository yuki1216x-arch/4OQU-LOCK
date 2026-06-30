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

constexpr int nworker = 7;
constexpr int deq_input_size = 1024;
constexpr int deq_output_size = 256;
constexpr int max_legal_num = 1000;
constexpr int max_belief_state = 12;

condition_variable cv_boss;
condition_variable cv_worker;
mutex mtx;
mutex zdd_mtx;

bool flag_worker_quit = false; // 仕事が終わったことを表すフラグ

class Work {
  unsigned long long int m_id;
  int m_nchild;
  int m_num_of_un;
  int m_nchild_opp[2][max_belief_state];
  int m_num_of_un_opp[2][max_belief_state];
  long long int m_array_id[2][max_legal_num][max_belief_state];
  long long int m_array_id_opp[2][max_belief_state][max_legal_num][max_belief_state];
public:
  Work() noexcept {}
  Work(unsigned long long int id) noexcept : m_id(id) {}
  void set_id(unsigned long long int id) noexcept { m_id = id; }
  void set(int nchild, int num_of_un) noexcept {
    m_nchild = nchild;
    m_num_of_un = num_of_un;
    assert(nchild < max_legal_num);
    assert(num_of_un <= max_belief_state);
  }
  void set(int nchild_opp[][max_belief_state], int num_of_un_opp[][max_belief_state],
	   long long int array_id[][max_legal_num][max_belief_state], long long int array_id_opp[][max_belief_state][max_legal_num][max_belief_state]) noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_nchild; j++) {
	for(int k = 0; k < m_num_of_un; k++) {
	  m_array_id[i][j][k] = array_id[i][j][k];
	}
      }
      for(int j = 0; j < m_num_of_un; j++) {
        m_nchild_opp[i][j] = nchild_opp[i][j];
	m_num_of_un_opp[i][j] = num_of_un_opp[i][j];
	assert(nchild_opp[i][j] > 0 && nchild_opp[i][j] < 1000);
	assert(num_of_un_opp[i][j] > 0 && num_of_un_opp[i][j] < 12);
	for(int k = 0; k < nchild_opp[i][j]; k++) {
	  for(int l = 0; l < num_of_un_opp[i][j]; l++) {
	    m_array_id_opp[i][j][k][l] = array_id_opp[i][j][k][l];
	  }
	}
      }
    }
  }
  unsigned long long int get_id() const noexcept { return m_id; }
  void get_count(int& nchild, int& num_of_un) const noexcept {
    nchild = m_nchild;
    num_of_un = m_num_of_un;
  }
  void get(long long int array_id[][max_legal_num][max_belief_state]) const noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_nchild; j++) {
	for(int k = 0; k < m_num_of_un; k++) {
	  array_id[i][j][k] = m_array_id[i][j][k];
	}
      }
    }
  }
  void get_opp(int nchild_opp[][max_belief_state], int num_of_un_opp[][max_belief_state],
	       long long int array_id_opp[][max_belief_state][max_legal_num][max_belief_state]) const noexcept {
    for(int i = 0; i < 2; i++) {
      for(int j = 0; j < m_num_of_un; j++) {
	nchild_opp[i][j] = m_nchild_opp[i][j];
	num_of_un_opp[i][j] = m_num_of_un_opp[i][j];
	for(int k = 0; k < m_nchild_opp[i][j]; k++) {
	  for(int l = 0; l < m_num_of_un_opp[i][j]; l++) {
	    array_id_opp[i][j][k][l] = m_array_id_opp[i][j][k][l];
	  }
	}
      }
    }
  }
};

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

unsigned long long int nwin = 0, nlose = 0, ndraw = 0, nunknown = 0, lunknown = 0, nnotwin = 0, nnotlose = 0;

unsigned long long int mm_me_win = 0, mm_ee_win = 0, me_mm_win = 0, me_em_win = 0; //(視点手番)_(視点手番)_(ラベル)
unsigned long long int mm_me_lose = 0, mm_ee_lose = 0, me_mm_lose = 0, me_em_lose = 0;
unsigned long long int mm_me_ntwin = 0, mm_ee_ntwin = 0, me_mm_ntwin = 0, me_em_ntwin = 0;
unsigned long long int mm_me_ntlose = 0, mm_ee_ntlose = 0, me_mm_ntlose = 0, me_em_ntlose = 0;
unsigned long long int all_draw = 0;
unsigned long long int win_length_counts[17] = {}, loss_length_counts[17] = {};

unsigned long long int al_win_num = 0;
unsigned long long int al_lose_num = 0;
unsigned long long int al_unknown_num = 0;
unsigned long long int perfect = 0;
unsigned long long int imperfect = 0;

deque<Work *> deq_input;
deque<Work *> deq_output;

static void boss(int iter, int vision, const char* write_file_name, const char* read_file_name1, const char* read_file_name2) noexcept {
  cout << "boss" << endl;

  size_t bits_per_entry, child_bits_per_entry;
  if(iter > 12) bits_per_entry = 4, child_bits_per_entry = 4;
  else if(iter > 11) bits_per_entry = 8, child_bits_per_entry = 4;
  else bits_per_entry = 8, child_bits_per_entry = 8;
  Table parent_table(16 - iter, write_file_name, bits_per_entry, placement_count[vision][16 - iter]);
  Table child_table(15 -iter, read_file_name1, child_bits_per_entry, placement_count[vision][15 - iter]);
  Table opp_child_table(15 - iter, read_file_name2, child_bits_per_entry, placement_count[1 - vision][15 - iter]);

  unsigned long long int max_placement = placement_count[vision][16 - iter];
  unsigned long long int count_input = 0ULL;
  unsigned long long int count_output = 0ULL;

  int nstack_work_idle = deq_input_size + deq_output_size + nworker;
  Work* stack_work_idle[nstack_work_idle];
  for(int i = 0; i < nstack_work_idle; i++) stack_work_idle[i] = new Work;

  cout << "aaa" << endl;

  while(true) {
    unique_lock<mutex> lck(mtx);
    cv_boss.wait(lck, [&](){
			return (((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && (count_input < max_placement))
				|| (0 < deq_output.size())); });

    if((deq_input.size() < deq_input_size) && (deq_output.size() < deq_output_size) && count_input < max_placement) {
      assert(nstack_work_idle >= 1);
      Work *pw = stack_work_idle[ --nstack_work_idle ];
      pw->set_id(count_input);
      deq_input.push_front(pw);
      lck.unlock();
      count_input++;

      if (count_input % 100000000ULL == 0ULL) { //1000000000ULL i
	cout << "id: " << count_input << ", nwin: " << nwin << ", nlose: " << nlose << ", ndraw: " << ndraw
	     << ", nnotwin: " << nnotwin << ", nnotlose: " << nnotlose << ", nunknown: " << nunknown << ", perfect: " << perfect << ", imperfect: " << imperfect << endl;
      }

      if (count_input % 10000000000ULL == 0ULL) { //1000000000ULL i
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
      cv_worker.notify_one();
      // cout << count_input << endl;
    } else {
      if(0 < deq_output.size()) {
	assert(0 < deq_output.size());
	deque<Work *> deq_tmp;
	swap(deq_tmp, deq_output);

	deq_output.clear();
	lck.unlock();
	count_output += deq_tmp.size();
	for(unsigned int workid = 0; workid < deq_tmp.size(); workid++) {
	  unsigned long long int id = deq_tmp[workid]->get_id();
	  int nchild, num_of_un;
	  deq_tmp[workid]->get_count(nchild, num_of_un);

	  assert(mm_me_win + mm_ee_win + me_mm_win + me_em_win + al_win_num == nwin);
	  assert(mm_me_lose + mm_ee_lose + me_mm_lose + me_em_lose + al_lose_num == nlose);
	  assert(mm_me_ntwin + mm_ee_ntwin + me_mm_ntwin + me_em_ntwin == nnotwin);
	  assert(mm_me_ntlose + mm_ee_ntlose + me_mm_ntlose + me_em_ntlose == nnotlose);
	  assert(all_draw == ndraw);
    
	  int notlose_flg = 0;
	  int which_ntlose = 0;

	  int win_game_length, lose_game_length;
	  lose_game_length = -1;
	  if(iter % 2 == vision) win_game_length = 99;
	  else win_game_length = -1;
    
	  if(num_of_un == 1) {
	    perfect++;
	  } else {
	    imperfect++;
	  }
	  assert(num_of_un > 0);
	  if(nchild == -1) {
	    al_win_num++;
	    win_game_length = 0;
	  } else if(nchild == -2) {
	    al_lose_num++;
	    lose_game_length = 0;
	  } else {
	    assert(nchild > 0);
	  }
	
	  int game_val = 0;
	  int already_decided = 0;
	  if(nchild > 0) {
	    long long int array_id[2][max_legal_num][max_belief_state];
	    deq_tmp[workid]->get(array_id);
	    
	    int num_of_action = nchild;
	    for(int which_table = 0; which_table < 2; which_table++) { //2種類のtableのどっちを使うか
	      int win_plan_num = 0; //必勝戦略の数
	      int lose_plan_num = 0; //必敗戦略の数
	      int nlose_plan_num = 0; //負無し戦略の数
	      int nwin_plan_num = 0; //勝無し戦略の数
	      int draw_plan_num = 0; //引分戦略の数
	      int unknown_plan_num = 0; //不明戦略の数
	      for(int j = 0; j < num_of_action; j++) { //合法手の数だけループ
		int kati = 0; //勝ち
		int make = 0; //負け
		int makenasi = 0; //負無し
		int katinasi = 0; //勝無し
		int hikiwake = 0; //引分
		int kati_game_length = -1;
		for(int m = 0; m < num_of_un; m++) { //belief state内の節点の数だけループ
		  long long int num_of_haiti = array_id[which_table][j][m];
		  if(num_of_haiti >= 0) { //まだ終わってない 表から値を取ってくる
		    if(which_table == 0) { //子が自分視点
		      int child_table_val = child_table.get_value((unsigned long long int)num_of_haiti); //子のtableから値を取得
		      assert(child_table_val >= 0 && child_table_val <= 5);
		      if(child_table_val == 1) {
			//視点プレイヤ勝ち
			kati++;
			kati_game_length = max((int)child_table.get_game_length((unsigned long long int)num_of_haiti) + 1, kati_game_length);
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
		    } else { //子が相手視点
		      int child_table_val = opp_child_table.get_value((unsigned long long int)num_of_haiti); //子のtableから値を取得
		      assert(child_table_val >= 0 && child_table_val <= 5);
		      if(child_table_val == 1) {
			//視点プレイヤ負け
			make++;
		      } else if(child_table_val == 2) {
			//視点プレイヤ勝ち
			kati++;
			kati_game_length = max((int)opp_child_table.get_game_length((unsigned long long int)num_of_haiti) + 1, kati_game_length);
		      } else if(child_table_val == 4) {
			katinasi++;
		      } else if(child_table_val == 5) {
			makenasi++;
		      } else if(child_table_val == 3) {
			hikiwake++;
		      }
		    }
		  } else {
		    if(num_of_haiti == -3) {
		      //視点プレイヤ負け
		      make++;
		    } else if(num_of_haiti == -2) {
		      //視点プレイヤ勝ち
		      kati++;
		      kati_game_length = max(1, kati_game_length);
		    } else if(num_of_haiti == -10) {
		      //引分
		      hikiwake++;
		    }
		  }
		}
		//if() { //手番プレイヤが視点プレイヤのとき
		if(kati == num_of_un) { //必勝
		  if(iter % 2 == vision) {
		    if(already_decided != 1) {
		      game_val = -4;
		      if(which_table == 0) {
			mm_me_win++;
		      } else {
			mm_ee_win++;
		      }
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
		if(j == num_of_action - 1 && iter % 2 == vision) {
		  if(nlose_plan_num > 0 || draw_plan_num > 0) {
		    if(already_decided != 1) {
		      notlose_flg = 1;
		      if(which_table == 0) {
			mm_me_ntlose++;
			which_ntlose = 1;
		      } else {
			mm_ee_ntlose++;
			which_ntlose = 2;
		      }
		    }
		    already_decided = 1;
		  }
		}
	      }
	      //すべての合法手を確認した後
	      if(already_decided == 0) {
		if(win_plan_num == num_of_action) {
		  game_val = -4; //必勝を与える
		  if(which_table == 0) {
		    me_mm_win++;
		  } else {
		    me_em_win++;
		  }
		} else if(nlose_plan_num + win_plan_num + draw_plan_num == num_of_action) {
		  if(which_table == 0) {
		    me_mm_ntlose++;
		    which_ntlose = 3;
		  } else {
		    me_em_ntlose++;
		    which_ntlose = 4;
		  }
		  notlose_flg = 1;
		}
	      }      
	      if(game_val < 0) {
		nchild = game_val;
		break;
	      } else if(notlose_flg == 1) {
		break;
	      }
	    }
	  }
	  if(nchild > 0) { //後半のループでは必敗、勝無しを判定
	    int nchild_opp[2][max_belief_state], num_of_un_opp[2][max_belief_state];
	    long long int array_id_opp[2][max_belief_state][max_legal_num][max_belief_state];
	    deq_tmp[workid]->get_opp(nchild_opp, num_of_un_opp, array_id_opp);
	    for(int which_table = 0; which_table < 2; which_table++) {
	      int opp_win_count = 0;
	      int opp_nlose_count = 0;
	      for(int j = 0; j < num_of_un; j++) { //親のbelief stateの中身の節点の数だけループ
		int num_of_action_opp = nchild_opp[which_table][j];
		int flg_l = 0;
		int flg_l2 = 0;
		int win_plan_num_opp = 0;
		int nlose_plan_num_opp = 0;
		int draw_plan_num_opp = 0;
		int unknown_plan_num_opp = 0;
		int lose_plan_game_length;
		if(iter % 2 != vision) lose_plan_game_length = 99;
		else lose_plan_game_length = -1;
		for(int k = 0; k < nchild_opp[which_table][j]; k++) {
		  int kati_opp = 0;
		  int make_opp = 0;
		  int hikiwake_opp = 0;
		  int makenasi_opp = 0;
		  int katinasi_opp = 0;
		  int make_game_length = -1;
		  flg_l = 0;
		  for(int m = 0; m < num_of_un_opp[which_table][j]; m++) {
		    long long int num_of_haiti_opp = array_id_opp[which_table][j][k][m];
		    if(num_of_haiti_opp >= 0) { //まだ終わってない 表から値を取ってくる
		      if(which_table == 0) {
			int child_table_val_opp = child_table.get_value((unsigned long long int)num_of_haiti_opp); //子のtableから値を取得
			assert(child_table_val_opp >= 0 && child_table_val_opp <= 5);
			if(child_table_val_opp == 1) {
			  kati_opp++; //元の親の視点プレイヤから見た必勝
			} else if(child_table_val_opp == 2) {
			  make_opp++; //の親の視点プレイヤから見た必敗
			  make_game_length = max((int)child_table.get_game_length((unsigned long long int)num_of_haiti_opp) + 1, make_game_length);
			} else if(child_table_val_opp == 3) {
			  hikiwake_opp++; //元の親の視点プレイヤから見た引分
			} else if(child_table_val_opp == 4) {
			  makenasi_opp++; //元の親の視点プレイヤから見た負無し
			} else if(child_table_val_opp == 5) {
			  katinasi_opp++; //元の親の視点プレイヤから見た勝無し
			}
		      } else {
			int child_table_val_opp = opp_child_table.get_value((unsigned long long int)num_of_haiti_opp); //子のtableから値を取得
			assert(child_table_val_opp >= 0 && child_table_val_opp <= 5);
			if(child_table_val_opp == 1) {
			  make_opp++; //元の親の視点プレイヤから見た必敗(visionが1なら白視点から見た負け)
			  make_game_length = max((int)opp_child_table.get_game_length((unsigned long long int)num_of_haiti_opp) + 1, make_game_length);
			} else if(child_table_val_opp == 2) {
			  kati_opp++; //元の親の視点プレイヤから見た必勝
			} else if(child_table_val_opp == 3) {
			  hikiwake_opp++;
			} else if(child_table_val_opp == 4) {
			  katinasi_opp++;
			} else if(child_table_val_opp == 5) {
			  makenasi_opp++;
			}
		      }
		    } else {
		      if(num_of_haiti_opp == -3) {
			//視点プレイヤ負け
			kati_opp++;
		      } else if(num_of_haiti_opp == -2) {
			//視点プレイヤ勝ち
			make_opp++;
			make_game_length = max(1, make_game_length);
		      } else if(num_of_haiti_opp == -10) {
			hikiwake_opp++;
		      }
		    }
		  }
		  if(make_opp == num_of_un_opp[which_table][j]) { //相手が必勝になる行動があった場合
		    if(iter % 2 != vision) {
		      if(flg_l2 != 1) opp_win_count++;
		      flg_l = 1;
		      flg_l2 = 1;
		      lose_plan_game_length = min(make_game_length, lose_plan_game_length);
		      lose_game_length = max(lose_plan_game_length, lose_game_length);
		    } else {
		      win_plan_num_opp++;
		      lose_plan_game_length = max(make_game_length, lose_plan_game_length);
		    }
		  } else if(make_opp + hikiwake_opp + katinasi_opp == num_of_un_opp[which_table][j]) {
		    if(make_opp == 0 && katinasi_opp == 0) {
		      draw_plan_num_opp++;
		    } else {
		      nlose_plan_num_opp++;
		    }
		  } else {
		    unknown_plan_num_opp++;
		  }
		  if(k == num_of_action_opp - 1 && iter % 2 != vision) {
		    if(nlose_plan_num_opp > 0 || draw_plan_num_opp > 0) {
		      if(flg_l2 != 1) opp_nlose_count++;
		      flg_l = 1;
		      flg_l2 = 1;
		    }
		  }
		}
		if(flg_l == 0) {
		  if(win_plan_num_opp == num_of_action_opp) {
		    flg_l = 1;
		    opp_win_count++;
		    lose_game_length = max(lose_plan_game_length, lose_game_length);
		  } else if(win_plan_num_opp + draw_plan_num_opp + nlose_plan_num_opp == num_of_action_opp) {
		    flg_l = 1;
		    opp_nlose_count++;
		  }
		}
		if(flg_l == 0) break;
	      }
	      if(opp_win_count == num_of_un) {
		nchild = -5; //必敗
		if(iter % 2 != vision) {
		  if(which_table == 0) {
		    me_mm_lose++;
		  } else {
		    me_em_lose++;
		  }
		} else {
		  if(which_table == 0) {
		    mm_me_lose++;
		  } else {
		    mm_ee_lose++;
		  }
		}
		break;
	      } else if(opp_win_count + opp_nlose_count == num_of_un) {
		if(notlose_flg == 1) { //引分
		  nchild = -8;
		  all_draw++;
		  if(iter % 2 == vision) {
		    if(which_ntlose == 1) {
		      mm_me_ntlose--;
		    } else if(which_ntlose == 2) {
		      mm_ee_ntlose--;
		    }
		  } else {
		    if(which_ntlose == 3) {
		      me_mm_ntlose--;
		    } else if(which_ntlose == 4) {
		      me_em_ntlose--;
		    }
		  }
		  break;
		} else {
		  nchild = -7; //勝無し
		  if(iter % 2 != vision) {
		    if(which_table == 0) {
		      me_mm_ntwin++;
		    } else {
		      me_em_ntwin++;
		    }
		  } else {
		    if(which_table == 0) {
		      mm_me_ntwin++;
		    } else {
		      mm_ee_ntwin++;
		    }
		  }
		  break;
		}
	      }
	    } 
	  }
	  if(nchild > 0 && notlose_flg == 1) {
	    nchild = -6;
	  }
	  if(nchild == -1 || nchild == -4) {
	    assert(win_game_length >= 0 && win_game_length <= 16 - iter);
	    parent_table.set(id, v_win, win_game_length);
	    //cout << "win" << endl;
	    nwin++;
	    win_length_counts[win_game_length]++;
	  } else if(nchild == -2 || nchild == -5) {
	    assert(lose_game_length >= 0 && lose_game_length <= 16 - iter);
	    parent_table.set(id, v_lose, lose_game_length);
	    //cout << "lose" << endl;
	    nlose++;
	    loss_length_counts[lose_game_length]++;
	  } else if(nchild == -8) {
	    parent_table.set(id, v_draw, 0ULL);
	    //cout << "draw" << endl;
	    ndraw++;
	  } else if(nchild == -6) {
	    parent_table.set(id, v_notlose, 0ULL);
	    //cout << "notlose" << endl;
	    nnotlose++;
	  } else if(nchild == -7) {
	    parent_table.set(id, v_notwin, 0ULL);
	    //cout << "notwin" << endl;
	    nnotwin++;
	  } else {
	    nunknown++;
	    parent_table.set(id, v_unknown, 0ULL);
	    //cout << "unknown" << endl;
	  }
	}
	//紙を棚に戻す作業
	for(size_t i = 0; i < deq_tmp.size(); i++) {
	  assert(nstack_work_idle < 2048);
	  stack_work_idle[ nstack_work_idle++ ] = deq_tmp[i];
	}
      }
      if(count_output >= max_placement) break;
    }
  }

  cout << "before_outtable" << endl;                                                                 // ***
  {   
    OutTable out_table(16 - iter, write_file_name, max_placement, bits_per_entry);
    for (unsigned long long int i = 0; i < max_placement; i++) { //haiti
      out_table.write(parent_table.get(i));
    }
    out_table.flush();
  }

  {
    unique_lock<mutex> lck(mtx); // ロック
    flag_worker_quit = true; // 仕事が終わったことを表すフラグ
  }
  cv_worker.notify_all();
}

static void worker(int iteration, int vision, const ZDD& zdd_parent, const ZDD& zdd_child, const ZDD& zdd_child_opp) noexcept {
  Work *w;
  while(true) {
    unique_lock<mutex> lck(mtx);

    cv_worker.wait(lck, [&](){ if(0 < deq_input.size()) return true;
	return flag_worker_quit; });
    if(flag_worker_quit) break;
    assert(0 < deq_input.size());
    w = deq_input.back();
    deq_input.pop_back();
    lck.unlock();

    unsigned long long int id = w->get_id();
    Posi p;
    p.make_posi(id, zdd_parent);
    Action actions[1000] = {};
    unsigned char board_belief[12][46] = {};
    int nchild = p.compute_actions(actions, vision, iteration);
    int num_of_un = p.getunknowninfo(board_belief, vision, iteration);
    assert(nchild == -2 || nchild == -1 || (nchild > 0 && nchild < max_legal_num));
    assert(num_of_un > 0 && num_of_un <= max_belief_state);

    w->set(nchild, num_of_un);
    if(nchild > 0) {
      int nchild_opp[2][max_belief_state], num_of_un_opp[2][max_belief_state];
      long long int array_id[2][max_legal_num][max_belief_state], array_id_opp[2][max_belief_state][max_legal_num][max_belief_state];

      for(int which_table = 0; which_table < 2; which_table++) {
	for(int j = 0; j < nchild; j++) {
	  for(int m = 0; m < num_of_un; m++) {
	    p.make_posi_n(board_belief, m); // 第3者視点の配置を作る
	    int board_check = p.make_action(actions[j], vision, iteration);
	    if(board_check == 0) { // まだ終わってない
	      if(which_table == 0) { // 子が自分視点
		p.make_posi_myself();
		array_id[which_table][j][m] = p.getzddnum(zdd_child); // 子のZDDから子の配置の番号を取得
	      } else { // 子が相手視点
		p.make_posi_opponent();
		array_id[which_table][j][m] = p.getzddnum(zdd_child_opp); // 子のZDDから子の配置の番号を取得
	      }
	    } else {
	      if(board_check == 3) array_id[which_table][j][m] = -3;
	      else if(board_check == 2) array_id[which_table][j][m] = -2;
	      else {
		assert(board_check == 10);
		array_id[which_table][j][m] = -10;
	      }
	    }
	  }
	  // p.make_posi(id, zdd_parent);
	}
      }

      p.make_posi(id, zdd_parent);
      for(int which_table = 0; which_table < 2; which_table++) {
	for(int j = 0; j < num_of_un; j++) {
	  Action actions_opp[1000] = {};
	  unsigned char board_belief_opp[12][46] = {};
	  p.make_posi_n(board_belief, j);
	  p.make_posi_opponent();
	  nchild_opp[which_table][j] = p.compute_actions(actions_opp, 1 - vision, iteration);
	  num_of_un_opp[which_table][j] = p.getunknowninfo(board_belief_opp, 1 - vision, iteration);
	  assert(nchild_opp[which_table][j] > 0);
	  assert(nchild_opp[which_table][j] < max_legal_num);
	  assert(num_of_un_opp[which_table][j] > 0 && num_of_un_opp[which_table][j] <= max_belief_state);
	  for(int k = 0; k < nchild_opp[which_table][j]; k++) {
	    for(int m = 0; m < num_of_un_opp[which_table][j]; m++) {
	      p.make_posi_n(board_belief_opp, m); // 第3者視点の配置を作る
	      int board_check = p.make_action(actions_opp[k], 1 - vision, iteration);
	      if(board_check == 0) {// まだ終わってない
		if(which_table == 0) { // 子が相手視点
		  p.make_posi_opponent();
		  array_id_opp[which_table][j][k][m] = p.getzddnum(zdd_child); // 子のZDDから子の配置の番号を取得
		} else {
		  p.make_posi_myself();
		  array_id_opp[which_table][j][k][m] = p.getzddnum(zdd_child_opp); // 子のZDDから子の配置の番号を取得
		}
	      } else {
		if(board_check == 3) array_id_opp[which_table][j][k][m] = -3;
		else if(board_check == 2) array_id_opp[which_table][j][k][m] = -2;
		else {
		  assert(board_check == 10);
		  array_id_opp[which_table][j][k][m] = -10;
		}
	      }
	    }
	    // p.make_posi(id, zdd_parent);
	  }
	}
      }
      w->set(nchild_opp, num_of_un_opp, array_id, array_id_opp);
    }

    lck.lock(); // ロック（deq_outputに触るため）
    deq_output.push_front(w); // 1つのスレッドしか触っちゃいけない
    lck.unlock(); // ロック解除
    cv_boss.notify_one(); // cv_bossで現在waitをcallしているスレッド1つに信号が行く。
  }
}

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]);
  int vision = atoi(argv[2]); //0なら白視点, 1なら黒視点
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

  thread th_boss([=]{boss(iteration, vision, read_filename_str.c_str(), read_filename_str_child.c_str(), read_filename_str_child_opp.c_str());}); // boss側を作る

  thread th_worker[nworker];
  for(int workerid = 0; workerid < nworker; workerid++) {
    th_worker[workerid] = thread(worker, iteration, vision, std::cref(*zdd_parent), std::cref(*zdd_child), std::cref(*zdd_child_opp)); // ここでworker()を呼び出す
  }

  // 終了処理
  th_boss.join();
  for(int workerid = 0; workerid < nworker; workerid++) {
    th_worker[workerid].join();
  }
  
  cout << "nwin  =  " << nwin << endl;
  cout << "nlose  =  " << nlose << endl;
  cout << "ndraw  =  " << ndraw << endl;
  cout << "nunknown  =  " << nunknown << endl;
  cout << "nnotwin  =  " << nnotwin << endl;
  cout << "nnotlose  =  " << nnotlose << endl;
  cout << "alwin = " << al_win_num << endl;
  cout << "allose = " << al_lose_num << endl;
  cout << "alunknown = " << al_unknown_num << endl;
  cout << "perfect = " << perfect << endl;
  cout << "imperfect = " << imperfect << endl;
  
  cout << "mm_me_win = " << mm_me_win << ", mm_ee_win = " << mm_ee_win << ", me_mm_win = " << me_mm_win << ", me_em_win = " << me_em_win << endl;
  cout << "mm_me_lose = " << mm_me_lose << ", mm_ee_lose = " << mm_ee_lose << ", me_mm_lose = " << me_mm_lose << ", me_em_lose = " << me_em_lose << endl;
  cout << "mm_me_ntwin = " << mm_me_ntwin << ", mm_ee_ntwin = " << mm_ee_ntwin << ", me_mm_ntwin = " << me_mm_ntwin << ", me_em_ntwin = " << me_em_ntwin << endl;
  cout << "mm_me_ntlose = " << mm_me_ntlose << ", mm_ee_ntlose = " << mm_ee_ntlose << ", me_mm_ntlose = " << me_mm_ntlose << ", me_em_ntlose = " << me_em_ntlose << endl;
  cout << "all_draw = " << all_draw << endl;

  cout << endl;
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
  
  return 0;
}

//  g++ -O2 -Wall -pthread main.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o maketable.out 2>&1
//  ./run_maktable.sh &
//  ./run_maketable_b.sh &

//  ./combtab.out 11 1 4 black_table black_table_11_4_0.bin black_table_11_4_1.bin black_table_11_4_2.bin black_table_11_4_3.bin db db db db db > 11_b_new2.txt 2>&1 &
//  ./combtab.out 11 0 4 white_table white_table_11_4_0.bin white_table_11_4_1.bin white_table_11_4_2.bin white_table_11_4_3.bin db db db db db > 11_w_new2.txt 2>&1 &

//  g++ -O2 -Wall -pthread CheckTable.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o checktable.out 2>&1
//  ./checktable.out 11 1 black_table_11.bin db > check_table_11_w.txt 2>&1 &

//  g++ -O2 -Wall -pthread notmaketable.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o notmaketable.out 2>&1
//  ./notmaketable.out 11 1 black_table_12.bin white_table_12.bin db db > notmaketab.txt 2>&1 &
