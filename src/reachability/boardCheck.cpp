#include <memory>
#include <iomanip>
#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::to_string;

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

unsigned long long int nunreachable = 0;

int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]); // number of moves
  int vision = atoi(argv[2]); // player perspective (0: white, 1: black)
  
  string base_filename = "reachability_" + base[vision];
  string read_filename_str = "data/db_reachability/" + base_filename + "_" + to_string(iteration) + ".bin";
  unique_ptr<ZDD> zdd = make_unique<ZDD>(vision, iteration);

  size_t bits_per_entry = 1;
    
  Table table(16 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);
  Posi p;

  unsigned long long int max_haiti = placement_count[vision][16 - iteration];
    
  for(unsigned long long int i = 0ULL; i < max_haiti; i++) {
    if(table.get(i) == 0ULL) {
      cout << nunreachable++ << endl;
      p.make_posi(i, *zdd);
      p.print();
    }
  }
  cout << "finish!!" << endl;
}
