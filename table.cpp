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

Table::Table(int iter, const char* read_file_name, const char* write_file_name, unsigned long long int size, unsigned long long int size1, unsigned long long int size2) noexcept : m_table(new uint64_t [size] ), m_game_length_table(new uint64_t [size1] ), m_table_size(size), m_game_length_table_size(size1), m_iteration(16 - iter) {
  assert(iter >= 0 && iter <= 16); //iter >= 0 && iter <= 15
  assert(read_file_name && read_file_name[0] != '\0');
  assert(write_file_name && write_file_name[0] != '\0');
  cout << "read_file_name: " << read_file_name << endl;
  fstream read_file (read_file_name, fstream::in | fstream::binary);
  if(!read_file) {    //readファイルがなかった場合
    cout << "no file" << endl;
    for(size_t tableid = 0; tableid < size; tableid++) m_table[tableid] = v_unknown; //全ての表を0にする
    for(size_t tableid = 0; tableid < size1; tableid++) m_game_length_table[tableid] = 0ULL; //一旦最大値
  } else {
    {   
      unsigned long long int nwin = 0, nlose = 0, ndraw = 0, nunknown = 0, lunknown = 0; // lunknownが最後のunknownの番号(配置数-1しておく)
      unsigned long long int nnotwin = 0, nnotlose = 0;
      
      cout << "aaaa" << endl;
      InTable in_table(iter, read_file_name, size2); //ここのInTableでエラー
      cout << "bbbb" << endl;
      for(unsigned long long int i = 0; i < size2; i++) {
	unsigned int get_res = in_table.read();   //in_Tableからidを一つずつ読み込んでいき、その値をvに代入
	unsigned int v, game_length;
	if(iter >= 12) {
	  if(get_res == 15U) {
	    v = v_draw;
	    game_length = 0U;
	  } else {
	    v = get_res & 7U;
	    game_length = get_res >> 3U;
	  }
	} else {
	  v = get_res & 7U;
	  game_length = get_res >> 3U;
	}
	
	if(v == v_win) {
	  nwin++;
	}else if(v == v_unknown) {
	  nunknown++;
	  lunknown = i;
	} else if(v == v_draw) {
	  ndraw++;
	} else if(v == v_notlose) {
	  nnotlose++;
	} else if(v == v_notwin) {
	  nnotwin++;
	} else {
	  nlose++;
	} 
	set(i, v);
	set_game_length(i, game_length);
      }
      cout << "before nwin  =  " << nwin << endl;
      cout << "before nlose  =  " << nlose << endl;
      cout << "before nunknown  =  " << nunknown << endl;
      cout << "last unknown  =  " << lunknown << endl;
    }
  }
  read_file.close();
}
