#include <memory>
#include <chrono>
#include <random>
#include <iomanip>
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


constexpr unsigned long long int placement_count[2][17] {
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
    144799200ULL, 61850880ULL, 17357760ULL, 4264960ULL,
    666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
    144799200ULL, 46388160ULL, 17357760ULL, 2932160ULL,
    666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

const string base[2] = {"white_table", "black_table"};

unsigned long long int nwin = 0, nlose = 0, ndraw = 0, nunknown = 0, nnotwin = 0, nnotlose = 0, nplacement = 0;

unsigned long long int win_length_counts[17] = {}, loss_length_counts[17] = {};

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]);
  int vision = atoi(argv[2]); //0なら白視点, 1なら黒視点
  string base_filename = base[vision];
  string read_filename_str = "data/db/" + base_filename + "_" + to_string(iteration) + ".bin";
  string read_filename_str_reachability = "data/db_reachability/reachability_" + base_filename + "_" + to_string(iteration) + ".bin";

  size_t bits_per_entry;
  if(iteration > 12) bits_per_entry = 4;
  else bits_per_entry = 8;
  Table table(16 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);   // 親のtable
  Table table_reachability(16 - iteration, read_filename_str_reachability.c_str(), 1, placement_count[vision][16 - iteration]); //子のtalbe

  for(unsigned long long int i = 0ULL; i < placement_count[vision][16 - iteration]; i++) {
    if(table_reachability.get(i) == 0ULL) continue;

    nplacement++;
    int value = table.get_value(i);
    if(value == 1) {
      nwin++;
      win_length_counts[table.get_game_length(i)]++;
    } else if(value == 2) {
      nlose++;
      loss_length_counts[table.get_game_length(i)]++;
    } else if(value == 3) {
      ndraw++;
    } else if(value == 4) {
      nnotlose++;
    } else if(value == 5) {
      nnotwin++;
    } else {
      assert(value == 0);
      nunknown++;
    }
  }

  double ratio_win = static_cast<double>(nwin) / static_cast<double>(nplacement) * 100.0;
  double ratio_lose = static_cast<double>(nlose) / static_cast<double>(nplacement) * 100.0;
  double ratio_draw = static_cast<double>(ndraw) / static_cast<double>(nplacement) * 100.0;
  double ratio_notlose = static_cast<double>(nnotlose) / static_cast<double>(nplacement) * 100.0;
  double ratio_notwin = static_cast<double>(nnotwin) / static_cast<double>(nplacement) * 100.0;

  cout << "Total configurations :" << nplacement << endl;
  cout << "nwin = " << nwin << " (" << std::fixed << std::setprecision(5) << ratio_win << "%)" << endl;
  cout << "nlose = " << nlose << " (" << std::fixed << std::setprecision(5) << ratio_lose << "%)" << endl;
  cout << "ndraw = " << ndraw << " (" << std::fixed << std::setprecision(5) << ratio_draw << "%)" << endl;
  cout << "nnotlose = " << nnotlose << " (" << std::fixed << std::setprecision(5) << ratio_notlose << "%)" << endl;
  cout << "nnotwin = " << nnotwin << " (" << std::fixed << std::setprecision(5) << ratio_notwin << "%)" << endl;
    
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
