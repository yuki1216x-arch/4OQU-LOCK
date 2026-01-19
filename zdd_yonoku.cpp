#include "zdd_yonoku.hpp"

//駒の状態を表す構造体(15種類)
struct LocInfo {
    char cube[2];
    unsigned char height;   //高さ
    unsigned char nb;       //黒の数
    unsigned char nw;       //白の数
    unsigned char uk;
    char top;               //一番上の駒の種類

    //与えられたオブジェクトが自身と同じかどうかを判定する関数
    bool equal(const LocInfo &o) const noexcept {
        return (cube[0] == o.cube[0] && cube[1] == o.cube[1] && /*cube[2] == o.cube[2]  &&*/ height == o.height);
    }
};

constexpr LocInfo tbl_objid2locinfo[8] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'W', 'W'}, 2, 0, 2, 0, 'W'}, //3
     { {'W', 'B'}, 2, 1, 1, 0, 'B'}, //4
     { {'B', 'W'}, 2, 1, 1, 0, 'W'}, //5
     { {'B', 'B'}, 2, 2, 0, 0, 'B'}, //6
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //7  
};

constexpr LocInfo tbl_objid2locinfo2[5] = {
     { {'.', '.'}, 0, 0, 0, 0, '.'}, //0
     { {'W', '.'}, 1, 0, 1, 0, 'W'}, //1
     { {'B', '.'}, 1, 1, 0, 0, 'B'}, //2
     { {'U', '.'}, 1, 0, 0, 1, 'U'}, //3
     { {'N', 'N'}, 99, 99, 99, 99, 'E'}  //4  
};

