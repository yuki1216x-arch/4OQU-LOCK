#include <memory>
#include "table.hpp"

using std::string;
using std::unique_ptr;
using std::make_unique;
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

int main(int argc, char* argv[]) {
    unsigned long long int win = 0;
    unsigned long long int lose = 0;
    unsigned long long int draw = 0;
    unsigned long long int notlose = 0;
    unsigned long long int notwin = 0;
    unsigned long long int unknown = 0;
    int iteration = atoi(argv[1]);
    int vision = atoi(argv[2]);
    int division = atoi(argv[3]);
    string base_filename = argv[4];
    string read_filename_str = base_filename + "_" + to_string(iteration) + ".bin";
    const char* read_filename_c_str = read_filename_str.c_str();
    unsigned long long int table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL;
    Table new_table(16 - iteration, read_filename_c_str, argv[5 + division], table_size64, placement_count[vision][16 - iteration]);
    unique_ptr<Table> old_table[division];
    for(int i = 0; i < division; i++) {
        old_table[i] = make_unique<Table>(16 - iteration, argv[5 + i], argv[6 + division + i], table_size64, placement_count[vision][16 - iteration]);
    }
    for(unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) {
        int val = old_table[i % division]->get(i);
        // if(val != 0 && old_table[1 - i % division]->get(i) != 0) {
        //     cout << "division error1" << endl;
        //     terminate();
        // }
        switch(val) {
            case 1:
                win++;
                break;
            case 2:
                lose++;
                break;
            case 3:
                draw++;
                break;
            case 4:
                notlose++;
                break;
            case 5:
                notwin++;
                break;
            case 0:
                unknown++;
                break;
            default:
                cout << "division error2" << endl;
                terminate();
        }
        new_table.set(i, val);
        if (i % 100000000ULL == 0ULL) { //1000000000ULL i
            cout << "id: " << i << ", nwin: " << win << ", nlose: " << lose << ", ndraw: " << draw << ", nnotwin: " << notwin << ", nnotlose: " << notlose << ", nunknown: " << unknown << endl;
        }
    }
    {   
        OutTable out_table(16 - iteration, read_filename_c_str, placement_count[vision][16 - iteration]);
        for (unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) { //haiti
            out_table.write(new_table.get(i));
        }
        out_table.flush();
    }

    cout << "nwin  =  " << win << endl;
    cout << "nlose  =  " << lose << endl;
    cout << "ndraw  =  " << draw << endl;
    cout << "nnotlose  =  " << notlose << endl;
    cout << "nnotwin  =  " << notwin << endl;
    cout << "nunknown  =  " << unknown << endl;
    cout << "total  =  " << win + lose + draw + notlose + notwin + unknown << endl;

    return 0;
}

//  g++ -O2 -Wall -pthread CombTable.cpp table.cpp -std=c++14 -o combtab.out 2>&1
//  ./combtab.out 10 0 4 white_table white_table_10_4_0.bin white_table_10_4_1.bin white_table_10_4_2.bin white_table_10_4_3.bin db db db db db > 10_w_new.txt 2>&1 &
//  ./combtab.out 10 1 4 black_table black_table_10_4_0.bin black_table_10_4_1.bin black_table_10_4_2.bin black_table_10_4_3.bin db db db db db > 10_b_new.txt 2>&1 &