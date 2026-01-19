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

unsigned long long int nwin = 0, nlose = 0, ndraw = 0, nunknown = 0, lunknown = 0, nnotwin = 0, nnotlose = 0;

unsigned long long int mm_me_win = 0, mm_ee_win = 0, me_mm_win = 0, me_em_win = 0; //(視点手番)_(視点手番)_(ラベル)
unsigned long long int mm_me_lose = 0, mm_ee_lose = 0, me_mm_lose = 0, me_em_lose = 0;
unsigned long long int mm_me_ntwin = 0, mm_ee_ntwin = 0, me_mm_ntwin = 0, me_em_ntwin = 0;
unsigned long long int mm_me_ntlose = 0, mm_ee_ntlose = 0, me_mm_ntlose = 0, me_em_ntlose = 0;
unsigned long long int all_draw = 0;

//---.exe iter read_file write_file
int main(int argc, char *argv[]) {
    int iteration = atoi(argv[1]);
    int vision = atoi(argv[2]); //0なら白視点, 1なら黒視点
    int division = atoi(argv[3]); //分割数 分割しないなら1
    int surplus = atoi(argv[4]); //2*i + jのjの部分 分割数が2のとき、0なら偶数だけの、1なら奇数番だけの表を作る 分割数が1のときは0固定
    string base_filename = base[vision];
    string base_filename_opp = base[1 - vision];
    string read_filename_str = base_filename + "_" + to_string(iteration) + "_" + to_string(division) + "_" + to_string(surplus) + ".bin";
    string read_filename_str_child = base_filename + "_" + to_string(iteration+1) + ".bin";
    string read_filename_str_child_opp = base_filename_opp + "_" + to_string(iteration+1) + ".bin";
    const char* read_filename_c_str = read_filename_str.c_str();
    const char* read_filename_c_str_child = read_filename_str_child.c_str();
    const char* read_filename_c_str_child_opp = read_filename_str_child_opp.c_str();
    unsigned long long int parent_table_size64 = (placement_count[vision][16 - iteration] + 15ULL) / 16ULL; //親のtable size
    unsigned long long int child_table_size64 = (placement_count[vision][15 - iteration] + 15ULL) / 16ULL; //子のtable size
    unsigned long long int opp_child_table_size64 = (placement_count[1 - vision][15 - iteration] + 15ULL) / 16ULL; //相手視点の子のtable size
    unique_ptr<ZDD_base> zdd_parent;
    unique_ptr<ZDD_base> zdd_child;
    unique_ptr<ZDD_base> zdd_child_opp;
    if(vision == 0) {
        zdd_parent = make_unique<ZDD_White>(iteration);
        if(iteration == 15) {
            zdd_child = make_unique<ZDD_White>(iteration);
            zdd_child_opp = make_unique<ZDD_Black>(iteration);
        } else {
            zdd_child = make_unique<ZDD_White>(iteration + 1);
            zdd_child_opp = make_unique<ZDD_Black>(iteration + 1);
        }
    } else {
        zdd_parent = make_unique<ZDD_Black>(iteration);
        if(iteration == 15) {
            zdd_child = make_unique<ZDD_Black>(iteration);
            zdd_child_opp = make_unique<ZDD_White>(iteration);
        } else {
            zdd_child = make_unique<ZDD_Black>(iteration + 1);
            zdd_child_opp = make_unique<ZDD_White>(iteration + 1);
        }
    }
    Table parent_table(16 - iteration, read_filename_c_str, argv[5], parent_table_size64, placement_count[vision][16 - iteration]);   // 親のtable
    Table child_table(15 - iteration, read_filename_c_str_child, argv[6], child_table_size64, placement_count[vision][15 - iteration]); //子のtalbe
    Table opp_child_table(15 - iteration, read_filename_c_str_child_opp, argv[7], opp_child_table_size64, placement_count[1 - vision][15 - iteration]); //相手側の子のtable
    Posi p;

    //簡易検証用
    // 乱数を格納する tbl_dbg 配列
    // unsigned long long int tbl_dbg[16];

    // // 乱数生成器のシード設定
    // mt19937_64 rng(high_resolution_clock::now().time_since_epoch().count());

    // // placement_count[i][j] の値を取得
    // unsigned long long int max_val = placement_count[vision][16 - iteration];

    // // 乱数生成範囲の設定 (0 から max_val まで)
    // uniform_int_distribution<unsigned long long int> dist(0, max_val);

    // cout << "placement_count[" << vision << "][" << 16 - iteration << "] (" << max_val << "以下) から16個の乱数を生成し、tbl_dbg に格納します。" << endl;

    // // 乱数を生成して tbl_dbg に格納
    // for (int k = 0; k < 16; ++k) {
    //     tbl_dbg[k] = dist(rng);
    // }

    // // 生成された乱数を確認
    // cout << "tbl_dbg に格納された値:";
    // for (int k = 0; k < 16; ++k) {
    //     cout << " " << tbl_dbg[k];
    // }
    // cout << endl;

    unsigned long long int al_win_num = 0;
    unsigned long long int al_lose_num = 0;
    unsigned long long int al_unknown_num = 0;
    unsigned long long int perfect = 0;
    unsigned long long int imperfect = 0;
    
    for(unsigned long long int i = 0ULL; division*i + surplus < placement_count[vision][16 - iteration]; i++) {
        // if(mm_me_ntlose + mm_ee_ntlose + me_mm_ntlose + me_em_ntlose != nnotlose) {
        //     cout << i << " " << mm_me_ntlose << " " << mm_ee_ntlose << " " << me_mm_ntlose << " " << me_em_ntlose << " " << nnotlose << endl;
        // }

        //if(division*i + surplus != 3398536176) continue;

        assert(mm_me_win + mm_ee_win + me_mm_win + me_em_win + al_win_num == nwin);
        assert(mm_me_lose + mm_ee_lose + me_mm_lose + me_em_lose + al_lose_num == nlose);
        assert(mm_me_ntwin + mm_ee_ntwin + me_mm_ntwin + me_em_ntwin == nnotwin);
        assert(mm_me_ntlose + mm_ee_ntlose + me_mm_ntlose + me_em_ntlose == nnotlose);
        assert(all_draw == ndraw);

        //int perfect_info_flg = 0;
        int notlose_flg = 0;
        int which_ntlose = 0;
    //  簡易検証用
    // for(int i0 = 0; i0 < 16; i0++) {
    //     unsigned long long int i = tbl_dbg[i0];
    //     assert(i < placement_count[vision][16 - iteration]);

        p.make_posi(division*i + surplus, *zdd_parent); //i番目の親の配置を作る i
        // cout << "i = " << i << endl;
        // p.print();

        Action actions[1000] = {}; //max_legal_numだった
        unsigned char board_belief[12][46] = {}; //第三者視点の配置を保存する
        int nchild = p.compute_actions(actions, vision, iteration); //合法手の列挙

        int num_of_un = p.getunknowninfo(board_belief, vision, iteration); //配列を渡して見えないコマの数をreturnするようにする
        if(num_of_un == 1) {
            perfect++;
            //perfect_info_flg = 1;
        } else {
            imperfect++;
        }
        assert(num_of_un > 0);
        if(nchild == -1) {
            al_win_num++;
        } else if(nchild == -2) {
            al_lose_num++;
        } else if(nchild == -3) {
            al_unknown_num++;
        }
        int game_val = 0;
        int already_decided = 0;
        if(nchild > 0) { //前半のループでは必勝、負無しを判定
            int num_of_action = nchild;
            for(int which_table = 0; which_table < 2; which_table++) { //2種類のtableのどっちを使うか
                int win_plan_num = 0; //必勝戦略の数
                int lose_plan_num = 0; //必敗戦略の数
                int nlose_plan_num = 0; //負無し戦略の数
                int nwin_plan_num = 0; //勝無し戦略の数
                int draw_plan_num = 0; //引分戦略の数
                int unknown_plan_num = 0; //不明戦略の数
                //cout << "using " << which_table << "table" << endl;
                for(int j = 0; j < num_of_action; j++) { //合法手の数だけループ
                    int kati = 0; //勝ち
                    int make = 0; //負け
                    int makenasi = 0; //負無し
                    int katinasi = 0; //勝無し
                    int hikiwake = 0; //引分
                    //cout << "|";
                    for(int m = 0; m < num_of_un; m++) { //belief state内の節点の数だけループ
                        p.make_posi_n(board_belief, m); //第3者視点の配置を作る
                        int board_check = p.make_action(actions[j], vision, iteration);
                        if(board_check == 0) { //まだ終わってない 表から値を取ってくる
                            if(which_table == 0) { //子が自分視点
                                p.make_posi_myself();
                                unsigned long long int num_of_haiti = p.getzddnum(*zdd_child); //子のZDDから子の配置の番号を取得
                                int child_table_val = child_table.get(num_of_haiti); //子のtableから値を取得
                                assert(child_table_val >= 0 && child_table_val <= 5);
                                if(child_table_val == 1) {
                                    //視点プレイヤ勝ち
                                    kati++;
                                    // cout << "id:" << num_of_haiti << endl;
                                    // p.print();
                                    //cout << "w";
                                } else if(child_table_val == 2) {
                                    //視点プレイヤ負け
                                    make++;
                                    //cout << "l";
                                } else if(child_table_val == 4) {
                                    makenasi++;
                                    //cout << "nl";
                                } else if(child_table_val == 5) {
                                    katinasi++;
                                    //cout << "nw";
                                } else if(child_table_val == 3) {
                                    hikiwake++;
                                    //cout << "d";
                                }
                            } else { //子が相手視点
                                p.make_posi_opponent();
                                unsigned long long int num_of_haiti = p.getzddnum(*zdd_child_opp); //子のZDDから子の配置の番号を取得
                                int child_table_val = opp_child_table.get(num_of_haiti); //子のtableから値を取得
                                assert(child_table_val >= 0 && child_table_val <= 5);
                                if(child_table_val == 1) {
                                    //視点プレイヤ負け
                                    make++;
                                    //cout << "l";
                                } else if(child_table_val == 2) {
                                    //視点プレイヤ勝ち
                                    kati++;
                                    //cout << "w";
                                } else if(child_table_val == 4) {
                                    katinasi++;
                                    //cout << "nw";
                                } else if(child_table_val == 5) {
                                    makenasi++;
                                    //cout << "nl";
                                } else if(child_table_val == 3) {
                                    hikiwake++;
                                    //cout << "d";
                                }
                            }
                        } else {
                            if(board_check == 3) {
                                //視点プレイヤ負け
                                make++;
                                //cout << "l";
                            } else if(board_check == 2) {
                                //視点プレイヤ勝ち
                                kati++;
                                //cout << "w";
                            } else if(board_check == 10) {
                                //引分
                                hikiwake++;
                                //cout << "d";
                            }
                        }
                        //cout << "|";
                    }
                    p.make_posi(division*i + surplus, *zdd_parent); //元に戻す
                    //if() { //手番プレイヤが視点プレイヤのとき
                    if(kati == num_of_un) { //必勝
                        if(iteration % 2 == vision) {
                            game_val = -4;
                            if(which_table == 0) {
                                mm_me_win++;
                            } else {
                                mm_ee_win++;
                            }
                            already_decided = 1;
                            break;
                        } else {
                            win_plan_num++;
                        }
                    } else if(kati + makenasi + hikiwake == num_of_un) { //負無し
                        if(hikiwake == num_of_un) { //引分
                            draw_plan_num++;
                        } else {
                            nlose_plan_num++;
                        }
                    } else if(make == num_of_un) { //必敗
                        lose_plan_num++;                    
                    } else if(make + katinasi + hikiwake == num_of_un) { //勝無し
                        nwin_plan_num++;
                    } else {
                        unknown_plan_num++;
                    }
                    if(j == num_of_action - 1 && iteration % 2 == vision) {
                        if(nlose_plan_num > 0 || draw_plan_num > 0) {
                            //game_val = -6;
                            notlose_flg = 1;
                            if(which_table == 0) {
                                mm_me_ntlose++;
                                //cout << "mm_me_ntlose++;" << i << endl;
                                which_ntlose = 1;
                            } else {
                                mm_ee_ntlose++;
                                //cout << "mm_ee_ntlose++;" << i << endl;
                                which_ntlose = 2;
                            }
                            already_decided = 1;
                            break;
                        }
                        // else if(draw_plan_num > 0 && unknown_plan_num == 0) {
                        //     game_val = -8;
                        //     all_draw++;
                        //     already_decided = 1;
                        //     break;
                        // }
                    }
                    // } else { //手番プレイヤが視点プレイヤでないとき
                    //     if(kati == num_of_un) { //必勝
                    //         win_plan_num++;
                    //     } else if(kati + makenasi + hikiwake == num_of_un) { //負無し
                    //         if(hikiwake == num_of_un) { //引分
                    //             draw_plan_num++;
                    //         } else {
                    //             nlose_plan_num++;
                    //         }
                    //     } else if(make == num_of_un) { //必敗
                    //         lose_plan_num++;
                    //     } else if(make + katinasi + hikiwake == num_of_un) { //勝無し
                    //         nwin_plan_num++;
                    //     } else {
                    //         unknown_plan_num++;
                    //     }
                    // }

                    //cout << endl;
                }
                //すべての合法手を確認した後
                if(already_decided == 0) {
                    if(win_plan_num == num_of_action) {
                        game_val = -4; //必勝を与える
                        if(which_table == 0) {
                            me_mm_win++;
                        } else {
                            me_em_win++;
                        }
                    } else if(nlose_plan_num + win_plan_num + draw_plan_num == num_of_action) {
                        //自分が完全情報でも相手が不完全情報なら負無しがつくが、相手も完全情報ならついてはいけない
                        // if(perfect_info_flg == 1) {
                        //     if(p.is_opp_uninfo(board_belief, vision, iteration)) { //相手が不完全情報なら
                        //         // game_val = -6; //負無しを与える
                        //         if(which_table == 0) {
                        //             me_mm_ntlose++;
                        //             which_ntlose = 3;
                        //         } else {
                        //             me_em_ntlose++;
                        //             which_ntlose = 4;
                        //         }
                        //         notlose_flg = 1;
                        //     }
                        // } else {
                            // game_val = -6; //負無しを与える
                        if(which_table == 0) {
                            me_mm_ntlose++;
                            which_ntlose = 3;
                        } else {
                            me_em_ntlose++;
                            which_ntlose = 4;
                        }
                        notlose_flg = 1;
                    }
                }      
                if(game_val < 0) {
                    nchild = game_val;
                    break;
                } else if(notlose_flg == 1) {
                    break;
                }

            }
        }
        if(nchild > 0) { //後半のループでは必敗、勝無しを判定
            for(int which_table = 0; which_table < 2; which_table++) {
                //cout << "using " << which_table << "table" << endl;
                int opp_win_count = 0;
                int opp_nlose_count = 0;
                //int opp_draw_count = 0;
                for(int j = 0; j < num_of_un; j++) { //親のbelief stateの中身の節点の数だけループ
                    //cout << "now j is " << j << endl;
                    Action actions_opp[1000] = {};
                    unsigned char board_belief_opp[12][46] = {};
                    //int perfect_info_flg_opp = 0;
                    p.make_posi_n(board_belief, j); //第3者視点の配置を作る
                    //p.print();
                    p.make_posi_opponent(); //相手視点の配置を作る
                    //p.print();
                    int nchild_opp = p.compute_actions(actions_opp, 1 - vision, iteration);
                    int num_of_un_opp = p.getunknowninfo(board_belief_opp, 1 - vision, iteration);
                    //cout << "num_of_un_opp is " << num_of_un_opp << endl;

                    //if(num_of_un_opp == 1) perfect_info_flg_opp = 1;

                    int num_of_action_opp = nchild_opp;
                    int flg_l = 0;
                    int win_plan_num_opp = 0;
                    int nlose_plan_num_opp = 0;
                    int draw_plan_num_opp = 0;
                    int unknown_plan_num_opp = 0;
                    for(int k = 0; k < num_of_action_opp; k++) {
                        int kati_opp = 0;
                        int make_opp = 0;
                        int hikiwake_opp = 0;
                        int makenasi_opp = 0;
                        int katinasi_opp = 0;
                        flg_l = 0;
                        //cout << "|";
                        for(int m = 0; m < num_of_un_opp; m++) {
                            p.make_posi_n(board_belief_opp, m); //第3者視点の配置を作る
                            int board_check_opp = p.make_action(actions_opp[k], 1 - vision, iteration);
                            if(board_check_opp == 0) { //まだ終わってない 表から値を取ってくる
                                if(which_table == 0) {
                                    p.make_posi_opponent(); //子供の相手視点の配置を作る
                                    unsigned long long int num_of_haiti_opp = p.getzddnum(*zdd_child); //子のZDDから子の配置の番号を取得
                                    int child_table_val_opp = child_table.get(num_of_haiti_opp); //子のtableから値を取得
                                    assert(child_table_val_opp >= 0 && child_table_val_opp <= 5);
                                    if(child_table_val_opp == 1) {
                                        kati_opp++; //元の親の視点プレイヤから見た必勝
                                        //cout << "w";
                                    } else if(child_table_val_opp == 2) {
                                        make_opp++; //元の親の視点プレイヤから見た必敗
                                        //cout << "l";
                                        // Posi p_test;
                                        // p_test.make_posi(num_of_haiti_opp, *zdd_child);
                                        //p.print();
                                        // p_test.print();
                                    } else if(child_table_val_opp == 3) {
                                        hikiwake_opp++; //元の親の視点プレイヤから見た引分
                                        //cout << "d";
                                    } else if(child_table_val_opp == 4) {
                                        makenasi_opp++; //元の親の視点プレイヤから見た負無し
                                        //cout << "nl";
                                    } else if(child_table_val_opp == 5) {
                                        katinasi_opp++; //元の親の視点プレイヤから見た勝無し
                                        //cout << "nw";
                                    }
                                } else {
                                    p.make_posi_myself();
                                    unsigned long long int num_of_haiti_opp = p.getzddnum(*zdd_child_opp); //子のZDDから子の配置の番号を取得
                                    int child_table_val_opp = opp_child_table.get(num_of_haiti_opp); //子のtableから値を取得
                                    assert(child_table_val_opp >= 0 && child_table_val_opp <= 5);
                                    if(child_table_val_opp == 1) {
                                        make_opp++; //元の親の視点プレイヤから見た必敗(visionが1なら白視点から見た負け)
                                        //cout << "l";
                                    } else if(child_table_val_opp == 2) {
                                        kati_opp++; //元の親の視点プレイヤから見た必勝
                                        //cout << "w";
                                        // Posi p_test;
                                        // p_test.make_posi(num_of_haiti_opp, *zdd_child);
                                        //p.print();
                                        // p_test.print();
                                    } else if(child_table_val_opp == 3) {
                                        hikiwake_opp++;
                                        //cout << "d";
                                    } else if(child_table_val_opp == 4) {
                                        katinasi_opp++;
                                        //cout << "nw";
                                    } else if(child_table_val_opp == 5) {
                                        makenasi_opp++;
                                        //cout << "nl";
                                    }
                                }
                            } else {
                                if(board_check_opp == 3) {
                                    //視点プレイヤ負け
                                    kati_opp++;
                                    //cout << "w";
                                } else if(board_check_opp == 2) {
                                    //視点プレイヤ勝ち
                                    make_opp++;
                                    //cout << "l";
                                } else if(board_check_opp == 10) {
                                    hikiwake_opp++;
                                    //cout << "d";
                                }
                            }
                        }
                        //cout << "|";
                        p.make_posi(division*i + surplus, *zdd_parent);
                        if(make_opp == num_of_un_opp) { //相手が必勝になる行動があった場合
                            if(iteration % 2 != vision) {
                                opp_win_count++;
                                flg_l = 1;
                                break;
                            } else {
                                win_plan_num_opp++;
                            }
                        } else if(make_opp + hikiwake_opp + katinasi_opp == num_of_un_opp) {
                            if(make_opp == 0 && katinasi_opp == 0) {
                                draw_plan_num_opp++;
                            } else {
                                nlose_plan_num_opp++;
                            }
                        } else { //if((kati_opp != num_of_un_opp) && (kati_opp + makenasi_opp + hikiwake_opp != num_of_un_opp))
                            unknown_plan_num_opp++;
                        }
                        if(k == num_of_action_opp - 1 && iteration % 2 != vision) {
                            if(nlose_plan_num_opp > 0 || draw_plan_num_opp > 0) {
                                opp_nlose_count++;
                                flg_l = 1;
                                break;
                            }
                        }
                        //cout << endl;
                    }
                    if(flg_l == 0) {
                        if(win_plan_num_opp == num_of_action_opp) {
                            flg_l = 1;
                            opp_win_count++;
                        } else if(win_plan_num_opp + draw_plan_num_opp + nlose_plan_num_opp == num_of_action_opp) {
                            // if(perfect_info_flg_opp == 1) {
                            //     if(perfect_info_flg != 1) {
                            //         flg_l = 1;
                            //         opp_nlose_count++;
                            //     }
                            // } else {
                            flg_l = 1;
                            opp_nlose_count++;
                            //}
                        }
                    }
                    if(flg_l == 0) break;
                }
                if(opp_win_count == num_of_un) {
                    nchild = -5; //必敗
                    if(iteration % 2 != vision) {
                        if(which_table == 0) {
                            me_mm_lose++;
                        } else {
                            me_em_lose++;
                        }
                    } else {
                        if(which_table == 0) {
                            mm_me_lose++;
                        } else {
                            mm_ee_lose++;
                        }
                    }
                    break;
                } else if(opp_win_count + opp_nlose_count == num_of_un) { //opp_win_count + opp_nlose_count + opp_draw_count
                    if(notlose_flg == 1) { //引分
                        nchild = -8;
                        all_draw++;
                        if(iteration % 2 == vision) {
                            if(which_ntlose == 1) {
                                mm_me_ntlose--;
                                //cout << "mm_me_ntlose--;" << i << endl;
                            } else if(which_ntlose == 2) {
                                mm_ee_ntlose--;
                                //cout << "mm_ee_ntlose--;" << i << endl;
                            }
                        } else {
                            if(which_ntlose == 3) {
                                me_mm_ntlose--;
                            } else if(which_ntlose == 4) {
                                me_em_ntlose--;
                            }
                        }
                        break;
                    } else {
                        nchild = -7; //勝無し
                        if(iteration % 2 != vision) {
                            if(which_table == 0) {
                                me_mm_ntwin++;
                            } else {
                                me_em_ntwin++;
                            }
                        } else {
                            if(which_table == 0) {
                                mm_me_ntwin++;
                            } else {
                                mm_ee_ntwin++;
                            }
                        }
                        break;
                    }
                }
            }

        }
        if(nchild > 0 && notlose_flg == 1) {
            nchild = -6;
        }
        if(nchild == -1 || nchild == -4) {
            parent_table.set(division*i + surplus, v_win);
            //cout << "win" << endl;
            nwin++;
        } else if(nchild == -2 || nchild == -5) {
            parent_table.set(division*i + surplus, v_lose);
            //cout << "lose" << endl;
            //if(i % 1000000 == 0) p.print();
            nlose++;
        } else if(nchild == -8) {
            parent_table.set(division*i + surplus, v_draw);
            //cout << "draw" << endl;
            ndraw++;
        } else if(nchild == -6) {
            parent_table.set(division*i + surplus, v_notlose);
            //cout << "notlose" << endl;
            nnotlose++;
        } else if(nchild == -7) {
            parent_table.set(division*i + surplus, v_notwin);
            //cout << "notwin" << endl;
            nnotwin++;
        } else {
            nunknown++;
            parent_table.set(division*i + surplus, v_unknown);
            //cout << "unknown" << endl;
        }
        if ((surplus == 0 && ((division*i + surplus) % 10000000ULL) == 0ULL) || (surplus == 1 && ((division*i + surplus) % 10000000ULL) == 1ULL) || (surplus == 2 && ((division*i + surplus) % 10000000ULL) == 2ULL) || (surplus == 3 && ((division*i + surplus) % 10000000ULL) == 3ULL)) { //1000000000ULL i
            cout << "id: " << division*i + surplus << ", nwin: " << nwin << ", nlose: " << nlose << ", ndraw: " << ndraw << ", nnotwin: " << nnotwin << ", nnotlose: " << nnotlose << ", nunknown: " << nunknown << ", perfect: " << perfect << ", imperfect: " << imperfect << endl;
        }
        // cout << "i = " << i << endl;
    }
    cout << "before_outtable" << endl;                                                                 // ***
    {   
        OutTable out_table(16 - iteration, read_filename_c_str, placement_count[vision][16 - iteration]);
        for (unsigned long long int i = 0; i < placement_count[vision][16 - iteration]; i++) { //haiti
            out_table.write(parent_table.get(i));
        }
        out_table.flush();
    }

    cout << "nwin  =  " << nwin << endl;
    cout << "nlose  =  " << nlose << endl;
    cout << "ndraw  =  " << ndraw << endl;
    cout << "nunknown  =  " << nunknown << endl;
    cout << "nnotwin  =  " << nnotwin << endl;
    cout << "nnotlose  =  " << nnotlose << endl;
    cout << "alwin = " << al_win_num << endl;
    cout << "allose = " << al_lose_num << endl;
    cout << "alunknown = " << al_unknown_num << endl;
    cout << "perfect = " << perfect << endl;
    cout << "imperfect = " << imperfect << endl;

    cout << "mm_me_win = " << mm_me_win << ", mm_ee_win = " << mm_ee_win << ", me_mm_win = " << me_mm_win << ", me_em_win = " << me_em_win << endl;
    cout << "mm_me_lose = " << mm_me_lose << ", mm_ee_lose = " << mm_ee_lose << ", me_mm_lose = " << me_mm_lose << ", me_em_lose = " << me_em_lose << endl;
    cout << "mm_me_ntwin = " << mm_me_ntwin << ", mm_ee_ntwin = " << mm_ee_ntwin << ", me_mm_ntwin = " << me_mm_ntwin << ", me_em_ntwin = " << me_em_ntwin << endl;
    cout << "mm_me_ntlose = " << mm_me_ntlose << ", mm_ee_ntlose = " << mm_ee_ntlose << ", me_mm_ntlose = " << me_mm_ntlose << ", me_em_ntlose = " << me_em_ntlose << endl;
    cout << "all_draw = " << all_draw << endl;
    
    return 0;
}

//  g++ -O2 -Wall -pthread main.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o maketable.out 2>&1
//  ./run_maktable.sh &
//  ./run_maketable_b.sh &

//  ./combtab.out 11 1 4 black_table black_table_11_4_0.bin black_table_11_4_1.bin black_table_11_4_2.bin black_table_11_4_3.bin db db db db db > 11_b_new2.txt 2>&1 &
//  ./combtab.out 11 0 4 white_table white_table_11_4_0.bin white_table_11_4_1.bin white_table_11_4_2.bin white_table_11_4_3.bin db db db db db > 11_w_new2.txt 2>&1 &

//  g++ -O2 -Wall -pthread CheckTable.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o checktable.out 2>&1
//  ./checktable.out 11 1 black_table_11.bin db > check_table_11_w.txt 2>&1 &

//  g++ -O2 -Wall -pthread notmaketable.cpp zdd_yonoku.cpp posi_yonoku.cpp table.cpp -std=c++14 -o notmaketable.out 2>&1
//  ./notmaketable.out 11 1 black_table_12.bin white_table_12.bin db db > notmaketab.txt 2>&1 &
