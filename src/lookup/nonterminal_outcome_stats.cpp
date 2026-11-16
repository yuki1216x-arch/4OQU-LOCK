#include <iostream>
#include <string>

#include "table.hpp"
#include "node.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using namespace std;

// Indexed by viewpoint (0: White, 1: Black) and 16 minus the turn.
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

// Statistics for reachable nonterminal positions.
struct OutcomeStats {
  unsigned long long win = 0;
  unsigned long long loss = 0;
  unsigned long long draw = 0;
  unsigned long long unknown = 0;
  unsigned long long no_win = 0;
  unsigned long long no_loss = 0;

  unsigned long long total() const noexcept
  {
    return win + loss + draw + unknown + no_win + no_loss;
  }
};

// Analyze one turn and one vision.
//
// The result table and reachability table are local variables.
// Therefore, their memory is released when this function returns.
OutcomeStats analyze(int turn, int viewpoint)
{
  const string database_prefix = (viewpoint == 0) ? "white_table" : "black_table";

  // Set filenames
  const string result_database_path = "data/db/" + database_prefix + "_" + to_string(turn) + ".bin";
  const string reachability_database_path = "data/db_reachability/reachability_" + database_prefix + "_" + to_string(turn) + ".bin";

  // Number of placements
  const unsigned long long configuration_count = configuration_counts[viewpoint][16 - turn];

  // Bits per entry of result database
  const size_t result_bits_per_entry = turn > 12 ? 4 : 8;

  // Open databases

  // Only these two databases are opened for the current turn and vision.
  // They are destroyed when this function returns.

  Table result_table(16 - turn, result_database_path.c_str(), result_bits_per_entry, configuration_count);

  /*
   * Reachability database:
   *
   *   1 bit per entry
   *
   *   0 : unreachable
   *   1 : reachable
   */

  constexpr size_t reachability_bits_per_entry = 1;
  Table reachability_table(16 - turn, reachability_database_path.c_str(), reachability_bits_per_entry, configuration_count);

  // Reconstruct postions using the ZDD for this viewpoint.
  ZDD zdd(viewpoint, turn);
  Posi position;

  // Statistics
  OutcomeStats stats;

  // Search all placements
  for(unsigned long long configuration_id = 0; configuration_id < configuration_count; configuration_id++) {
      // Skip unreachable positions.
      if(reachability_table.get(configuration_id) != 1) continue;

      // Reconstruct the position.
      position.make_posi(configuration_id, zdd);

      // Skip terminal positions.
      if(position.TerminalTest(viewpoint, turn) != 0) continue;

      // Get the outcome label.
      const unsigned int outcome_label = result_table.get_value(configuration_id);

      // Count each outcome label.
      switch(outcome_label)
	{
	case v_win:
	  stats.win++;
	  break;

	case v_lose:
	  stats.loss++;
	  break;

	case v_draw:
	  stats.draw++;
	  break;

	case v_unknown:
	  stats.unknown++;
	  break;

	case v_notwin:
	  stats.no_win++;
	  break;

	case v_notlose:
	  stats.no_loss++;
	  break;

	default:
	  cerr << "Error: invalid outcome label"
	       << " turn=" << turn
	       << " vision=" << viewpoint
	       << " id=" << configuration_id
	       << " value=" << outcome_label
	       << endl;

	  terminate();
	}
    }

  
  return stats;
}


// Print one result.
void print_stats(int turn, int viewpoint, const OutcomeStats& stats) {
  cout << turn << '\t'
       << ((viewpoint == 0) ? "white" : "black") << '\t'
       << stats.win << '\t' << stats.loss << '\t' << stats.draw << '\t'
       << stats.unknown << '\t' << stats.no_win << '\t' << stats.no_loss << '\t' << stats.total() << endl;
}

int main() {
  // Header
  cout << "turn" << '\t' << "vision" << '\t'
       << "win" << '\t' << "loss" << '\t' << "draw" << '\t' << "unknown" << '\t'
       << "no win" << '\t' << "no loss" << '\t' << "total" << endl;

  // Analyze turn 0 to 15
  for(int turn = 0; turn <= 15; turn++) {
    // White viewpoint
    {
      const OutcomeStats stats = analyze(turn, 0);
      print_stats(turn, 0, stats);
    }

    // Black viewpoint
    {
      const OutcomeStats stats = analyze(turn, 1);
      print_stats(turn, 1, stats);
    }
  }

  return 0;
}
