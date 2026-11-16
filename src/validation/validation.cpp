// Validate the ZDD configuration order and ID round trip and validate parent-child outcome labels.
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "node.hpp"
#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

namespace {
  constexpr int terminal_depth = 168;
  constexpr std::size_t object_count = 32;
  constexpr unsigned long long progress_interval = 100000000ULL;

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

  using ConfigurationId = unsigned long long;
  using ObjectIds = std::vector<unsigned char>;

  bool read_arguments(int argc, char* argv[], int& iteration, int& viewpoint) {
    if(argc < 3 || argc > 4 || (argc == 4 && std::string(argv[3]) != "--labels-only")  || (std::string(argv[2]) != "0" && std::string(argv[2]) != "1")) {
      std::cerr << "Usage: " << argv[0] << " ITERATION VIEWPOINT [--labels-only] (0: White; 1: Black)\n";
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

  bool validate_configuration(ObjectIds& enumerated_objects, const ZDD& zdd, ConfigurationId configuration_id) {
    if(enumerated_objects.size() != object_count) {
      std::cerr << "[error] Unexpected object count for configuration "
		<< configuration_id << ": " << enumerated_objects.size() << '\n';
      return false;
    }

    std::array<unsigned char, object_count> decoded_objects{};
    zdd.compute_array(configuration_id, decoded_objects.data(), decoded_objects.size());
    for(std::size_t object_index = 0; object_index < object_count; ++object_index) {
      if(enumerated_objects[object_index] != decoded_objects[object_index]) {
	std::cerr << "[error] ZDD decoding mismatch for configuration " << configuration_id
		  << ", object index " << object_index
		  << ": expected " << +enumerated_objects[object_index]
		  << ", got " << +decoded_objects[object_index] << '\n';
	return false;
      }
    }

    const auto encoded_id = zdd.compute_id(enumerated_objects.data(), enumerated_objects.size());
    if(encoded_id != configuration_id) {
      std::cerr << "[error] ZDD encoding mismatch: expected ID " << configuration_id
		<< ", got " << encoded_id << '\n';
      return false;
    }

    if(configuration_id % progress_interval == 0) {
      std::cout << "[progress] Configuration " << configuration_id << " verified: ";
      for(unsigned char object_id : enumerated_objects) std::cout << +object_id;
      std::cout << std::endl;
    }
    return true;
  }

  bool validate_subtree(Node_base& node, ObjectIds& enumerated_objects, const ZDD& zdd, int depth, ConfigurationId& next_configuration_id, int iteration, int viewpoint) {
    if(depth == terminal_depth) {
      if(!validate_configuration(enumerated_objects, zdd, next_configuration_id)) return false;
      ++next_configuration_id;
      return true;
    }

    // Visit branch 0 before branch 1 to match the ZDD's configuration order.
    for(int branch = 0; branch < 2; ++branch) {
      if(node.IsNextLeaf0(depth, branch, iteration)) continue;

      std::unique_ptr<Node_base> child;
      if(viewpoint == 0) child = std::make_unique<Node_White>(node, branch);
      else child = std::make_unique<Node_Black>(node, branch);

      // Only branch 1 selects the current object's state.
      if(branch == 1) enumerated_objects.push_back(node.get_loc_stateid());
      const bool valid = validate_subtree(*child, enumerated_objects, zdd, depth + 1, next_configuration_id, iteration, viewpoint);
      if(branch == 1) enumerated_objects.pop_back();
      if(!valid) return false;
    }
    return true;
  }
  using CompatibleBoards = unsigned char[12][46];

  struct ChildLayer {
    std::unique_ptr<ZDD> zdd;
    std::unique_ptr<Table> table;
    ConfigurationId configuration_count = 0;
  };

  std::unique_ptr<Table> load_result_table(int iteration, int viewpoint) {
    const std::string path = std::string("data/db/") + (viewpoint == 0 ? "white_table_" : "black_table_") + std::to_string(iteration) + ".bin";
    const ConfigurationId count = configuration_counts[viewpoint][16 - iteration];
    const std::size_t bits_per_entry = iteration > 12 ? 4 : 8;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if(!input) throw std::runtime_error("Cannot open result database: " + path);
    const auto expected_size = static_cast<std::streamoff>(8 + (count * bits_per_entry + 7) / 8);
    if(input.tellg() != expected_size)
      throw std::runtime_error("Unexpected result database size: " + path);

    input.seekg(0);
    std::array<unsigned char, 8> header{};
    if(!input.read(reinterpret_cast<char*>(header.data()), header.size()))
      throw std::runtime_error("Cannot read result database header: " + path);
    ConfigurationId stored_count = 0;
    for(int i = 7; i >= 2; --i) stored_count = (stored_count << 8) | header[i];
    if(header[0] != 0 || header[1] != 16 - iteration || stored_count != count)
      throw std::runtime_error("Incorrect result database header: " + path);

    return std::make_unique<Table>(16 - iteration, path.c_str(), bits_per_entry, count);
  }

  unsigned flip_label(unsigned label) {
    switch (label) {
    case v_win: return v_lose;
    case v_lose: return v_win;
    case v_notwin: return v_notlose;
    case v_notlose: return v_notwin;
    default: return label;
    }
  }

  unsigned evaluate_child(Posi position, const Action& action, int iteration, int source_viewpoint, int child_viewpoint, const ChildLayer& child) {
    const int result = position.make_action(action, source_viewpoint, iteration);
    if(result == 2) return v_win;
    if(result == 3) return v_lose;
    if(result == 10) return v_draw;
    if(result != 0 || !child.table)
      throw std::runtime_error("Unexpected action result when evaluating a child");

    if(source_viewpoint == child_viewpoint) position.make_posi_myself();
    else position.make_posi_opponent();
    const auto child_id = position.getzddnum(*child.zdd);
    if(child_id >= child.configuration_count)
      throw std::runtime_error("Child ID outside the result table: " + std::to_string(child_id));

    // get_value() extracts the outcome without the stored game length.
    const unsigned label = child.table->get_value(child_id);
    if(label > v_notwin) throw std::runtime_error("Invalid child outcome label");
    return source_viewpoint == child_viewpoint ? label : flip_label(label);
  }

  // A plan must satisfy the requested outcome for every compatible board.
  // On the viewpoint's turn one qualifying action is enough; otherwise all are required.
  bool supports_outcome(Posi position, int iteration, int viewpoint, int child_viewpoint,
			const ChildLayer& child, bool require_win) {
    Action actions[1000]{};
    const int action_count = position.compute_actions(actions, viewpoint, iteration);
    if(action_count == -1) return true;
    if(action_count == -2) return false;
    if(action_count <= 0 || action_count > 1000)
      throw std::runtime_error("Invalid legal action count");

    CompatibleBoards boards{};
    const int board_count = position.getunknowninfo(boards, viewpoint, iteration);
    if(board_count <= 0 || board_count > 12)
      throw std::runtime_error("Invalid compatible board count");

    const bool viewpoint_to_move = iteration % 2 == viewpoint;
    for(int action_index = 0; action_index < action_count; ++action_index) {
      bool supported = true;
      for(int board_index = 0; board_index < board_count; ++board_index) {
	position.make_posi_n(boards, board_index);
	const unsigned label = evaluate_child(position, actions[action_index], iteration, viewpoint, child_viewpoint, child);
	if(label != v_win && (require_win || (label != v_notlose && label != v_draw))) {
	  supported = false;
	  break;
	}
      }
      if(supported == viewpoint_to_move) return supported;
    }
    return !viewpoint_to_move;
  }

  bool supports_opponent_outcome(Posi position, int iteration, int viewpoint, int child_viewpoint, const ChildLayer& child, bool require_win) {
    CompatibleBoards boards{};
    const int board_count = position.getunknowninfo(boards, viewpoint, iteration);
    if(board_count <= 0 || board_count > 12)
      throw std::runtime_error("Invalid compatible board count");

    // Check each compatible board from the opponent's information perspective.
    for(int board_index = 0; board_index < board_count; ++board_index) {
      position.make_posi_n(boards, board_index);
      position.make_posi_opponent();
      if(!supports_outcome(position, iteration, 1 - viewpoint, child_viewpoint, child, require_win))
	return false;
    }
    return true;
  }

  bool validate_labels(int iteration, int viewpoint, const ZDD& parent_zdd) {
    const auto parent = load_result_table(iteration, viewpoint);
    std::array<ChildLayer, 2> children;
    // At turn 15, actions end the game; no turn-16 table is required.
    if(iteration < 15) {
      for(int view = 0; view < 2; ++view) children[view].table = load_result_table(iteration + 1, view);
      for(int view = 0; view < 2; ++view) {
	children[view].configuration_count = configuration_counts[view][15 - iteration];
	children[view].zdd = std::make_unique<ZDD>(view, iteration + 1);
      }
    }

    ConfigurationId checked_count = 0;
    ConfigurationId unknown_count = 0;
    const auto count = configuration_counts[viewpoint][16 - iteration];
    for(ConfigurationId id = 0; id < count; ++id) {
      const unsigned label = parent->get_value(id);
      Posi position;
      position.make_posi(id, parent_zdd);
      const int terminal = position.TerminalTest(viewpoint, iteration);
      bool valid = false;
      try {
	if(terminal == 2 || terminal == 3) {
	  valid = label == (terminal == 2 ? v_win : v_lose);
	} else if(label == v_unknown) {
	  ++unknown_count;
	  continue;
	} else if(label <= v_notwin) {
	  bool supports_self = false;
	  bool supports_opponent = false;
	  // A proof may use either child table; each proof uses one viewpoint throughout.
	  for(int child_view = 0; child_view < 2; ++child_view) {
	    if(!supports_self && (label == v_win || label == v_notlose || label == v_draw))
	      supports_self = supports_outcome(position, iteration, viewpoint, child_view, children[child_view], label == v_win);
	    if(!supports_opponent && (label == v_lose || label == v_notwin || label == v_draw))
	      supports_opponent = supports_opponent_outcome(position, iteration, viewpoint, child_view, children[child_view], label == v_lose);
	  }
	  valid = label == v_draw ? supports_self && supports_opponent : supports_self || supports_opponent;
	}
      } catch (const std::runtime_error& error) {
	std::cerr << "[error] Configuration " << id << ": " << error.what() << '\n';
	return false;
      }
      if(!valid) {
	std::cerr << "[error] Unsupported parent label " << label << " at configuration " << id
		  << " (iteration " << iteration << ", viewpoint " << viewpoint << ")\n";
	return false;
      }
      ++checked_count;
      if(id % progress_interval == 0)
	std::cout << "[progress] Parent label verified: " << id << std::endl;
    }
    std::cout << "Parent-child label validation passed: " << checked_count
	      << " checked, " << unknown_count << " unknown skipped.\n";
    return true;
  }
}   // namespace

int main(int argc, char* argv[]) {
  int iteration, viewpoint;
  if(!read_arguments(argc, argv, iteration, viewpoint)) return 1;

  try {
    const ZDD zdd(viewpoint, iteration);
    if(argc != 4) {
      std::unique_ptr<Node_base> root;
      if(viewpoint == 0) root = std::make_unique<Node_White>();
      else root = std::make_unique<Node_Black>();
  
      ObjectIds enumerated_objects;
      enumerated_objects.reserve(object_count);
      ConfigurationId verified_count = 0;
      if(!validate_subtree(*root, enumerated_objects, zdd, 0, verified_count, iteration, viewpoint)) return 1;

      std::cout << "ZDD validation passed: iteration " << iteration
		<< ", viewpoint " << (viewpoint == 0 ? "White" : "Black")
		<< ", configurations " << verified_count << '\n';
    }
    return validate_labels(iteration, viewpoint, zdd) ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "[error] " << error.what() << '\n';
    return 1;
  }
}