//節点nからx(0/1)枝に行ったときに、その節点が0かどうかの判定
//d:nの深さ
bool ZDD_base::IsNextLeaf0(const Node* n, int d, int x, int nmove) const noexcept {
    assert(n);
    assert(0 <= d && d <= 167);
    assert(x == 0 || x == 1);
    assert(0 <= nmove && nmove < 16);
    int nhand = 16 - nmove; //後何手で終了か

    if(x == 1 && n->f == 1) return true; //objが置いてあるのに置こうとしている
    if(d < 12) { //相手側(黒プレイヤ)の手ゴマ(set1, 左)
        if(n->f == 1 && x == 1) return true; //objが置いてあるのに置こうとしている
        if(x == 1 && n->loc_stateid != 0 && n->hand_id1 > nhand) return true; //マス番号(残り何手か)よりも大きいマスに空白以外は置けない
        if(x == 1 && n->loc_stateid == 0 && n->hand_id1 <= nhand) return true;
        if(x == 1 && n->loc_stateid == 0 && n->f1 == 1) return true; //コマ置いたフラグが立っているのに空白を置いてはいけない
        if(x == 1 && n->loc_stateid == 1 && n->nw_set1 >= nw_set12_min()) return true; //白手番の時に、set1に白が1つあるのにもう1つ置こうとしてはいけない
        if(x == 1 && n->loc_stateid == 2 && n->nb_set1 >= nb_set12_min()) return true; //白手番の時に、set1に黒が3つあるのにもう1つ置こうとしてはいけない
        if(x == 0 && d % 3 == 2 && n->f == 0) return true; //何も置かないのはだめ
        return false;
    }
    if (d < 24) { //相手側(黒プレイヤ)の手ゴマ(set2, 右)
        if(n->f == 1 && x == 1) return true; //objが置いてあるのに置こうとしている
        if(x == 1 && n->loc_stateid != 0 && n->hand_id1 > nhand) return true;
        if(x == 1 && n->loc_stateid == 0 && n->hand_id1 <= nhand) return true;
        if(x == 1 && n->loc_stateid == 0 && n->f1 == 1) return true;
        if(x == 1 && n->loc_stateid == 1 && n->nw_set2 >= nw_set12_min()) return true;
        if(x == 1 && n->loc_stateid == 2 && n->nb_set2 >= nb_set12_min()) return true;
        if(x == 0 && d % 3 == 2 && n->f == 0) return true;
        return false;
    }
    if(d < 136) { //盤上
        if(x == 1 && n->f == 1) return true;

        if(x == 0 && (d - 24) % 7 == 6 && n->f == 0) return true;
        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && (n->nb_board + 2) - n->nw_board > 1 && ok_opp_set1_num_gb(n)) return true; //黒の方が2個多い
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && n->nb_board - n->nw_board > 1 && ok_opp_set1_num_gb(n)) return true; //黒の方が2個多い
        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && n->nw_board - (n->nb_board + 2) > 1 && ok_opp_set1_num_gw(n)) return true; //白の方が2個多い
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && n->nw_board - n->nb_board > 1 && ok_opp_set1_num_gw(n)) return true; //白の方が2個多い

        if(d == 135 && x == 1 && n->f == 0 && nhand <= 8 && (n->nb_board + 2) - n->nw_board > 1 && ok_opp_set2_num_gb(n)) return true; //黒の方が2個多い
        if(d == 135 && x == 0 && n->f == 1 && nhand <= 8 && n->nb_board - n->nw_board > 1 && ok_opp_set2_num_gb(n)) return true; //黒の方が2個多い
        if(d == 135 && x == 1 && n->f == 0 && nhand <= 8 && n->nw_board - (n->nb_board + 2) > 1 && ok_opp_set2_num_gw(n)) return true; //白の方が2個多い
        if(d == 135 && x == 0 && n->f == 1 && nhand <= 8 && n->nw_board - n->nb_board > 1 && ok_opp_set2_num_gw(n)) return true; //白の方が2個多い

        if(d == 135 && x == 1 && n->f == 0 && nhand >= sub_wb_two_nhand() && 8 - set12_board_num_x1(n) > 2) return true; //10
        if(d == 135 && x == 0 && n->f == 1 && nhand >= sub_wb_two_nhand() && 8 - set12_board_num_x0(n) > 2) return true;
        if(d == 135 && x == 1 && n->f == 0 && nhand < sub_wb_two_nhand() && 8 - set12_board_num_x1(n) > 1) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand < sub_wb_two_nhand() && 8 - set12_board_num_x0(n) > 1) return true;

        if(d == 135 && x == 1 && n->f == 0 && ((n->nw_board - n->nb_board > 5) || (n->nb_board - n->nw_board > 1))) return true;
        if(d == 135 && x == 0 && n->f == 1 && ((n->nw_board - n->nb_board > 3) || (n->nb_board - n->nw_board > 3))) return true;

        if(d == 135 && x == 1 && n->f == 0 && nhand == 8 && !((n->nw_board == 4) && (n->nb_board == 2))) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand == 8 && !((n->nw_board == 4) && (n->nb_board == 4))) return true;

        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && ((n->nw_board > 4) || (n->nb_board > 2))) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && ((n->nw_board > 4) || (n->nb_board > 4))) return true;
        if(d == 135 && x == 1 && n->f == 0 && nhand <= 8 && ((n->nw_board < 4) || (n->nb_board < 2))) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand <= 8 && ((n->nw_board < 4) || (n->nb_board < 4))) return true;

        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && ((n->nw_board + n->nw_set1 > 4) || (n->nb_board + n->nb_set1 > 2))) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && ((n->nw_board + n->nw_set1 > 4) || (n->nb_board + n->nb_set1 > 4))) return true;

        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && (nw_set12_min() - n->nw_set1) > n->nw_board) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && (nw_set12_min() - n->nw_set1) > n->nw_board) return true;
        if(d == 135 && x == 1 && n->f == 0 && nhand > 8 && (nb_set12_min() - n->nb_set1) > n->nb_board + 2) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand > 8 && (nb_set12_min() - n->nb_set1) > n->nb_board) return true;

        if(d == 135 && x == 1 && n->f == 0 && nhand <= 8 && 4 + (nw_set12_min() - n->nw_set2) > n->nw_board) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand <= 8 && 4 + (nw_set12_min() - n->nw_set2) > n->nw_board) return true;
        if(d == 135 && x == 1 && n->f == 0 && nhand <= 8 && 4 + (nb_set12_min() - n->nb_set2) > n->nb_board + 2) return true;
        if(d == 135 && x == 0 && n->f == 1 && nhand <= 8 && 4 + (nb_set12_min() - n->nb_set2) > n->nb_board) return true;
        return false;
    }
    if(d < 152) { //自分側(白プレイヤ)の手ゴマ(set3, 右)
        if(x == 1 && n->f == 1) return true;
        if(x == 1 && n->loc_stateid != 0 && n->hand_id2 > nhand) return true;
        if(x == 1 && n->loc_stateid == 0 && n->hand_id2 <= nhand) return true;
        if(x == 1 && n->loc_stateid == 0 && n->f2 == 1) return true;
        if(x == 1 && n->loc_stateid == 1 && n->nw_board - (nw_set12_min() - n->nw_set1) - (nw_set12_min() - n->nw_set2) >= nb_set12_min()) return true; //n->nw_board - (1 - n->nw_set1) - (1 - n->nw_set2)が自分が置いた白の数
        if(x == 1 && n->loc_stateid == 2 && n->nb_board - (nb_set12_min() - n->nb_set1) - (nb_set12_min() - n->nb_set2) >= nw_set12_min()) return true; //n->nb_board - (3 - n->nb_set1) - (3 - n->nb_set2)
        if(x == 1 && n->loc_stateid == opp_col() && sub_wb(n) > 1) return true;

        if(x == 1 && n->loc_stateid == opp_col() && set12_board_num_x0(n) == 7) return true;

        if(x == 1 && (n->loc_stateid == 1 || n->loc_stateid == 2) && n->f3 == 1) return true; //unknownの後に白/黒のコマを置いてはいけない
        if(x == 1 && n->loc_stateid == 3 && n->f3 == 0) return true; //白/黒のコマが置かれてないのにunknownを置いてはいけない
        if(x == 0 && (d - 136) % 4 == 3 && n->f == 0) return true;
        return false;
    }

    //自分側(白プレイヤ)の手ゴマ(set4, 左)
    if(x == 1 && n->f == 1) return true;
    if(x == 1 && n->loc_stateid != 0 && n->hand_id2 > nhand) return true;
    if(x == 1 && n->loc_stateid == 0 && n->hand_id2 <= nhand) return true;
    if(x == 1 && n->loc_stateid == 0 && n->f2 == 1) return true;
    if(x == 1 && n->loc_stateid == 1 && n->nw_board - (nw_set12_min() - n->nw_set1) - (nw_set12_min() - n->nw_set2) - nb_set12_min() >= nb_set12_min()) return true; // n->nw_board - (1 - n->nw_set1) - (1 - n->nw_set2) - 3
    if(x == 1 && n->loc_stateid == 2 && n->nb_board - (nb_set12_min() - n->nb_set1) - (nb_set12_min() - n->nb_set2) - nw_set12_min() >= nw_set12_min()) return true; //n->nb_board - (3 - n->nb_set1) - (3 - n->nb_set2) - 1
    if(x == 1 && n->loc_stateid == opp_col() && sub_wb(n) > 1) return true;
    if(x == 1 && n->loc_stateid == opp_col() && set12_board_num_x0(n) == 8) return true;
    if(x == 1 && (n->loc_stateid == 1 || n->loc_stateid == 2) && n->f3 == 1) return true;
    if(x == 1 && n->loc_stateid == 3 && n->f3 == 0) return true;
    if(x == 0 && (d - 136) % 4 == 3 && n->f == 0) return true;

    if(d == 167 && x == 1 && n->f == 0 && !(n->nb + n->nw + n->uk == 15 && n->nb <= 8 && n->nw <= 8)) return true;
    if(d == 167 && x == 0 && n->f == 1 && !(n->nb + n->nw + n->uk == 16 && n->nb <= 8 && n->nw <= 8)) return true;
    return false;
}

