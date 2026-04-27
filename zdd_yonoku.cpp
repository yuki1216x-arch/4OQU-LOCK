#include "node.hpp"
#include "zdd_yonoku.hpp"

//駒の状態を表す構造体(15種類)
struct LocInfo {
    char cube[2];
    unsigned char height;   //高さ
    unsigned char nb;       //黒の数
    unsigned char nw;       //白の数
    unsigned char uk;
    char top;               //一番上の駒の種類

    //与えられたオブジェクトが自身と同じかどうかを判定する関数
    bool equal(const LocInfo &o) const noexcept {
        return (cube[0] == o.cube[0] && cube[1] == o.cube[1] && /*cube[2] == o.cube[2]  &&*/ height == o.height);
    }
};

constexpr LocInfo tbl_objid2locinfo[8] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'W', 'W'}, 2, 0, 2, 0, 'W'}, //3
     { {'W', 'B'}, 2, 1, 1, 0, 'B'}, //4
     { {'B', 'W'}, 2, 1, 1, 0, 'W'}, //5
     { {'B', 'B'}, 2, 2, 0, 0, 'B'}, //6
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //7  
};

constexpr LocInfo tbl_objid2locinfo2[5] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'U', '.'}, 1, 0, 0, 1, 'U'}, //3
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //4  
};

