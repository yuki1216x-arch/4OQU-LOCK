#include "zdd_yonoku.hpp"

void ZDD::build_diagram(int viewpoint, int iteration) noexcept {
  unique_ptr<ZddNode> root;
  int depth = -1;
  if(viewpoint == 0) root = std::make_unique<WhiteViewNode>();
  else root = std::make_unique<BlackViewNode>();

  ZddNode* node;
  m_nodes_by_depth[0].push_back(std::move(root));
  // Visit the 8 opponent hand slots, 16 board cells, then 8 player hand slots.
  for(int i = 0; i < 32; i++) {
    if(i < 8) {
      for(int j = 0; j < 3; j++) {
	depth++;
	for(size_t k = 0; k < m_nodes_by_depth[depth].size(); k++) {
	  node = m_nodes_by_depth[depth][k].get();
	  for(int branch = 0; branch < 2; branch++) {
	    if(node->branch_leads_to_zero_terminal(depth, branch, iteration)) {
	      if(branch == 0) node->m_zero_child = m_zero_terminal.get();
	      else node->m_one_child = m_zero_terminal.get();
	    } else {
	      std::unique_ptr<ZddNode> child;
	      if(viewpoint == 0) child = std::make_unique<WhiteViewNode>(*node, branch);
	      else child = std::make_unique<BlackViewNode>(*node, branch);
	      
	      if(m_nodes_by_depth[depth + 1].size() == 0) {
		if(branch == 0) node->m_zero_child = child.get();
		else node->m_one_child = child.get();
		m_nodes_by_depth[depth + 1].push_back(std::move(child));
	      } else {
		for(size_t l = 0; l < m_nodes_by_depth[depth + 1].size(); l++) {
		  if(m_nodes_by_depth[depth + 1][l]->has_equivalent_opponent_hand_state(*child)) {
		    if(branch == 0) node->m_zero_child = m_nodes_by_depth[depth + 1][l].get();
		    else node->m_one_child = m_nodes_by_depth[depth + 1][l].get();
		    break;
		  } else if(l == m_nodes_by_depth[depth + 1].size() - 1) {
		    if(branch == 0) node->m_zero_child = child.get();
		    else node->m_one_child = child.get();
		    m_nodes_by_depth[depth + 1].push_back(std::move(child));
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    } else if(i < 24) {
      for(int j = 0; j < 7; j++) {
	depth++;
	for(size_t k = 0; k < m_nodes_by_depth[depth].size(); k++) {
	  node = m_nodes_by_depth[depth][k].get();
	  for(int branch = 0; branch < 2; branch++) {
	    if(node->branch_leads_to_zero_terminal(depth, branch, iteration)) {
	      if(branch == 0) node->m_zero_child = m_zero_terminal.get();
	      else node->m_one_child = m_zero_terminal.get();
	    } else {
	      std::unique_ptr<ZddNode> child;
	      if(viewpoint == 0) child = std::make_unique<WhiteViewNode>(*node, branch);
	      else child = std::make_unique<BlackViewNode>(*node, branch);
	      
	      if(m_nodes_by_depth[depth + 1].size() == 0) {
		if(branch == 0) node->m_zero_child = child.get();
		else node->m_one_child = child.get();
		m_nodes_by_depth[depth + 1].push_back(std::move(child));
	      } else {
		for(size_t l = 0; l < m_nodes_by_depth[depth + 1].size(); l++) {
		  if(m_nodes_by_depth[depth + 1][l]->has_equivalent_board_state(*child)) {
		    if(branch == 0) node->m_zero_child = m_nodes_by_depth[depth + 1][l].get();
		    else node->m_one_child = m_nodes_by_depth[depth + 1][l].get();
		    break;
		  } else if(l == m_nodes_by_depth[depth + 1].size() - 1) {
		    if(branch == 0) node->m_zero_child = child.get();
		    else node->m_one_child = child.get();
		    m_nodes_by_depth[depth + 1].push_back(std::move(child));
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    } else {
      for(int j = 0; j < 4; j++) {
	depth++;
	for(size_t k = 0; k < m_nodes_by_depth[depth].size(); k++) {
	  node = m_nodes_by_depth[depth][k].get();
	  for(int branch = 0; branch < 2; branch++) {
	    if(node->branch_leads_to_zero_terminal(depth, branch, iteration)) {
	      if(branch == 0) node->m_zero_child = m_zero_terminal.get();
	      else node->m_one_child = m_zero_terminal.get();
	    } else if(depth == 167) {
	      if(branch == 0) node->m_zero_child = m_one_terminal.get();
	      else node->m_one_child = m_one_terminal.get();
	    } else {
	      std::unique_ptr<ZddNode> child;
	      if(viewpoint == 0) child = std::make_unique<WhiteViewNode>(*node, branch);
	      else child = std::make_unique<BlackViewNode>(*node, branch);
	      
	      if(m_nodes_by_depth[depth + 1].size() == 0) {
		if(branch == 0) node->m_zero_child = child.get();
		else node->m_one_child = child.get();
		m_nodes_by_depth[depth + 1].push_back(std::move(child));
	      } else {
		for(size_t l = 0; l < m_nodes_by_depth[depth + 1].size(); l++) {
		  if(m_nodes_by_depth[depth + 1][l]->has_equivalent_player_hand_state(*child)) {
		    if(branch == 0) node->m_zero_child = m_nodes_by_depth[depth + 1][l].get();
		    else node->m_one_child = m_nodes_by_depth[depth + 1][l].get();
		    break;
		  } else if(l == m_nodes_by_depth[depth + 1].size() - 1) {
		    if(branch == 0) node->m_zero_child = child.get();
		    else node->m_one_child = child.get();
		    m_nodes_by_depth[depth + 1].push_back(std::move(child));
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    } 
  }

  int reduction_changed = 1;

  // Bypass nodes whose one-branch reaches the zero terminal.
  while(reduction_changed == 1) {
    reduction_changed = 0;
    for(int i = 0; i < 167; i++){
      for(size_t j = 0; j < m_nodes_by_depth[i].size(); j++){
	if(!m_nodes_by_depth[i][j]->m_zero_child->is_zero_terminal() && !m_nodes_by_depth[i][j]->m_zero_child->is_one_terminal() && !m_nodes_by_depth[i][j]->is_redundant()){
	  if(m_nodes_by_depth[i][j]->m_zero_child->m_one_child->is_zero_terminal()){
	    reduction_changed = 1;
	    m_nodes_by_depth[i][j]->m_zero_child->mark_redundant();
	    m_nodes_by_depth[i][j]->m_zero_child = m_nodes_by_depth[i][j]->m_zero_child->m_zero_child;
	  }
	}
	if(!m_nodes_by_depth[i][j]->m_one_child->is_zero_terminal() && !m_nodes_by_depth[i][j]->m_one_child->is_one_terminal() && !m_nodes_by_depth[i][j]->is_redundant()){
	  if(m_nodes_by_depth[i][j]->m_one_child->m_one_child->is_zero_terminal()){
	    reduction_changed = 1;
	    m_nodes_by_depth[i][j]->m_one_child->mark_redundant();
	    m_nodes_by_depth[i][j]->m_one_child = m_nodes_by_depth[i][j]->m_one_child->m_zero_child;
	  }
	}
      }
    }
  }

  // Remove nodes marked redundant after all pointers have been redirected.
  for(int i = 0; i < 168; i++){
    for(size_t j = 0; j < m_nodes_by_depth[i].size(); j++){
      if(m_nodes_by_depth[i][j]->is_redundant()){
	m_nodes_by_depth[i].erase(m_nodes_by_depth[i].begin() + j);
	j--;
      }
    }
  }

  m_nodes_by_depth[0][0]->compute_subtree_statistics();

  print_statistics();
}
