#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <string>

#include "table.hpp"

namespace {

  // Match the database builders: index by viewpoint and then by 16 - iteration.
  constexpr unsigned long long int configuration_counts[2][17] {
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
	19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
	144799200ULL, 61850880ULL, 17357760ULL, 4264960ULL,
	666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
      {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
	  19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
	  144799200ULL, 46388160ULL, 17357760ULL, 2932160ULL,
	  666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
  };

  const string database_prefixes[2] = {"white_table", "black_table"};

  //Preserve the report's two-column layout for lengths 0 through 16.
  void print_length_histogram(const char* title, const unsigned long long (&counts)[17]) {
    cout << endl << title << endl;
    for(int length = 0; length <= 8; ++length) {
      cout << length << (length == 1 ? " move : " : " moves: ")
	   << std::setw(12) << counts[length] << " positions";
      if(length < 8) {
	const int right_length = length + 9;
	cout << std::setw(11) << right_length << " moves: "
	     << std::setw(12) << counts[right_length] << " positions";
      }
      cout << endl;
    }
  }
}   // namespace

int main(int argc, char* argv[]) {
  // The arguments are the iteration and the database viewpoint.
  const int iteration = std::atoi(argv[1]);
  const int viewpoint = std::atoi(argv[2]);   // 0: White; 2: Black.
  const int remaining_placement_count = 16 - iteration;
  const auto configuration_count = configuration_counts[viewpoint][remaining_placement_count];

  // Both tables use the same configuration IDs for this iteration and viewpoint.
  const std::string database_filename = database_prefixes[viewpoint] + "_" + std::to_string(iteration) + ".bin";
  const std::string result_database_path = "data/db/" + database_filename;
  const std::string reachability_database_path = "data/db_reachability/reachability_" + database_filename;

  const std::size_t result_bits_per_entry = iteration > 12 ? 4 : 8;
  const Table result_table(remaining_placement_count, result_database_path.c_str(), result_bits_per_entry, configuration_count);
  const Table reachability_table(remaining_placement_count, reachability_database_path.c_str(), 1, configuration_count);

  unsigned long long reachable_configuration_count = 0;
  unsigned long long win_count = 0, loss_count = 0, draw_count = 0;
  unsigned long long no_loss_count = 0, no_win_count = 0, unknown_count = 0;
  unsigned long long win_length_counts[17] = {}, loss_length_counts[17] = {};

  // Count only the configuration marked reachable by the reachability table.
  for(unsigned long long configuration_id = 0ULL; configuration_id < configuration_count; ++configuration_id) {
    if(reachability_table.get(configuration_id) == 0) continue;

    ++reachable_configuration_count;
    const auto outcome_label = result_table.get_value(configuration_id);
    if(outcome_label == v_win) {
      ++win_count;
      ++win_length_counts[result_table.get_game_length(configuration_id)];
    } else if(outcome_label == v_lose) {
      ++loss_count;
      ++loss_length_counts[result_table.get_game_length(configuration_id)];
    } else if(outcome_label == v_draw) {
      ++draw_count;
    } else if(outcome_label == v_notlose) {
      ++no_loss_count;
    } else if(outcome_label == v_notwin) {
      ++no_win_count;
    } else {
      assert(outcome_label == v_unknown);
      ++unknown_count;
    }
  }

  // Use all reachable configurations, including unknown labels, as the denominator.
  const auto percentage = [reachable_configuration_count](unsigned long long count) {
			    return static_cast<double>(count) / static_cast<double>(reachable_configuration_count) * 100.0;
			  };
  cout << "Total configurations :" << reachable_configuration_count << endl;
  cout << "win_count = " << win_count << " (" << std::fixed << std::setprecision(5) << percentage(win_count) << "%)" << endl;
  cout << "loss_count = " << loss_count << " (" << std::fixed << std::setprecision(5) << percentage(loss_count) << "%)" << endl;
  cout << "draw_count = " << draw_count << " (" << std::fixed << std::setprecision(5) << percentage(draw_count) << "%)" << endl;
  cout << "no_loss_count = " << no_loss_count << " (" << std::fixed << std::setprecision(5) << percentage(no_loss_count) << "%)" << endl;
  cout << "no_win_count = " << no_win_count << " (" << std::fixed << std::setprecision(5) << percentage(no_win_count) << "%)" << endl;
    
  print_length_histogram("win positions by shortest win length:", win_length_counts);
  print_length_histogram("lose positions by longest lose length;", loss_length_counts);

  return 0;
}
