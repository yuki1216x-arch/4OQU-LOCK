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

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]);
  int vision = atoi(argv[2]); // 0: white's perspective, 1: black's perspective
  unsigned int value = atoi(argv[3]); // Target result to search for (1 or 2)
  unsigned int game_length = atoi(argv[4]); // Target game length
  string base_filename = base[vision];
  string read_filename_str = "data/db/" + base_filename + "_" + to_string(iteration) + ".bin";
  unique_ptr<ZDD> zdd_parent = make_unique<ZDD>(vision, iteration);

  size_t bits_per_entry;
  if(iteration > 12) bits_per_entry = 4;
  else bits_per_entry = 8;
  Table parent_table(16 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);   // parent table
  Posi p;

  unsigned long long int haiti_num = 0;
  
  for(unsigned long long int i = 0ULL; i < placement_count[vision][16 - iteration]; i++) {
    if(parent_table.get_value(i) == value && parent_table.get_game_length(i) == game_length) {
      cout << "number = " << haiti_num << ", id = " << i << endl;

      p.make_posi(i, *zdd_parent); // create the i-th parent configuration
      p.print();
      
      haiti_num++;
    }
  }
  cout << "total count = " << haiti_num << endl;

  return 0;
}
