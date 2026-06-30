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
    144799200ULL, 61850880ULL, 11571840ULL, 2932160ULL,
    666080ULL, 102400ULL, 10336ULL, 768ULL, 32ULL},
    {0ULL, 63952986240ULL, 55896469200ULL, 34197443280ULL,
    19628376768ULL, 6485285664ULL, 2803351824ULL, 540933120ULL,
    144799200ULL, 30925440ULL, 11571840ULL, 2932160ULL,
    666080ULL, 73600ULL, 10336ULL, 512ULL, 32ULL}
};

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
    int iteration = atoi(argv[1]);
    int vision = atoi(argv[2]); // 0: white's perspective, 1: black's perspective
    string read_filename_str = "data/db/" + string(argv[3]);
    unique_ptr<ZDD> zdd_check = make_unique<ZDD>(vision, iteration);

    size_t bits_per_entry;
    if(iteration > 12) bits_per_entry = 4;
    else bits_per_entry = 8;
    Table check_table(16 - iteration, read_filename_str.c_str(), bits_per_entry, placement_count[vision][16 - iteration]);   // [2]: input file, [4]: output file; parent table
    Posi p;

//     constexpr LocInfo tbl_objid2locinfo[8] = {
//      { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
//      { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
//      { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
//      { {'W', 'W'}, 2, 0, 2, 0, 'W'}, //3
//      { {'W', 'B'}, 2, 1, 1, 0, 'B'}, //4
//      { {'B', 'W'}, 2, 1, 1, 0, 'W'}, //5
//      { {'B', 'B'}, 2, 2, 0, 0, 'B'}, //6
//      { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //7  
// };

// constexpr LocInfo tbl_objid2locinfo2[5] = {
//      { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
//      { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
//      { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
//      { {'U', '.'}, 1, 0, 0, 1, 'U'}, //3
//      { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //4  
// };

    unsigned char array_objid[32] = {};
    array_objid[0] = 0;
    array_objid[1] = 0;
    array_objid[2] = 0;
    array_objid[3] = 0;
    array_objid[4] = 0;
    array_objid[5] = 0;
    array_objid[6] = 0;
    array_objid[7] = 0;

    array_objid[8] = 1;
    array_objid[9] = 4;
    array_objid[10] = 0;
    array_objid[11] = 0;

    array_objid[12] = 1;
    array_objid[13] = 4;
    array_objid[14] = 5;
    array_objid[15] = 0;

    array_objid[16] = 0;
    array_objid[17] = 2;
    array_objid[18] = 5;
    array_objid[19] = 0;

    array_objid[20] = 1;
    array_objid[21] = 2;
    array_objid[22] = 2;
    array_objid[23] = 2;

    array_objid[24] = 0;
    array_objid[25] = 0;
    array_objid[26] = 0;
    array_objid[27] = 0;
    array_objid[28] = 0;
    array_objid[29] = 0;
    array_objid[30] = 0;
    array_objid[31] = 1;

    unsigned long long int i = zdd_check->compute_id(array_objid, 32);
    p.make_posi(i, *zdd_check);
    int gameval = check_table.get_value(i);

    cout << "id = " << i << endl;
    p.print();
    //enum { v_unknown = 0, v_win = 1, v_lose = 2, v_draw = 3, v_notlose = 4, v_notwin = 5};
    switch(gameval) {
        case 0:
            cout << "unknown" << endl;
            break;
        case 1:
            cout << "win" << endl;
            break;
        case 2:
            cout << "lose" << endl;
            break;
        case 3:
            cout << "draw" << endl;
            break;
        case 4:
            cout << "notlose" << endl;
            break;
        case 5:
            cout << "notwin" << endl;
            break;
        default:
            break;
    } 
    
    return 0;
}

//  g++ -O2 -Wall -pthread debug.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o debug.out 2>&1
//  ./debug.out 10 0 white_table_10.bin db > deb.txt 2>&1 &
//  ./debug.out 2 0 white_table_2.bin db > deb.txt 2>&1 &

//id = 700836752: id of the original configuration
//id = 1412134976: id of the swapped configuration
