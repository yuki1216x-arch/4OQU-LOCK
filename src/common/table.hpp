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

enum { v_unknown = 0, v_win = 1, v_lose = 2, v_draw = 3, v_notlose = 4, v_notwin = 5};

class Table {
private:
  uint64_t *m_table;  //これで配置数分*2
  unsigned long long int m_table_size;
  size_t m_bits_per_entry;
  size_t m_entries_per_word;
  size_t m_value_bits;
  size_t m_game_length_bits;
  int m_iteration;

  unsigned int merge_value(unsigned int value, unsigned int game_length) noexcept {
    if(value != v_win && value != v_lose) assert(game_length == 0);
    return (((unsigned int)((1U << m_game_length_bits) - 1U) & game_length) << m_value_bits) | ((unsigned int)((1U << m_value_bits) - 1U) & value);
  }

public:
  Table(int iter, const char* read_file_name, size_t bits_per_entry, unsigned long long int placement_size) noexcept;
  ~Table() noexcept { delete [] m_table; }
  
  //引数で与えたid番のw,l,unkを得る
  unsigned int get(unsigned long long int id) const noexcept {
    unsigned long long int id64 = id / m_entries_per_word;
    unsigned long long int id1 = (id % m_entries_per_word) * m_bits_per_entry;
    assert(id64 < m_table_size);
    unsigned int v = (unsigned int)((1U << m_bits_per_entry) - 1U) & (unsigned int)(m_table[id64] >> (64ULL-id1-m_bits_per_entry));
    return v;
  }
  unsigned int get_value(unsigned long long int id) const noexcept { return (unsigned int)((1U << m_value_bits) - 1U) & get(id); }
  unsigned int get_game_length(unsigned long long int id) const noexcept { return (unsigned int)((1U << m_game_length_bits) - 1U) & (get(id) >> m_value_bits); }
  
  //表(2bit)のid2番のところにu2(w,l,unk)をセットする関数
  void set(unsigned long long int id, unsigned int entry) noexcept {
    unsigned long long int id64 = id / m_entries_per_word;
    unsigned long long int id1 = (id % m_entries_per_word) * m_bits_per_entry;
    unsigned long long int mask = (unsigned long long int)((1U << m_bits_per_entry) - 1U) << (64ULL-id1-m_bits_per_entry);
    assert(id64 < m_table_size);
    unsigned long long int t = m_table[id64] & ~mask;
    m_table[id64] = t | ((unsigned long long int)entry << (64ULL-id1-m_bits_per_entry));
  }
  void set(unsigned long long int id, unsigned int value, unsigned int game_length) noexcept { set(id, merge_value(value, game_length)); }
};

//1繰り返しの最後に表(4bit)を全部書き出すクラス
class OutTable {
private:
  unsigned char m_buffer;
  int m_num_keep;
  fstream m_ofs;
  size_t m_bits_per_entry;

public:
  OutTable() = delete;
  OutTable(int iter, const string &s, size_t num, size_t bits_per_entry) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::out | fstream::binary | fstream::trunc), m_bits_per_entry(bits_per_entry) {
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
  void write(unsigned int entry) noexcept {
    m_buffer |= static_cast<unsigned char>(entry << (m_num_keep * m_bits_per_entry));
    if(++m_num_keep < static_cast<int>(8 / m_bits_per_entry)) return;
    m_ofs.write((char*)&m_buffer, 1U);
    m_num_keep = 0;
    m_buffer = 0U;
  }

  void flush() noexcept {
    while(m_num_keep != 0) {
      write(0);
    }
  }
};

#endif
