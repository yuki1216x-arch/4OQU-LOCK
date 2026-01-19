#include <memory>
#include <chrono>
#include <random>
#include "table.hpp"
#include "zdd_yonoku.hpp"
#include "posi_yonoku.hpp"

using std::unique_ptr;
using std::make_unique;
using std::mt19937_64;
using std::uniform_int_distribution;
using std::chrono::high_resolution_clock;
using std::to_string;


constexpr unsigned long long int placement_count[2][17] {
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
    144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
    666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
    144799200ULL, 30925440ULL, 11571840ULL, 2932160ULL,
    666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

const string base[2] = {"white_table", "black_table"};

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
    int iteration = atoi(argv[1]);
    int vision = atoi(argv[2]); //0なら白視点, 1なら黒視点
    string base_filename = base[vision];
    string read_filename_str = base_filename + "_" + to_string(iteration) + ".bin";
    const char* read_filename_c_str = read_filename_str.c_str();
    unsigned long long int parent_table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL; //親のtable size

    Table parent_table(16 - iteration, read_filename_c_str, argv[3], parent_table_size64, placement_count[vision][16 - iteration]);   // 親のtable
    
    cout << "before_outtable" << endl;                                                                 // ***
    {   
        OutTable out_table(16 - iteration, read_filename_c_str, placement_count[vision][16 - iteration]);
        for (unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) { //haiti
            out_table.write(parent_table.get(i));
        }
        out_table.flush();
    }
    
    return 0;
}

//  g++ -O2 -Wall -pthread main.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o maketable.out 2>&1
//  ./run_maktable.sh &
//  ./run_maketable_b.sh &
