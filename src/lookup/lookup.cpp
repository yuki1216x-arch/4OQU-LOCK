#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

namespace {
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

  // Decode the two reserve rows and four board rows into the ZDD's object order.
  std::array<unsigned char, 32> parse_position(const std::string& notation) {
    std::array<unsigned char, 32> objects{};
    std::istringstream input(notation);
    std::string row;
    std::size_t offset = 0;
    for(int row_index = 0; row_index < 6; ++row_index) {
      if(!std::getline(input, row, '/')) throw std::runtime_error("Expected six position rows.");
      const bool reserve = row_index == 0 || row_index == 5;
      const auto row_end = offset + (reserve ? 8 : 4);
      for(std::size_t i = 0; i < row.size(); ++i) {
	const char piece = row[i];
	if(offset >= row_end) throw std::runtime_error("Position row is too wide.");
	if(reserve) {
	  const auto code = std::string(".WBU").find(piece);
	  if(code == std::string::npos) throw std::runtime_error("Invalid reserve piece.");
	  objects[offset++] = code;
	} else if(piece >= '1' && piece <= '4') {
	  offset += piece - '0';
	} else {
	  if(i + 1 == row.size()) throw std::runtime_error("Incomplete board cell.");
	  const auto cell = row.substr(i, 2);
	  static const std::array<std::string, 6> cells{{"W.", "B.", "WW", "WB", "BW", "BB"}};
	  std::size_t code = 0;
	  while(code < cells.size() && cells[code] != cell) ++code;
	  if(code == cells.size()) throw std::runtime_error("Invalid board cell.");
	  objects[offset++] = code + 1;
	  ++i;
	}
      }
      if(offset != row_end) throw std::runtime_error("Incorrect position row width.");
    }
    if(input.peek() != std::char_traits<char>::eof() || notation.back() == '/')
      throw std::runtime_error("Expected exactly six position rows.");
    return objects;
  }

  unsigned read_label(const std::string& path, int iteration, std::uint64_t configuration_id,
		      std::uint64_t configuration_count) {
    std::ifstream file(path, std::ios::binary);
    std::array<unsigned char, 8> header{};
    if(!file.read(reinterpret_cast<char*>(header.data()), header.size()))
      throw std::runtime_error("Cannot read database header: " + path);
    std::uint64_t stored_count = 0;
    for(int i = 7; i >= 2; --i) stored_count = (stored_count << 8) | header[i];
    if(header[0] != 0 || header[1] != 16 - iteration || stored_count != configuration_count)
      throw std::runtime_error("Database header does not match the selected table: " + path);

    // Read the requested entry after the header, then remove the game-length bits.
    const bool packed_nibbles = iteration > 12;
    file.seekg(header.size() + (packed_nibbles ? configuration_id / 2 : configuration_id));
    const int byte = file.get();
    if(byte == std::char_traits<char>::eof())
      throw std::runtime_error("Cannot read database entry: " + path);
    return packed_nibbles ? (byte >> (4 * (configuration_id % 2))) & 3 : byte & 7;
  }
}   // namespace

int main(int argc, char* argv[]) {
  if(argc != 4 || (std::string(argv[2]) != "0" && std::string(argv[2]) != "1")) {
    std::cerr << "Usage: " << argv[0] << " ITERATION VIEWPOINT POSITION (0: White, 1: Black)\n";
    return 1;
  }
  char* end = nullptr;
  const long iteration_arg = std::strtol(argv[1], &end, 10);
  if(end == argv[1] || *end || iteration_arg < 0 || iteration_arg > 15) {
    std::cerr << "Iteration must be between 0 and 15.\n";
    return 1;
  }
  const int iteration = static_cast<int>(iteration_arg);
  int viewpoint = argv[2][0] - '0';
  try {
    auto objects = parse_position(argv[3]);
    const ZDD zdd(viewpoint, iteration);
    const auto configuration_id = zdd.compute_id(objects.data(), objects.size());
    const auto configuration_count = configuration_counts[viewpoint][16 - iteration];
    if(configuration_id >= configuration_count)
      throw std::runtime_error("Position is outside the selected table.");
    // Reject a well-formed position that is absent from the selected ZDD.
    std::array<unsigned char, 32> decode{};
    zdd.compute_array(configuration_id, decode.data(), decode.size());
    if(decode != objects) throw std::runtime_error("Position is outside the selected table.");
    const std::string database_path = std::string("data/db/") +
      (viewpoint == 0 ? "white_table_" : "black_table_") + std::to_string(iteration) + ".bin";
    const auto label = read_label(database_path, iteration, configuration_id, configuration_count);
    static const std::array<const char*, 6> label_names{{"unknown", "win", "loss", "draw", "no loss", "no win"}};
    if(label >= label_names.size()) throw std::runtime_error("Invalid outcome label.");

    std::cout << "ID of this configuration: " << configuration_id << "\n";
    Posi position;
    position.make_posi(configuration_id, zdd);
    position.print();
    std::cout << "label index = " << label << ", label: " << label_names[label] << '\n';
  } catch (const std::exception& error) {
    std::cerr << "[error] " << error.what() << '\n';
    return 1;
  }
}
