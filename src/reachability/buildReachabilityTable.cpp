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

unsigned long long int nreachable = 0;

int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]); // number of moves
  int vision = atoi(argv[2]); // player perspective (0: white, 1: black)

  size_t bits_per_entry = 1;
  
  if(iteration == 0) {
    string base_filename = "reachability_" + base[vision];
    string read_filename_str = "data/db_reachability/" + base_filename + "_" + to_string(iteration) + ".bin";

    cout << "before_outtable" << endl;
    { 
      OutTable out_table(16 - iteration, read_filename_str.c_str(), placement_count[vision][16 - iteration], bits_per_entry);
      for(unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) {
	out_table.write(1ULL);
	nreachable++;
      }
      out_table.flush();
    }

    double ratio = static_cast<double>(nreachable) / static_cast<double>(placement_count[vision][16 - iteration]) * 100.0;

    cout << "Total configurations      : " << placement_count[vision][16 - iteration] << endl;
    cout << "Reachable configurations  : " << nreachable << " (" << std::fixed << std::setprecision(5) << ratio << "%)" << endl;
  } else {
    string base_filename = "reachability_" + base[vision];
    string read_filename_str = "data/db_reachability/" + base_filename + "_" + to_string(iteration-1) + ".bin";
    string read_filename_str_child = "data/db_reachability/" + base_filename + "_" + to_string(iteration) + ".bin";
    unique_ptr<ZDD> zdd_parent = make_unique<ZDD>(vision, iteration - 1);
    unique_ptr<ZDD> zdd_child = make_unique<ZDD>(vision, iteration);
    
    Table parent_table(17 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][17 - iteration]);
    Table child_table(16 - iteration, read_filename_str_child.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);
    Posi p;

    unsigned long long int max_haiti = placement_count[vision][17 - iteration];
    unsigned long long int max_haiti_child = placement_count[vision][16 - iteration];
    
    for(unsigned long long int i = 0ULL; i < max_haiti; i++) {
      if(nreachable > max_haiti_child) std::terminate();
      if(parent_table.get(i) == 0ULL) continue;

      p.make_posi(i, *zdd_parent);
      Action actions[1000] = {}; // store legal moves
      unsigned char board_belief[12][46] = {}; // store the configuration from a third-party perspective
      int nchild = p.compute_actions(actions, vision, iteration-1); // enumerate legal moves
      int num_of_un = p.getunknowninfo(board_belief, vision, iteration-1); // store physical states (up to 12) in board_belief and return the count
      
      assert(num_of_un > 0);

      if(nchild > 0) {
	for(int j = 0; j < nchild; j++) { // loop over the number of legal moves
	  for(int k = 0; k < num_of_un; k++) { // loop over all nodes in the belief state
	    p.make_posi_n(board_belief, k); // create the configuration from a third-party perspective
	    p.make_action(actions[j], vision, iteration-1);
	    p.make_posi_myself();
	    unsigned long long int num_of_haiti = p.getzddnum(*zdd_child);
	    if(child_table.get(num_of_haiti) == 0ULL) {
	      child_table.set(num_of_haiti, 1ULL);
	      nreachable++;
	    }
	  }
	  p.make_posi(i, *zdd_parent);
	}
      }

      if(i % 100000000 == 0ULL) {
	cout << "id: " << i << ", Reachable configurations: " << nreachable << endl;
      }
    }
    
    cout << "before_outtable" << endl;
    {
      OutTable out_table(16 - iteration, read_filename_str_child.c_str(), placement_count[vision][16 - iteration], bits_per_entry);
      for(unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) {
	out_table.write(child_table.get(i));
      }
      out_table.flush();
    }
    
    double ratio = static_cast<double>(nreachable) / static_cast<double>(placement_count[vision][16 - iteration]) * 100.0;
    
    cout << "Total configurations      : " << placement_count[vision][16 - iteration] << endl;
    cout << "Reachable configurations  : " << nreachable << " (" << std::fixed << std::setprecision(5) << ratio << "%)" << endl;
  }
}
