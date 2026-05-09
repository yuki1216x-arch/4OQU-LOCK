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

constexpr unsigned long long int placement_count[2][17] {
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 8793607680ULL, 2803351824ULL, 811399680ULL,
    144799200ULL, 61850880ULL, 17357760ULL, 4264960ULL,
    666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
    144799200ULL, 46388160ULL, 17357760ULL, 2932160ULL,
    666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
    int iteration = atoi(argv[1]);
    int vision = atoi(argv[2]); // 0: white's perspective, 1: black's perspective
    string read_filename_str = "data/db/" + string(argv[3]);
    unique_ptr<ZDD> zdd_check = make_unique<ZDD>(vision, iteration);
    Table check_table(16 - iteration, read_filename_str.c_str(), argv[4], placement_count[vision][16 - iteration]);   // [2]: input file, [4]: output file; parent table
    Posi p;

    // for quick validation
    // tbl_dbg array storing random values
    // unsigned long long int tbl_dbg[50];

    // // seed the random number generator
    // mt19937_64 rng(high_resolution_clock::now().time_since_epoch().count());

    // // retrieve the value of placement_count[i][j]
    // unsigned long long int max_val = placement_count[vision][16 - iteration];

    // // set the random range (0 to max_val)
    // uniform_int_distribution<unsigned long long int> dist(0, max_val);

    // cout << "Generate 16 random values from placement_count[" << vision << "][" << 16 - iteration << "] (<=" << max_val << ") and store them in tbl_dbg" << endl;

    // // generate random values and store them in tbl_dbg
    // for (int k = 0; k < 16; ++k) {
    //     //do {
    //         tbl_dbg[k] = dist(rng);
    //     //} while(check_table.get(tbl_dbg[k]) != 3);
    // }

    // // check the generated random values
    // cout << "Values stored in tbl_dbg: ";
    // for (int k = 0; k < 16; ++k) {
    //     cout << " " << tbl_dbg[k];
    // }
    // cout << endl;

    
    for(unsigned long long int i = 0; i < 32; i++) { //int i0 = 0; i0 < 16; i0++
        //unsigned long long int i = tbl_dbg[i0];
        assert(i < placement_count[vision][16 - iteration]);
        p.make_posi(i, *zdd_check);
        cout << "i = " << i << endl;
        p.print();
        int val = check_table.get_value(i);
        if(val == 1) {
            cout << "win" << endl;
        } else if(val == 2) {
            cout << "lose" << endl;
        } else if(val == 3) {
            cout << "draw" << endl;
        } else if(val == 4) {
            cout << "notlose" << endl;
        } else if(val == 5) {
            cout << "notwin" << endl;
        } else {
            cout << "unknown" << endl;
        }
    }
    
    return 0;
}

//  g++ -O2 -Wall -pthread CheckTable.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o checktable.out 2>&1
//  ./checktable.out 10 0 white_table_10.bin db > check_table_10_w.txt 2>&1 &
//  ./checktable.out 10 1 black_table_10.bin db > check_table_10_b.txt 2>&1 &
