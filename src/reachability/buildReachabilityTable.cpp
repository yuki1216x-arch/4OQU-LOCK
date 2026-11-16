#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

namespace {

  // Indexed by viewpoint (0: White, 1: Black) and 16 minus the iteration.
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

  constexpr std::size_t reachability_bits_per_entry = 1;
  constexpr unsigned unreachable = 0;
  constexpr unsigned reachable = 1;

  std::string reachability_database_path(int viewpoint, int iteration) {
    const char* prefix = viewpoint == 0 ? "white_table_" : "black_table_";
    return std::string("data/db_reachability/reachability_") + prefix + std::to_string(iteration) + ".bin";
  }

  bool read_arguments(int argc, char* argv[], int& iteration, int& viewpoint) {
    if(argc < 3 || (std::string(argv[2]) != "0" && std::string(argv[2]) != "1")) {
      std::cerr << "Usage: " << argv[0] << " ITERATION VIEWPOINT (0: White; 1: Black)\n";
      return false;
    }
    char* end = nullptr;
    const long value = std::strtol(argv[1], &end, 10);
    if(end == argv[1] || *end || value < 0 || value > 15) {
      std::cerr << "Iteration must be between 0 and 15.\n";
      return false;
    }
    iteration = static_cast<int>(value);
    viewpoint = argv[2][0] - '0';
    return true;
  }

  bool can_read_database(const std::string& path) {
    if(std::ifstream(path, std::ios::binary)) return true;
    std::cerr << "Cannot open reachability database: " << path << '\n';
    return false;
  }

  void print_summary(unsigned long long reachable_count, unsigned long long configuration_count) {
    const double percentage = 100.0 * reachable_count / configuration_count;
    std::cout << "Total configurations      : " << configuration_count << '\n'
	      << "Reachable configurations  : " << reachable_count << " ("
	      << std::fixed << std::setprecision(5) << percentage << "%)\n";
  }
}   // namespace

int main(int argc, char* argv[]) {
  int iteration, viewpoint;
  if(!read_arguments(argc, argv, iteration, viewpoint)) return 1;

  const auto output_path = reachability_database_path(viewpoint, iteration);
  const auto configuration_count = configuration_counts[viewpoint][16 - iteration];

  // Every initial configuration is reachable.
  if(iteration == 0) {
    std::cout << "[output] Writing database: " << output_path << '\n';
    OutTable writer(16 - iteration, output_path, configuration_count, reachability_bits_per_entry);
    for(unsigned long long id = 0; id < configuration_count; ++id) writer.write(reachable);
    writer.flush();
    print_summary(configuration_count, configuration_count);
    return 0;
  }

  // Expand the previous layer into the current layer for the same viewpoint.
  const int previous_iteration = iteration - 1;
  const auto parent_path = reachability_database_path(viewpoint, previous_iteration);
  if(!can_read_database(parent_path)) return 1;

  const auto parent_count = configuration_counts[viewpoint][16 - previous_iteration];
  const Table parent_table(16 - previous_iteration, parent_path.c_str(), reachability_bits_per_entry, parent_count);
  Table child_table(16 - iteration, output_path.c_str(), reachability_bits_per_entry, configuration_count);
  const ZDD parent_zdd(viewpoint, previous_iteration);
  const ZDD child_zdd(viewpoint, iteration);
  Posi position;
  unsigned long long newly_reachable_count = 0;
  
  // Apply legal actions to every board compatible with each reachable parent.
  for(unsigned long long parent_id = 0; parent_id < parent_count; ++parent_id) {
    if(parent_table.get(parent_id) == unreachable) continue;
    
    position.make_posi(parent_id, parent_zdd);
    Action actions[1000] = {};
    unsigned char compatible_boards[12][46] = {};
    const int action_count = position.compute_actions(actions, viewpoint, previous_iteration);
    const int belief_state_size = position.getunknowninfo(compatible_boards, viewpoint, previous_iteration);
    assert(belief_state_size > 0);
    
    for(int action_index = 0; action_index < action_count; ++action_index) {
      for(int board_index = 0; board_index < belief_state_size; ++board_index) {
	position.make_posi_n(compatible_boards, board_index);
	position.make_action(actions[action_index], viewpoint, previous_iteration);
	position.make_posi_myself();
	const auto child_id = position.getzddnum(child_zdd);
	if(child_table.get(child_id) == unreachable) {
	  child_table.set(child_id, reachable);
	  ++newly_reachable_count;
	}
      }
      position.make_posi(parent_id, parent_zdd);
    }
    if(parent_id % 100000000 == 0) {
      std::cout << "[progress] Parent ID: " << parent_id << ", newly reachable configurations: " << newly_reachable_count << '\n';
    }
  }
  
  // Count every set bit while writing, including those loaded from an existing table.
  std::cout << "[output] Writing database: " << output_path << '\n';
  OutTable writer(16 - iteration, output_path, configuration_count, reachability_bits_per_entry);
  unsigned long long reachable_count = 0;
  for(unsigned long long id = 0; id < configuration_count; ++id) {
    const auto label = child_table.get(id);
    writer.write(label);
    if(label == reachable) ++ reachable_count;
  }
  writer.flush();
  print_summary(reachable_count, configuration_count);
}