//zddを作る関数
Node* ZDD_base::construct_zdd(int nmove) noexcept {
    Node* root;                 //根節点

    int d = -1;                 //深さ
    deque<Node*> N[168];        //N[i]:深さiの節点集合
    root = new Node;            
    Node* l0 = new Node;        //0葉
    Node* l1 = new Node;        //1葉
    Node* n;

    //初期化
    root->f = 0;
    root->f1 = 0;
    root->f2 = 0;
    root->f3 = 0;
    root->nb = 0;
    root->nw = 0;
    root->uk = 0;
    root->de = 0;
    root->nw_set1 = 0;
    root->nb_set1 = 0;
    root->nw_set2 = 0;
    root->nb_set2 = 0;
    root->nw_set3 = 0;
    root->nb_set3 = 0;
    root->nw_set4 = 0;
    root->nb_set4 = 0;
    root->nw_board = 0;
    root->nb_board = 0;
    root->num = 0;
    root->loc_stateid = 0;
    root->locid = 0;
    root->hand_id1 = get_hand_id1();
    root->hand_id2 = get_hand_id2();
    l0->f = 20;
    l0->f1 = 20;
    l0->f2 = 20;
    l0->f3 = 20;
    l0->nb = 20;
    l0->nw = 20;
    l0->uk = 20;
    l0->de = 20;
    l0->left = NULL;
    l0->right = NULL;
    l0->nw_set1 = 20;
    l0->nb_set1 = 20;
    l0->nw_set2 = 20;
    l0->nb_set2 = 20;
    l0->nw_set3 = 20;
    l0->nb_set3 = 20;
    l0->nw_set4 = 20;
    l0->nb_set4 = 20;
    l0->nw_board = 20;
    l0->nb_board = 20;
    l0->num = 0;
    l0->visited = 1;
    l0->lengthmax = -500;
    l0->lengthmin = 500;
    l0->lengthsum = 0;
    l0->hand_id1 = 20;
    l0->hand_id2 = 20;

    l1->f = 30;
    l1->f1 = 30;
    l1->f2 = 30;
    l1->f3 = 30;
    l1->nb = 30;
    l1->nw = 30;
    l1->uk = 30;
    l1->de = 30;
    l1->left = NULL;
    l1->right = NULL;
    l1->nw_set1 = 30;
    l1->nb_set1 = 30;
    l1->nw_set2 = 30;
    l1->nb_set2 = 30;
    l1->nw_set3 = 30;
    l1->nb_set3 = 30;
    l1->nw_set4 = 30;
    l1->nb_set4 = 30;
    l1->nw_board = 30;
    l1->nb_board = 30;
    l1->num = 1;
    l1->visited = 1;
    l1->lengthmax = 0;
    l1->lengthmin = 0;
    l1->lengthsum = 0;
    l1->hand_id1 = 30;
    l1->hand_id2 = 30;

    N[0].push_back(root);   //N[0]に根節点を入れる
    for(int i = 0; i < 32; i++){    //各マスに対するループ(30マス分)(俺は16マス分)
        //cout << i << endl;
        if(i < 8) { //黒プレイヤの手ゴマ
            for(int j = 0; j < 3; j++){    //各オブジェクトに対するループ(15種類)(俺は7種類)
                //cout << j << endl;
                d++;                        //深さを+1
                for(size_t k = 0; k < N[d].size(); k++){    //深さdの節点集合に対するループ
                    //cout <<  << endl;
                    n = N[d][k];            //要素(節点)を一つ取り出す
                    for(int x = 0; x < 2; x++){     //0枝、1枝のループ
                        if(IsNextLeaf0(n, d, x, nmove)){ //もし節点nからx-枝に進んだときに0-葉なら
                            if(x == 0) n->left = l0;
                            else n->right = l0;
                        }else{                    //子供に節点をもつ時
                            //cout << "abcd" << endl;
                            Node *c = new Node;   //子どもの初期化
                            if(j == 2) c->locid = i + 1;   //次のマスへ
                            else c->locid = i;

                            if(j == 2) c->loc_stateid = 0; //オブジェクト番号を0に戻す
                            else c->loc_stateid = j + 1;    //次の種類のオブジェクト

                            if(j == 2) c->hand_id1 = n->hand_id1 - 2;
                            else c->hand_id1 = n->hand_id1;

                            c->hand_id2 = n->hand_id2;
                            c->f = 0;   //ひとまず0
                            c->f1 = n->f1;
                            c->f2 = n->f2;
                            c->f3 = n->f3; //f2やった
                            c->de = n->de + 1;

                            if(j != 2 && (n->f == 1 || x == 1)) {
                                c->f = 1;  //n->f:親の状況を引継ぎ, x==1なら親のオブジェクトが置かれた
                            }

                            

                            if(j != 0 && x == 1) c->f1 = 1;
                            
                            c->nb = n->nb;  //この時点までの盤面の駒の数を引き継ぐ
                            c->nw = n->nw;
                            c->uk = n->uk; 
                            
                            

                            c->nb_set1 = n->nb_set1;
                            c->nw_set1 = n->nw_set1;
                            c->nb_set2 = n->nb_set2;
                            c->nw_set2 = n->nw_set2;
                            c->nb_set3 = n->nb_set3;
                            c->nw_set3 = n->nw_set3;
                            c->nb_set4 = n->nb_set4;
                            c->nw_set4 = n->nw_set4;
                            c->nw_board = n->nw_board;
                            c->nb_board = n->nb_board;

                            for(int p = 0; p < 8; p++) {
                                c->p1h[p] = n->p1h[p];
                                c->p2h[p] = n->p2h[p];
                            }
                            

                            if(x == 1) {
                                if(i < 4) {
                                    c->nw_set1 = c->nw_set1 + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb_set1 = c->nb_set1 + tbl_objid2locinfo2[j].nb;
                                    c->nw = c->nw + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb = c->nb + tbl_objid2locinfo2[j].nb;
                                    c->uk = c->uk + tbl_objid2locinfo2[j].uk;
                                    c->p1h[i] = j;
                                } else {
                                    c->nw_set2 = c->nw_set2 + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb_set2 = c->nb_set2 + tbl_objid2locinfo2[j].nb;
                                    c->nw = c->nw + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb = c->nb + tbl_objid2locinfo2[j].nb;
                                    c->uk = c->uk + tbl_objid2locinfo2[j].uk;
                                    c->p1h[i] = j;
                                }
                                      
                            }
                            

                            c->num = 0;
                            c->visited = 0;

                                                      

                            if(N[d+1].size() == 0){     //次の深さの節点集合がまだないとき
                                   
                                N[d+1].push_back(c);
                                if(x == 0) n->left = c; //親と子をつなぐ
                                else n->right = c;
                            }else{  //2個目以降の節点の時
                                for(size_t l = 0; l < N[d+1].size(); l++){
                                    
                                    if(N[d+1][l]->f == c->f && N[d+1][l]->nb_set1 == c->nb_set1 && N[d+1][l]->nb_set2 == c->nb_set2 && N[d+1][l]->nb_set3 == c->nb_set3 && N[d+1][l]->nb_set4 == c->nb_set4 && N[d+1][l]->nw_set1 == c->nw_set1 && N[d+1][l]->nw_set2 == c->nw_set2 && N[d+1][l]->nw_set3 == c->nw_set3 && N[d+1][l]->nw_set4 == c->nw_set4 && N[d+1][l]->nb == c->nb && N[d+1][l]->nw == c->nw){
                                        
                                        if(x == 0) n->left = N[d+1][l];
                                        else n->right = N[d+1][l];
                                        delete c;
                                        break;
                                    }else if(l == N[d+1].size() - 1){
                                        N[d+1].push_back(c);
                                        if(x == 0) n->left = c;
                                        else n->right = c;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if(i < 24) {        //以下盤面について
            for(int j = 0; j < 7; j++){    //各オブジェクトに対するループ(15種類)(俺は7種類)
                //cout << "j:" << j << endl;
                d++;                       //深さを+1
                for(size_t k = 0; k < N[d].size(); k++){    //深さdの節点集合に対するループ
                    n = N[d][k];            //要素(節点)を一つ取り出す
                    for(int x = 0; x < 2; x++){     //0枝、1枝のループ
                        if(IsNextLeaf0(n, d, x, nmove)){ //もし節点nからx-枝に進んだときに0-葉なら
                            if(x == 0) n->left = l0;
                            else n->right = l0;
                        }else if(d == -1){       //深さが最大の時は1葉
                            if(x == 0) n->left = l1;
                            else n->right = l1;
                        }else{                    //子供に節点をもつ時
                            Node *c = new Node;   //子どもの初期化
                            if(j == 6) c->locid = i + 1;   //次のマスへ
                            else c->locid = i;

                            if(j == 6) c->loc_stateid = 0; //オブジェクト番号を0に戻す
                            else c->loc_stateid = j + 1;    //次の種類のオブジェクト

                            c->f = 0;   //ひとまず0
                            c->f1 = n->f1;
                            c->f2 = n->f2;
                            c->f3 = n->f3;
                            c->f3 = n->f3;
                            c->de = n->de + 1;
                            c->hand_id1 = n->hand_id1;
                            c->hand_id2 = n->hand_id2;

                            if(j != 6 && (n->f == 1 || x == 1)) c->f = 1;  //n->f:親の状況を引継ぎ, x==1なら親のオブジェクトが置かれた
                            
                            c->nb = n->nb;  //この時点までの盤面の駒の数を引き継ぐ
                            c->nw = n->nw;
                            c->uk = n->uk;
                            c->nw_board = n->nw_board;
                            c->nb_board = n->nb_board;

                            for(int p = 0; p < 8; p++) {
                                c->p1h[p] = n->p1h[p];
                                c->p2h[p] = n->p2h[p];
                            }

                            for(int p = 0; p < 4; p++) {
                                for(int q = 0; q < 4; q++) {
                                    c->board[p][q] = n->board[p][q];
                                }
                            }
                            
                            if(x == 1){     //yesで来た時
                                c->nw = c->nw + tbl_objid2locinfo[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                c->nb = c->nb + tbl_objid2locinfo[j].nb;
                                c->nw_board = c->nw_board + tbl_objid2locinfo[j].nw;
                                c->nb_board = c->nb_board + tbl_objid2locinfo[j].nb;
                                c->board[(i - 8) / 4][(i - 8) % 4] = j;
                                //if(d == 154) cout << (int)c->nw << " " << (int)c->nb << endl;
                            }


                            
                            c->nb_set1 = n->nb_set1;
                            c->nw_set1 = n->nw_set1;
                            c->nb_set2 = n->nb_set2;
                            c->nw_set2 = n->nw_set2;
                            c->nb_set3 = n->nb_set3;
                            c->nw_set3 = n->nw_set3;
                            c->nb_set4 = n->nb_set4;
                            c->nw_set4 = n->nw_set4;

                            c->num = 0;
                            c->visited = 0;

                            //ここまでやったpart3

                            if(N[d+1].size() == 0){     //次の深さの節点集合がまだないとき
                                N[d+1].push_back(c);    //次の深さの節点集合に追加
                                if(x == 0) n->left = c; //親と子をつなぐ
                                else n->right = c;
                            }else{  //2個目以降の節点の時
                                for(size_t l = 0; l < N[d+1].size(); l++){
                                    //以下if文内で等価節点があったら(条件は論文内)
                                    if(N[d+1][l]->f == c->f && N[d+1][l]->nb == c->nb && N[d+1][l]->nw == c->nw && N[d+1][l]->nb - N[d+1][l]->nb_set1 - N[d+1][l]->nb_set2 - N[d+1][l]->nb_set3 - N[d+1][l]->nb_set4 == c->nb - c->nb_set1 - c->nb_set2 - c->nb_set3 - c->nb_set4 && N[d+1][l]->nw - N[d+1][l]->nw_set1 - N[d+1][l]->nw_set2 - N[d+1][l]->nw_set3 - N[d+1][l]->nw_set4 == c->nw - c->nw_set1 - c->nw_set2 - c->nw_set3 - c->nw_set4){

                                        if(x == 0) n->left = N[d+1][l];
                                        else n->right = N[d+1][l];
                                        delete c;
                                        break;
                                    }else if(l == N[d+1].size() - 1){
                                        N[d+1].push_back(c);
                                        if(x == 0) n->left = c;
                                        else n->right = c;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            for(int j = 0; j < 4; j++){    //各オブジェクトに対するループ(15種類)(俺は7種類)
                //cout << "j:" << j << endl;
                d++;                       //深さを+1
                for(size_t k = 0; k < N[d].size(); k++){    //深さdの節点集合に対するループ
                    n = N[d][k];            //要素(節点)を一つ取り出す
                    for(int x = 0; x < 2; x++){     //0枝、1枝のループ
                        if(IsNextLeaf0(n, d, x, nmove)){ //もし節点nからx-枝に進んだときに0-葉なら
                            if(x == 0) n->left = l0;
                            else n->right = l0;
                        }else if(d == 167){       //深さが最大の時は1葉
                            if(x == 0) n->left = l1;
                            else n->right = l1;
                        }else{                    //子供に節点をもつ時
                            Node *c = new Node;   //子どもの初期化
                            if(j == 3) c->locid = i + 1;   //次のマスへ
                            else c->locid = i;

                            if(j == 3) c->loc_stateid = 0; //オブジェクト番号を0に戻す
                            else c->loc_stateid = j + 1;    //次の種類のオブジェクト

                            if(j == 3) c->hand_id2 = n->hand_id2 - 2;
                            else c->hand_id2 = n->hand_id2;

                            c->f = 0;   //ひとまず0
                            c->f1 = n->f1;
                            c->f2 = n->f2;
                            c->f3 = n->f3;
                            c->de = n->de + 1;
                            c->hand_id1 = n->hand_id1;

                            if(j != 3 && (n->f == 1 || x == 1)) c->f = 1;  //n->f:親の状況を引継ぎ, x==1なら親のオブジェクトが置かれた

                            if(j != 0 && x == 1) { //j != 3 && x == 1
                                c->f2 = 1;
                            }

                            if(x == 1 && (j == 1 || j == 2)) c->f3 = 1;
                            
                            c->nb = n->nb;  //この時点までの盤面の駒の数を引き継ぐ
                            c->nw = n->nw;
                            c->uk = n->uk;

                            c->nb_set1 = n->nb_set1;
                            c->nw_set1 = n->nw_set1;
                            c->nb_set2 = n->nb_set2;
                            c->nw_set2 = n->nw_set2;
                            c->nb_set3 = n->nb_set3;
                            c->nw_set3 = n->nw_set3;
                            c->nb_set4 = n->nb_set4;
                            c->nw_set4 = n->nw_set4;
                            c->nw_board = n->nw_board;
                            c->nb_board = n->nb_board;

                            for(int p = 0; p < 8; p++) {
                                c->p1h[p] = n->p1h[p];
                                c->p2h[p] = n->p2h[p];
                            }

                            for(int p = 0; p < 4; p++) {
                                for(int q = 0; q < 4; q++) {
                                    c->board[p][q] = n->board[p][q];
                                }
                            }
                            

                            //P2
                            if(x == 1) {
                                if(i < 28) {
                                    //cout << k << ", " << i << ", " << j << endl;
                                    c->nw_set3 = c->nw_set3 + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb_set3 = c->nb_set3 + tbl_objid2locinfo2[j].nb;
                                    c->nw = c->nw + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb = c->nb + tbl_objid2locinfo2[j].nb;
                                    c->uk = c->uk + tbl_objid2locinfo2[j].uk;
                                    c->p2h[i - 24] = j;   
                                } else {
                                    //cout << k << ", " << i << ", " << j << endl;
                                    c->nw_set4 = c->nw_set4 + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb_set4 = c->nb_set4 + tbl_objid2locinfo2[j].nb;
                                    c->nw = c->nw + tbl_objid2locinfo2[j].nw;    //右辺2項目が親で置かれたオブジェクトの色の数
                                    c->nb = c->nb + tbl_objid2locinfo2[j].nb;
                                    c->uk = c->uk + tbl_objid2locinfo2[j].uk;
                                    c->p2h[i - 24] = j;
                                }
                            }



                            c->num = 0;
                            c->visited = 0;

                            if(N[d+1].size() == 0){     //次の深さの節点集合がまだないとき
                                N[d+1].push_back(c);    //次の深さの節点集合に追加
                                if(x == 0) n->left = c; //親と子をつなぐ
                                else n->right = c;
                            }else{  //2個目以降の節点の時
                                for(size_t l = 0; l < N[d+1].size(); l++){
                                    //以下if文内で等価節点があったら(条件は論文内)
                                    if(N[d+1][l]->f == c->f && N[d+1][l]->nb == c->nb && N[d+1][l]->uk == c->uk && N[d+1][l]->nw == c->nw && N[d+1][l]->nb - N[d+1][l]->nb_set1 - N[d+1][l]->nb_set2 - N[d+1][l]->nb_set3 - N[d+1][l]->nb_set4 == c->nb - c->nb_set1 - c->nb_set2 - c->nb_set3 - c->nb_set4 && N[d+1][l]->nw - N[d+1][l]->nw_set1 - N[d+1][l]->nw_set2 - N[d+1][l]->nw_set3 - N[d+1][l]->nw_set4 == c->nw - c->nw_set1 - c->nw_set2 - c->nw_set3 - c->nw_set4 && N[d+1][l]->nb_set1 == c->nb_set1 && N[d+1][l]->nb_set2 == c->nb_set2 && N[d+1][l]->nb_set3 == c->nb_set3 && N[d+1][l]->nb_set4 == c->nb_set4 && N[d+1][l]->nw_set1 == c->nw_set1 && N[d+1][l]->nw_set2 == c->nw_set2 && N[d+1][l]->nw_set3 == c->nw_set3 && N[d+1][l]->nw_set4 == c->nw_set4){
                                        if(x == 0) n->left = N[d+1][l];
                                        else n->right = N[d+1][l];
                                        delete c;
                                        break;
                                    }else if(l == N[d+1].size() - 1){
                                        N[d+1].push_back(c);
                                        if(x == 0) n->left = c;
                                        else n->right = c;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
    }
    int end = 1;
    //冗長節点の削除

    while(end == 1){
        end = 0;
        for(int i = 0; i < 167; i++){
            for(size_t j = 0; j < N[i].size(); j++){
                //cout << end << endl;
                if(N[i][j]->left->f != 20 && N[i][j]->left->f != 30 && N[i][j]->f != 40){
                    if(N[i][j]->left->right->f == 20){
                        end = 1;
                        N[i][j]->left->f = 40;
                        N[i][j]->left = N[i][j]->left->left;
                    }
                }
                //cout << end << endl;
                if(N[i][j]->right->f != 20 && N[i][j]->right->f != 30 && N[i][j]->f != 40){
                    if(N[i][j]->right->right->f == 20){
                        end = 1;
                        N[i][j]->right->f = 40;
                        N[i][j]->right = N[i][j]->right->left;
                    }
                }
            }
        }
    }
    //cout << "12345" << endl;
    for(int i = 0; i < 168; i++){
        for(size_t j = 0; j < N[i].size(); j++){
            if(N[i][j]->f == 40){
                N[i].erase(N[i].begin() + j);
                j--;
            }
        }
    }
    
    dfs(root);
    
    cout << "root->num = " << root->num << endl;
    cout << "root->lengthmax = " << root->lengthmax << endl;
    cout << "root->lengthmin = " << root->lengthmin << endl;
    cout << "root->lengthave = " << (double)root->lengthsum / (double)root->num << endl;

    int sum = 0;

    for(int i = 0; i < 168; i++){
        //cout << "N[" << i << "]" << N[i].size() << endl;
        sum += N[i].size();
    }
    cout << "sumad = " << sum << endl;
    
    return root;
}
