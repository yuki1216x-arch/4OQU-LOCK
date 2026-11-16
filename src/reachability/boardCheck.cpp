#include <cstdlib>
#include <fstream>
#include <iostream>
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

  std::string reachability_database_path(int viewpoint, int iteration) {
    const char* prefix = viewpoint == 0 ? "white_table_" : "black_table_";
    return std::string("data/db_reachability/reachability_") + prefix + std::to_string(iteration) + ".bin";
  }

  bool read_arguments(int argc, char * argv[], int& iteration, int& viewpoint) {
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
}   // namespace

int main(int argc, char* argv[]) {
  int iteration, viewpoint;
  if(!read_arguments(argc, argv, iteration, viewpoint)) return 1;
  
  const auto database_path = reachability_database_path(viewpoint, iteration);
  if(!can_read_database(database_path)) return 1;

  const auto configuration_count = configuration_counts[viewpoint][16 - iteration];
  const Table reachability_table(16 - iteration, database_path.c_str(), reachability_bits_per_entry, configuration_count);
  const ZDD zdd(viewpoint, iteration);
  Posi position;
  unsigned long long unreachable_count = 0;

  // Print each configuration marked unreachable in the selected table.
  for(unsigned long long configuration_id = 0; configuration_id < configuration_count; ++configuration_id) {
    if(reachability_table.get(configuration_id) != unreachable) continue;
    std::cout << unreachable_count++ << '\n';
    position.make_posi(configuration_id, zdd);
    position.print();
  }
  std::cout << "Unreachable configuration: " << unreachable_count << '\n';
}
