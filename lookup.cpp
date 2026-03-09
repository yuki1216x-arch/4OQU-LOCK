#include <memory>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::mt19937_64;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::to_string;

std::vector<string> split(const string text, const char delimiter='/') {
  std::vector<string> columns;

  if (text.empty()) {
    return columns;
  }

  std::stringstream stream{text};
  string buff;
  while (getline(stream, buff, delimiter)) {
    columns.push_back(buff);
  }
  return columns;
}

/*struct LocInfo {
  unsigned int height;
  char cube[2];   //unsigned int idx;
  };*/

constexpr unsigned long long int placement_count[2][17] {
  {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
      19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
      144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
      666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
	19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
	144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
	666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL}
};

constexpr LocInfo tbl_objid2locinfo[7] = {
					  { 0, {'.', '.'} },
					  { 1, {'W', '.'} },
					  { 1, {'B', '.'} },
					  { 2, {'W', 'W'} },
					  { 2, {'W', 'B'} },
					  { 2, {'B', 'W'} },
					  { 2, {'B', 'B'} }
};

constexpr LocInfo tbl_objid2locinfo2[4] = {
					   { 0, {'.', '.'} },
					   { 1, {'W', '.'} },
					   { 1, {'B', '.'} },
					   { 1, {'U', '.'} }
};

const string base[2] = {"white_table", "black_table"};

unsigned long long int getzddnum(const ZDD_base& zdd, string fen_str) noexcept {
  unsigned char array_objid[32] = {};
  std::vector<string> cols = split(fen_str);
  int array_objid_iter = 0;
  assert(cols.size() == 6);

  for (std::size_t i = 0; i < cols.size(); i++) {
    for (std::size_t j = 0; j < cols[i].size(); j++) {
      if (i == 0 || i == 5) {
	assert((array_objid_iter >= 0 && array_objid_iter <= 7) || (array_objid_iter >= 24 && array_objid_iter <= 31));
	if (cols[i][j] == tbl_objid2locinfo2[0].cube[0]) array_objid[array_objid_iter] = 0;
	else if (cols[i][j] == tbl_objid2locinfo2[1].cube[0]) array_objid[array_objid_iter] = 1;
	else if (cols[i][j] == tbl_objid2locinfo2[2].cube[0]) array_objid[array_objid_iter] = 2;
	else {
	  assert(cols[i][j] == tbl_objid2locinfo2[3].cube[0]);
	  array_objid[array_objid_iter] = 3;
	}
	array_objid_iter++;
      } else {
	assert(i >= 1 && i <= 4);
	assert(array_objid_iter >= 8 && array_objid_iter <= 23);
	if (cols[i][j] != tbl_objid2locinfo[1].cube[0] && cols[i][j] != tbl_objid2locinfo[2].cube[0]) {
	  int check = cols[i][j] - '0';
	  assert(check <= 4 - array_objid_iter % 4);
	  for (int k = cols[i][j] - '0'; k > 0; k--) {
	    array_objid[array_objid_iter] = 0;
	    array_objid_iter++;
	  }
	} else {
	  assert(cols[i].size() > j+1);
	  if (cols[i][j] == tbl_objid2locinfo[1].cube[0] && cols[i][j+1] == tbl_objid2locinfo[1].cube[1]) array_objid[array_objid_iter] = 1;
	  else if (cols[i][j] == tbl_objid2locinfo[2].cube[0] && cols[i][j+1] == tbl_objid2locinfo[2].cube[1]) array_objid[array_objid_iter] = 2;
	  else if (cols[i][j] == tbl_objid2locinfo[3].cube[0] && cols[i][j+1] == tbl_objid2locinfo[3].cube[1]) array_objid[array_objid_iter] = 3;
	  else if (cols[i][j] == tbl_objid2locinfo[4].cube[0] && cols[i][j+1] == tbl_objid2locinfo[4].cube[1]) array_objid[array_objid_iter] = 4;
	  else if (cols[i][j] == tbl_objid2locinfo[5].cube[0] && cols[i][j+1] == tbl_objid2locinfo[5].cube[1]) array_objid[array_objid_iter] = 5;
	  else {
	    assert(cols[i][j] == tbl_objid2locinfo[6].cube[0] && cols[i][j+1] == tbl_objid2locinfo[6].cube[1]);
	    array_objid[array_objid_iter] = 6;
	  }
	  array_objid_iter++, j++;
	}
      }
    }
  }
  assert(array_objid_iter == 32);
  return zdd.compute_id(array_objid);
}

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
  int iteration = atoi(argv[1]); //手数
  int vision = atoi(argv[2]); //0なら白視点，1なら黒視点
  // unsigned long long int parent_table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL; //親のtable size
  string base_filename = base[vision];
  string read_filename_str = base_filename + '_' + to_string(iteration) + ".bin";
  
  unique_ptr<ZDD_base> zdd_check;
  if(vision == 0) {
    zdd_check = make_unique<ZDD_White>(iteration);
  } else {
    zdd_check = make_unique<ZDD_Black>(iteration);
  }
  Posi p;

  unsigned long long int position_id = getzddnum(*zdd_check, argv[3]);
  unsigned long long int seek_id = (position_id / 2) + 1;

  std::cout << "この配置のid：" << position_id << endl;

  p.make_posi(position_id, *zdd_check);
  p.print();

  std::ifstream file(read_filename_str, std::ios::binary);
  if (!file) {
    std::cerr << "ファイルを開けませんでした\n";
    return 1;
  }
  
  file.seekg(static_cast<std::streamoff>(seek_id)); // ← 読みたいnバイト目（0始まり）

  char byte;
  file.read(&byte, 1);

  if(position_id % 2 == 1) byte = byte >> 4U;
  unsigned int val = byte & 15U;

  file.close();

  std::cout << "ラベルid：" << val << endl;

  if (val == v_unknown) std::cout << "ラベル：不明" << endl;
  else if (val == v_win) std::cout << "ラベル：必勝" << endl;
  else if (val == v_lose) std::cout << "ラベル：必敗" << endl;
  else if (val == v_draw) std::cout << "ラベル：引き分け" << endl;
  else if (val == v_notlose) std::cout << "ラベル：負けなし" << endl;
  else {
    assert(val == v_notwin);
    std::cout << "ラベル：勝ちなし" << endl;
  }
}
