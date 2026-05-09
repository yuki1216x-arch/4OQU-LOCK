#include "table.hpp"

// 表(4bit)を読み込むクラス
class InTable {
private:
  unsigned char m_buffer;
  int m_num_keep;
  int m_iteration;
  fstream m_ofs;

public:
  InTable() = delete; 
  InTable(int iter, const string &s, size_t num) noexcept;
  ~InTable() noexcept {
    assert(m_num_keep == 0);
    if (! m_ofs) {
      cerr << "Read Error" << endl;
      terminate();
    }
    m_ofs.close();  //ファイルを閉じる
  }
  
  unsigned int read() noexcept {
    unsigned int val;
    if(m_iteration >= 12) {
      if(m_num_keep == 0) {
	m_ofs.read((char*)&m_buffer, 1U);  //1byteリード 
	m_num_keep = 2;
      }
      val = m_buffer & 15U;   //今のbuffer(アドレス)の1111(15U)と＆を取って、値を読み取る(val)
      m_buffer =  m_buffer >> 4U; //4bit分アドレスを進める
      m_num_keep--;
    } else {
      m_ofs.read((char*)&m_buffer, 1U);
      val = m_buffer & 255U;  //今のbuffer(アドレス)の11111111(255U)と＆を取って、値を読み取る(val)
    }
    return val;
  }
};

//iter: 表を読み込むときに、今週目か, s: ファイル名, num: 配置数
InTable::InTable(int iter, const string &s, size_t num) noexcept : m_buffer(0), m_num_keep(0), m_ofs(s, fstream::in | fstream::binary) {
  assert(iter >= 0 && iter <= 16); //iter >= 0 && iter <= 15
  assert(s.size() > 0 && s.size() < 255);
  unsigned char header[8];    //header[0]: 0で固定, header[1]: iterの回数, 以降: 配置数
  std::size_t num_check = 0ULL;
  for(int i = 0; i < 8; i++) m_ofs.read((char*)header+i, 1U); //各表(2bit)の前のヘッダーを読み込む(ヘッダーは個人的なやつ)
  
  for(int i = 0; i < 8; i++) {
    cout << "header[" << i << "]: " << (int)header[i] << endl;
  }

  //headerに記憶されている配置数を取得(256進数)
  for(int i = 5; i >= 0; i--){
    num_check *= 256ULL;    
    num_check += header[i+2];
  }
  //header[0],[1]の値を正誤判定
  if(header[0] != 0 || header[1] != iter) { //iter-1
    cerr << "Header Error" << endl;
    terminate();
  }
  m_iteration = 16 - iter;
  //配置数の確認(header[2-7])
  if(num_check != num) {
    cerr << "Size Error" << endl;
    terminate();
  }
}

Table::Table(int iter, const char* read_file_name, const char* write_file_name, unsigned long long int placement_size) noexcept : m_iteration(16 - iter) {
  if(m_iteration >= 13) {
    m_table_size = (placement_size + 15ULL) / 16ULL;
    m_bits_per_entry = 4;
    m_entries_per_word = 16;
    m_value_bits = 2;
    m_game_length_bits = 2;
  } else {
    m_table_size = (placement_size + 7ULL) / 8ULL;
    m_bits_per_entry = 8;
    m_entries_per_word = 8;
    m_value_bits = 3;
    m_game_length_bits = 5;
  }
  
  assert(iter >= 0 && iter <= 16); //iter >= 0 && iter <= 15
  m_table = new uint64_t [m_table_size];
  
  assert(read_file_name && read_file_name[0] != '\0');
  assert(write_file_name && write_file_name[0] != '\0');
  cout << "read_file_name: " << read_file_name << endl;
  fstream read_file (read_file_name, fstream::in | fstream::binary);
  if(!read_file) {    //readファイルがなかった場合
    cout << "no file" << endl;
    for(size_t tableid = 0; tableid < m_table_size; tableid++) m_table[tableid] = v_unknown; //全ての表を0にする
  } else {
    {   
      unsigned long long int nwin = 0, nlose = 0, ndraw = 0, nunknown = 0, lunknown = 0; // lunknownが最後のunknownの番号(配置数-1しておく)
      unsigned long long int nnotwin = 0, nnotlose = 0;
      
      cout << "aaaa" << endl;
      InTable in_table(iter, read_file_name, placement_size); //ここのInTableでエラー
      cout << "bbbb" << endl;
      for(unsigned long long int i = 0; i < placement_size; i++) {
	unsigned int entry = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
	unsigned int value = (unsigned int)((1U << m_value_bits) - 1U) & entry;
	unsigned int game_length = (unsigned int)((1U << m_game_length_bits) - 1U) & (entry >> m_value_bits);
	
	if(value == v_win) {
	  nwin++;
	}else if(value == v_unknown) {
	  assert(game_length == 0ULL);
	  nunknown++;
	  lunknown = i;
	} else if(value == v_draw) {
	  assert(game_length == 0ULL);
	  ndraw++;
	} else if(value == v_notlose) {
	  assert(game_length == 0ULL);
	  nnotlose++;
	} else if(value == v_notwin) {
	  assert(game_length == 0ULL);
	  nnotwin++;
	} else {
	  nlose++;
	} 
	set(i, entry);
      }
      cout << "before nwin  =  " << nwin << endl;
      cout << "before nlose  =  " << nlose << endl;
      cout << "before nnotlose = " << nnotlose << endl;
      cout << "before nnotwin = " << nnotwin << endl;
      cout << "before nunknown  =  " << nunknown << endl;
      cout << "last unknown  =  " << lunknown << endl;
    }
  }
  read_file.close();
}
