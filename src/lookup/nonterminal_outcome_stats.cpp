#include <iostream>
#include <string>

#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using namespace std;


// Number of placements for each vision and turn.
//
// placement_count[vision][16 - turn]
//
// vision:
//   0 : white
//   1 : black
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


// Statistics for reachable nonterminal positions.
struct OutcomeStats {
  unsigned long long win = 0;
  unsigned long long lose = 0;
  unsigned long long draw = 0;
  unsigned long long unknown = 0;
  unsigned long long notwin = 0;
  unsigned long long notlose = 0;

  unsigned long long total() const noexcept
  {
    return win + lose + draw + unknown + notwin + notlose;
  }
};


// Analyze one turn and one vision.
//
// The result table and reachability table are local variables.
// Therefore, their memory is released when this function returns.
OutcomeStats analyze(int turn, int vision)
{
  const string base =
    (vision == 0) ? "white_table" : "black_table";


  // ------------------------------------------------------------
  // Set filenames
  // ------------------------------------------------------------
  
  const string result_filename =
    "data/db/" + base + "_" + to_string(turn) + ".bin";


  const string reachability_filename =
    "data/db_reachability/reachability_" + base + "_" + to_string(turn) + ".bin";


  // ------------------------------------------------------------
  // Number of placements
  // ------------------------------------------------------------

  const unsigned long long max_placement =
    placement_count[vision][16 - turn];


  // ------------------------------------------------------------
  // Bits per entry of result database
  // ------------------------------------------------------------

  size_t result_bits_per_entry;

  if(turn > 12) {
    result_bits_per_entry = 4;
  } else {
    result_bits_per_entry = 8;
  }


  // ------------------------------------------------------------
  // Open databases
  // ------------------------------------------------------------

  /*
   * Only these two databases are opened for the current
   * turn and vision.
   *
   * They are destroyed when this function returns.
   */

  Table result_table(
		     16 - turn,
		     result_filename.c_str(),
		     result_bits_per_entry,
		     max_placement
		     );


  /*
   * Reachability database:
   *
   *   1 bit per entry
   *
   *   0 : unreachable
   *   1 : reachable
   */

  constexpr size_t REACHABILITY_BITS_PER_ENTRY = 1;


  Table reachability_table(
			   16 - turn,
			   reachability_filename.c_str(),
			   REACHABILITY_BITS_PER_ENTRY,
			   max_placement
			   );


  // ------------------------------------------------------------
  // Create ZDD and Posi
  // ------------------------------------------------------------

  ZDD zdd(vision, turn);

  Posi posi;


  // ------------------------------------------------------------
  // Statistics
  // ------------------------------------------------------------

  OutcomeStats stats;


  // ------------------------------------------------------------
  // Search all placements
  // ------------------------------------------------------------

  for(unsigned long long id = 0;
      id < max_placement;
      id++)
    {
      // Skip unreachable positions.
      if(reachability_table.get(id) != 1) {
	continue;
      }


      // Reconstruct the position.
      posi.make_posi(id, zdd);


      // Skip terminal positions.
      if(posi.TerminalTest(vision, turn) != 0) {
	continue;
      }


      // Get the outcome label.
      const unsigned int value =
	result_table.get_value(id);


      // Count each outcome label.
      switch(value)
	{
	case v_win:
	  stats.win++;
	  break;

	case v_lose:
	  stats.lose++;
	  break;

	case v_draw:
	  stats.draw++;
	  break;

	case v_unknown:
	  stats.unknown++;
	  break;

	case v_notwin:
	  stats.notwin++;
	  break;

	case v_notlose:
	  stats.notlose++;
	  break;

	default:
	  cerr << "Error: invalid outcome label"
	       << " turn=" << turn
	       << " vision=" << vision
	       << " id=" << id
	       << " value=" << value
	       << endl;

	  terminate();
	}
    }

  
  return stats;
}


// Print one result.
void print_stats(
		 int turn,
		 int vision,
		 const OutcomeStats& stats)
{
  cout << turn
       << '\t'
       << ((vision == 0) ? "white" : "black")
       << '\t'
       << stats.win
       << '\t'
       << stats.lose
       << '\t'
       << stats.draw
       << '\t'
       << stats.unknown
       << '\t'
       << stats.notwin
       << '\t'
       << stats.notlose
       << '\t'
       << stats.total()
       << endl;
}


int main()
{
  // ------------------------------------------------------------
  // Header
  // ------------------------------------------------------------

  cout << "turn"
       << '\t'
       << "vision"
       << '\t'
       << "win"
       << '\t'
       << "lose"
       << '\t'
       << "draw"
       << '\t'
       << "unknown"
       << '\t'
       << "notwin"
       << '\t'
       << "notlose"
       << '\t'
       << "total"
       << endl;


  // ------------------------------------------------------------
  // Analyze turn 0 to 15
  // ------------------------------------------------------------

  for(int turn = 0; turn <= 15; turn++) {

    // ------------------------------------------------------------
    // White vision
    // ------------------------------------------------------------

    {
      const OutcomeStats stats =
	analyze(turn, 0);

      print_stats(
		  turn,
		  0,
		  stats
		  );
    }


    // ------------------------------------------------------------
    // Black vision
    // ------------------------------------------------------------

    {
      const OutcomeStats stats =
	analyze(turn, 1);

      print_stats(
		  turn,
		  1,
		  stats
		  );
    }
  }


  return 0;
}
