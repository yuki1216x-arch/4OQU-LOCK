#include <iostream>
#include <string>
#include <cstdlib>

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

constexpr int MAX_LOCKED_PIECES = 8;


int main(int argc, char* argv[])
{
  // ------------------------------------------------------------
  // Read arguments
  // ------------------------------------------------------------

  if(argc != 3) {
    cerr << "Usage: "
	 << argv[0]
	 << " <turn: 0-16> <vision: 0=white, 1=black>"
	 << endl;
    
    return 1;
  }
  
  const int turn = atoi(argv[1]);
  const int vision = atoi(argv[2]);
  
  
  if(turn < 0 || turn > 16) {
    cerr << "Error: turn must be between 0 and 16."
	 << endl;
    
    return 1;
  }
  
  
  if(vision != 0 && vision != 1) {
    cerr << "Error: vision must be 0 (white) or 1 (black)."
	 << endl;
    
    return 1;
  }
  
  
  // ------------------------------------------------------------
  // Set filenames
  // ------------------------------------------------------------
  
  const string base =
    (vision == 0)
    ? "white_table"
    : "black_table";
  
  
  const string result_filename =
    "data/db/"
    + base
    + "_"
    + to_string(turn)
    + ".bin";
  
  const string reachability_filename =
    "data/db_reachability/reachability_"
    + base
    + "_"
    + to_string(turn)
    + ".bin";
  
  
  // ------------------------------------------------------------
  // Number of placements
  // ------------------------------------------------------------
  
  const unsigned long long max_placement =
    placement_count[vision][16 - turn];
  
  
  // ------------------------------------------------------------
  // Open result database
  // ------------------------------------------------------------
  
  /*
   * The result database uses the same number of bits per entry
   * as src/analysis/main.cpp.
   */
  
  size_t result_bits_per_entry;
  
  if(turn > 12) {
    result_bits_per_entry = 4;
  }
  else {
    result_bits_per_entry = 8;
  }
  
  
  Table result_table(
		     16 - turn,
		     result_filename.c_str(),
		     result_bits_per_entry,
		     max_placement
		     );
  
  
  // ------------------------------------------------------------
  // Open reachability database
  // ------------------------------------------------------------
  
  /*
   * Reachability database:
   *
   *   1 bits per entry
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
  
  /*
   * reachable_count[white][black]
   *
   * Number of reachable positions having
   * "white" locked white pieces and
   * "black" locked black pieces.
   */
  unsigned long long
    reachable_count[MAX_LOCKED_PIECES + 1]
    [MAX_LOCKED_PIECES + 1] = {};


  /*
   * win_count[white][black]
   *
   * Number of v_win positions among the corresponding
   * reachable positions.
   */
  unsigned long long
  win_count[MAX_LOCKED_PIECES + 1]
  [MAX_LOCKED_PIECES + 1] = {};
  
  unsigned long long total_reachable = 0;
  unsigned long long total_win = 0;


  // ------------------------------------------------------------
  // Search all placements
  // ------------------------------------------------------------

  for(unsigned long long id = 0;
      id < max_placement;
      id++)
  {
    /*
     * Reachability:
     *
     *   0 : unreachable
     *   1 : reachable
     */
    if(reachability_table.get(id) != 1) {
      continue;
    }


    // Create the position corresponding to this ID.
    posi.make_posi(id, zdd);


    // Count locked pieces.
    int locked_white = 0;
    int locked_black = 0;


    posi.count_locked_pieces(
			     locked_white,
			     locked_black
			     );


    // Safety check.
    if(locked_white < 0
		     || locked_white > MAX_LOCKED_PIECES
       || locked_black < 0
			|| locked_black > MAX_LOCKED_PIECES)
    {
      cerr << "Error: invalid locked-piece count"
      << " id=" << id
      << " white=" << locked_white
      << " black=" << locked_black
      << endl;

      return 1;
    }


    // Count reachable position.
    reachable_count[locked_white][locked_black]++;

    total_reachable++;


    // Count winning position.
    if(result_table.get_value(id) == v_win) {
      win_count[locked_white][locked_black]++;

      total_win++;
    }
  }


  // ------------------------------------------------------------
  // Output results
  // ------------------------------------------------------------

  cout << "turn = "
  << turn
  << endl;

  
  cout << "vision = "
  << ((vision == 0) ? "white" : "black")
  << endl;


  cout << endl;


  cout << "locked_white"
  << '\t'
  << "locked_black"
  << '\t'
  << "reachable"
  << '\t'
  << "win"
  << endl;


  for(int white = 0;
      white <= MAX_LOCKED_PIECES;
      white++)
  {
    for(int black = 0;
	black <= MAX_LOCKED_PIECES;
	black++)
    {
      /*
       * Do not output combinations for which
       * no reachable position exists.
       */
      if(reachable_count[white][black] == 0) {
	continue;
      }

      cout << white
      << '\t'
      << black
      << '\t'
      << reachable_count[white][black]
      << '\t'
      << win_count[white][black]
      << endl;
    }
  }


  cout << endl;


  cout << "total_reachable = "
  << total_reachable
  << endl;


  cout << "total_win       = "
  << total_win
  << endl;


  return 0;
}
