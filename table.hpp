#ifndef INCLUDE_TABLE
#define INCLUDE_TABLE
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <exception>
#include <fstream>
#include <string>

using std::fstream;
using std::size_t;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::terminate;

enum { v_unknown = 5, v_win = 0, v_lose = 1, v_draw = 2, v_notlose = 3, v_notwin = 4};


class Table {
private:
  uint64_t *m_table;  //これで配置数分*2
  uint64_t *m_game_length_table;
  unsigned long long int m_table_size;
  unsigned long long int m_game_length_table_size;
  int m_iteration;

public:
  Table(int iter, const char* read_file_name, const char* write_file_name, unsigned long long int size, unsigned long long int size1, unsigned long long int size2) noexcept;
  ~Table() noexcept { delete [] m_table; }
  
  //引数で与えたid番のw,l,unkを得る
  unsigned int get(unsigned long long int id2) const noexcept {
    unsigned long long int id64 = id2 / 16ULL; //id64 = id2 / 32ULL
    unsigned long long int id1 = (id2 % 16ULL) * 4ULL; //id1 = (id2 % 32ULL) * 2ULL;
    assert(id64 < m_table_size);
    unsigned int v = 15U & (unsigned int)(m_table[id64] >> (64ULL-id1-4ULL)); //3U & (unsigned int)(m_table[id64] >> (64ULL-id1-2ULL));
    return v;
  }
  int get_game_length(unsigned long long int id2) const noexcept {
    if(m_iteration >= 13) {
      unsigned long long int id64 = id2 / 32ULL;
      unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
      assert(id64 < m_game_length_table_size);
      int v = 3U & (int)(m_game_length_table[id64] >> (64ULL-id1-2ULL));
      return v;
    } else if(m_iteration >= 1) {
      unsigned long long int id64 = id2 / 16ULL;
      unsigned long long int id1 = (id2 % 16ULL) * 4ULL;
      assert(id64 < m_game_length_table_size);
      int v = 15U & (int)(m_game_length_table[id64] >> (64ULL-id1-4ULL));
      return v;
    } else {
      unsigned long long int id64 = id2 / 8ULL;
      unsigned long long int id1 = (id2 % 8ULL) * 8ULL;
      assert(id64 < m_game_length_table_size);
      int v = 255U & (int)(m_game_length_table[id64] >> (64ULL-id1-8ULL));
      return v;
    }
  }
  
  //表(2bit)のid2番のところにu2(w,l,unk)をセットする関数
  void set(unsigned long long int id2, unsigned int u2) noexcept {
    unsigned long long int id64 = id2 / 16ULL; //id64 = id2 / 32ULL;
    unsigned long long int id1 = (id2 % 16ULL) * 4ULL; //id1 = (id2 % 32ULL) * 2ULL;
    unsigned long long int mask = 15ULL << (60ULL-id1); //mask = 3ULL << (62ULL-id1);
    assert(id64 < m_table_size);
    unsigned long long int t = m_table[id64] & ~mask; // いらない説 t = m_table[id64] & ~mask;
    m_table[id64] = t | ((unsigned long long int)u2 << (60ULL-id1)); //t | ((unsigned long long int)u2 << (62ULL-id1));
  }
  void set_game_length(unsigned long long int id2, unsigned int u2) noexcept {
    if(m_iteration >= 13) {
      unsigned long long int id64 = id2 / 32ULL;
      unsigned long long int id1 = (id2 % 32ULL) * 2ULL;
      unsigned long long int mask = 3ULL << (62ULL-id1);
      assert(id64 < m_game_length_table_size);
      unsigned long long int t = m_game_length_table[id64] & ~mask;
      m_game_length_table[id64] = t | ((unsigned long long int)u2 << (62ULL-id1));
    } else if(m_iteration >= 1) {
      unsigned long long int id64 = id2 / 16ULL;
      unsigned long long int id1 = (id2 % 16ULL) * 4ULL;
      unsigned long long int mask = 15ULL << (60ULL-id1);
      assert(id64 < m_game_length_table_size);
      unsigned long long int t = m_game_length_table[id64] & ~mask;
      m_game_length_table[id64] = t | ((unsigned long long int)u2 << (60ULL-id1));
    } else {
      unsigned long long int id64 = id2 / 8ULL;
      unsigned long long int id1 = (id2 % 8ULL) * 8ULL;
      unsigned long long int mask = 255ULL << (56ULL-id1);
      assert(id64 < m_game_length_table_size);
      unsigned long long int t = m_game_length_table[id64] & ~mask;
      m_game_length_table[id64] = t | ((unsigned long long int)u2 << (56ULL-id1));
    }
  }
};

//1繰り返しの最後に表(4bit)を全部書き出すクラス
class OutTable {
private:
  unsigned char m_buffer;
  int m_num_keep;
  fstream m_ofs;
  int m_iteration;

public:
  OutTable() = delete;
  OutTable(int iter, const string &s, size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::out | fstream::binary | fstream::trunc), m_iteration(16 - iter) {
    assert(iter >= 0 && iter <= 16); //iter >= 0 && iter <= 15
    assert(s.size() > 0 && s.size() < 255);
    cout << "write" << endl;
    unsigned char header[8] = {0};
    header[0] = 0;
    header[1] = (unsigned char)iter;
    for(int i = 2; i < 8; i++){
      header[i] = num % 256U;
      num /= 256U;
    }
    // output header[]
    m_ofs.write((char *)header, 8U);
  }
  
  ~OutTable() noexcept {
    // output keeping data in m_buffer;
    assert(m_num_keep == 0);
    if (! m_ofs) {
      cerr << "Write Error" << endl;
      terminate();
    }
    m_ofs.close();
  }

    //表(4bit)の各番地に書き込んでいく(bitesがw,l,unk)
  void write(unsigned int value_bytes, unsigned int game_length_bytes) noexcept {    
    // put bites to m_buffer
    unsigned int bytes = compute_output_bytes(value_bytes, game_length_bytes);
    if(m_iteration >= 12) {
      m_buffer |= static_cast<unsigned char>(bytes << (m_num_keep * 4));
      if (++m_num_keep < 2) return;
      m_ofs.write((char*)&m_buffer, 1U);
      m_num_keep = 0;
      m_buffer = 0U;
    } else {
      m_buffer |= static_cast<unsigned char>(bytes);
      m_ofs.write((char*)&m_buffer, 1U);
      m_buffer = 0U;
    }
  }

  unsigned int compute_output_bytes(unsigned int value_bytes, unsigned int game_length_bytes) noexcept {
    unsigned int bytes;
    if(m_iteration >= 12) {
      assert(value_bytes == v_win || value_bytes == v_lose || value_bytes == v_draw);
      if(value_bytes == v_draw) bytes = 15U;
      else bytes = ((game_length_bytes & 0b111) << 1) | (value_bytes & 0b1);
    } else {
      bytes = ((game_length_bytes & 0b00011111) << 3) | (value_bytes & 0b00000111);
    }
    return bytes;
  }   

  void flush() noexcept {
    while(m_num_keep != 0) {
      write(0, 0);
    }
  }
};

#endif