//zddを作る関数
void ZDD::construct_zdd(int vision, int nmove) noexcept {
  unique_ptr<Node_base> root;   // 根節点

  int d = -1;                 //深さ
  if(vision == 0) root = std::make_unique<Node_White>();
  else root = std::make_unique<Node_Black>();

  Node_base* n;

  m_N[0].push_back(std::move(root));   //m_N[0]に根節点を入れる
  for(int i = 0; i < 32; i++) {    //各マスに対するループ(30マス分)(俺は16マス分)
    if(i < 8) { //黒プレイヤの手ゴマ
      for(int j = 0; j < 3; j++) {    //各オブジェクトに対するループ(15種類)(俺は7種類)
	d++;                        //深さを+1
	for(size_t k = 0; k < m_N[d].size(); k++) {    //深さdの節点集合に対するループ
	  n = m_N[d][k].get();            //要素(節点)を一つ取り出す
	  for(int x = 0; x < 2; x++) {     //0枝、1枝のループ
	    if(n->IsNextLeaf0(d, x, nmove)) { //もし節点nからx-枝に進んだときに0-葉なら
	      if(x == 0) n->m_left = l0.get();
	      else n->m_right = l0.get();
	    } else {                    //子供に節点をもつ時
	      std::unique_ptr<Node_base> c;   //子どもの初期化
	      if(vision == 0) c = std::make_unique<Node_White>(*n, x);
	      else c = std::make_unique<Node_Black>(*n, x);
	      
	      if(m_N[d+1].size() == 0) {     //次の深さの節点集合がまだないとき                   
		if(x == 0) n->m_left = c.get(); //親と子をつなぐ
		else n->m_right = c.get();
		m_N[d+1].push_back(std::move(c));
	      } else {  //2個目以降の節点の時
		for(size_t l = 0; l < m_N[d+1].size(); l++) {                
		  if(m_N[d+1][l]->IsEquivalentVisited_opponent_hand(*c)) {
		    if(x == 0) n->m_left = m_N[d+1][l].get();
		    else n->m_right = m_N[d+1][l].get();
		    break;
		  } else if(l == m_N[d+1].size() - 1) {
		    if(x == 0) n->m_left = c.get();
		    else n->m_right = c.get();
		    m_N[d+1].push_back(std::move(c));
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    } else if(i < 24) {        //以下盤面について
      for(int j = 0; j < 7; j++) {    //各オブジェクトに対するループ(15種類)(俺は7種類)
	d++;                       //深さを+1
	for(size_t k = 0; k < m_N[d].size(); k++) {    //深さdの節点集合に対するループ
	  n = m_N[d][k].get();            //要素(節点)を一つ取り出す
	  for(int x = 0; x < 2; x++) {     //0枝、1枝のループ
	    if(n->IsNextLeaf0(d, x, nmove)) { //もし節点nからx-枝に進んだときに0-葉なら
	      if(x == 0) n->m_left = l0.get();
	      else n->m_right = l0.get();
	    } else {                    //子供に節点をもつ時
	      std::unique_ptr<Node_base> c;   //子どもの初期化
	      if(vision == 0) c = std::make_unique<Node_White>(*n, x);
	      else c = std::make_unique<Node_Black>(*n, x);
	      
	      //ここまでやったpart3
	      if(m_N[d+1].size() == 0) {     //次の深さの節点集合がまだないとき
		if(x == 0) n->m_left = c.get(); //親と子をつなぐ
		else n->m_right = c.get();
		m_N[d+1].push_back(std::move(c));    //次の深さの節点集合に追加
	      } else {  //2個目以降の節点の時
		for(size_t l = 0; l < m_N[d+1].size(); l++) {
		  //以下if文内で等価節点があったら(条件は論文内)
		  if(m_N[d+1][l]->IsEquivalentVisited_board(*c)) {
		    if(x == 0) n->m_left = m_N[d+1][l].get();
		    else n->m_right = m_N[d+1][l].get();
		    break;
		  } else if(l == m_N[d+1].size() - 1) {
		    if(x == 0) n->m_left = c.get();
		    else n->m_right = c.get();
		    m_N[d+1].push_back(std::move(c));
		    break;
		  }
		}
	      }
	    }
	  }
	}
      }
    } else {
      for(int j = 0; j < 4; j++) {    //各オブジェクトに対するループ(15種類)(俺は7種類)
	d++;                       //深さを+1
	for(size_t k = 0; k < m_N[d].size(); k++) {    //深さdの節点集合に対するループ
	  n = m_N[d][k].get();            //要素(節点)を一つ取り出す
	  for(int x = 0; x < 2; x++) {     //0枝、1枝のループ
	    if(n->IsNextLeaf0(d, x, nmove)) { //もし節点nからx-枝に進んだときに0-葉なら
	      if(x == 0) n->m_left = l0.get();
	      else n->m_right = l0.get();
	    } else if(d == 167) {       //深さが最大の時は1葉
	      if(x == 0) n->m_left = l1.get();
	      else n->m_right = l1.get();
	    } else {                    //子供に節点をもつ時
	      std::unique_ptr<Node_base> c;   //子どもの初期化
	      if(vision == 0) c = std::make_unique<Node_White>(*n, x);
	      else c = std::make_unique<Node_Black>(*n, x);
	      
	      if(m_N[d+1].size() == 0) {     //次の深さの節点集合がまだないとき
		if(x == 0) n->m_left = c.get(); //親と子をつなぐ
		else n->m_right = c.get();
		m_N[d+1].push_back(std::move(c));    //次の深さの節点集合に追加
	      } else {  //2個目以降の節点の時
		for(size_t l = 0; l < m_N[d+1].size(); l++) {
		  //以下if文内で等価節点があったら(条件は論文内)
		  if(m_N[d+1][l]->IsEquivalentVisited_player_hand(*c)) {
		    if(x == 0) n->m_left = m_N[d+1][l].get();
		    else n->m_right = m_N[d+1][l].get();
		    break;
		  } else if(l == m_N[d+1].size() - 1) {
		    if(x == 0) n->m_left = c.get();
		    else n->m_right = c.get();
		    m_N[d+1].push_back(std::move(c));
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

  int end = 1;
  //冗長節点の削除

  while(end == 1) {
    end = 0;
    for(int i = 0; i < 167; i++){
      for(size_t j = 0; j < m_N[i].size(); j++){
	//cout << end << endl;
	if(!m_N[i][j]->m_left->IsLeaf0() && !m_N[i][j]->m_left->IsLeaf1() && !m_N[i][j]->IsRedundantNode()){
	  if(m_N[i][j]->m_left->m_right->IsLeaf0()){
	    end = 1;
	    m_N[i][j]->m_left->setRedundantNode();
	    m_N[i][j]->m_left = m_N[i][j]->m_left->m_left;
	  }
	}
	//cout << end << endl;
	if(!m_N[i][j]->m_right->IsLeaf0() && !m_N[i][j]->m_right->IsLeaf1() && !m_N[i][j]->IsRedundantNode()){
	  if(m_N[i][j]->m_right->m_right->IsLeaf0()){
	    end = 1;
	    m_N[i][j]->m_right->setRedundantNode();
	    m_N[i][j]->m_right = m_N[i][j]->m_right->m_left;
	  }
	}
      }
    }
  }

  for(int i = 0; i < 168; i++){
    for(size_t j = 0; j < m_N[i].size(); j++){
      if(m_N[i][j]->IsRedundantNode()){
	m_N[i].erase(m_N[i].begin() + j);
	j--;
      }
    }
  }

  m_N[0][0]->dfs();

  cout << "root->num = " << m_N[0][0]->get_num() << endl;
  cout << "root->lengthmax = " << m_N[0][0]->get_lengthmax() << endl;
  cout << "root->lengthmin = " << m_N[0][0]->get_lengthmin() << endl;
  cout << "root->lengthave = " << (double)m_N[0][0]->get_lengthsum() / (double)m_N[0][0]->get_num() << endl;
  
  int sum = 0;
  
  for(int i = 0; i < 168; i++){
    //cout << "N[" << i << "]" << N[i].size() << endl;
    sum += m_N[i].size();
  }
  cout << "sumad = " << sum << endl;
}
