#ifndef INCLUDE_ZDD_YONOKU_H
#define INCLUDE_ZDD_YONOKU_H

#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <exception>

using std::max;
using std::min;
using std::deque;
using std::cout;
using std::endl;
using std::cerr;
using std::terminate;

//zddの節点
struct Node{
    unsigned char nb;   //現節点までの黒の駒の数
    unsigned char nw;   //現節点までの白の駒の数
    unsigned char uk;
    unsigned char f;    //そのマスにオブジェクトを置いたかどうか(0/1)(同じマスに2種類以上のオブジェクトは来ない)
    unsigned char nb_set1;
    unsigned char nw_set1;
    unsigned char nb_set2;
    unsigned char nw_set2;
    unsigned char nb_set3;
    unsigned char nw_set3;
    unsigned char nb_set4;
    unsigned char nw_set4;
    unsigned char nw_board;
    unsigned char nb_board;
    unsigned char f1;
    unsigned char f2;
    unsigned char f3;
    unsigned char de;
    unsigned char hand_id1;
    unsigned char hand_id2;
    int board[4][4] =
    {
        {-1, -1, -1, -1},
        {-1, -1, -1, -1},
        {-1, -1, -1, -1},
        {-1, -1, -1, -1}
    };
    int p1h[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    int p2h[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    unsigned long long int num; //その節点から到達できる葉が1になっている葉の数(盤面の配置の種類数)
    unsigned char visited; //経路長を求めるためのdfsのため
    unsigned char locid;    //現節点の盤面のマス番号(0-31)
    unsigned char loc_stateid;  //現節点のオブジェクト番号(0-7)
    int lengthmax;  //最大経路長ための値保持
    int lengthmin;  //最小経路長ための値保持
    unsigned long long int lengthsum;   //平均経路長ための合計の経路長
    Node *left; //0-枝
    Node *right; //1-枝
};


class ZDD_base { //使いまわせるやつをこの中に入れる
private:
    virtual int nw_set12_min() const noexcept { return -1; }
    virtual int nb_set12_min() const noexcept { return -1; }
    virtual bool ok_opp_set1_num_gb(const Node* n) const noexcept { return true; }
    virtual bool ok_opp_set1_num_gw(const Node* n) const noexcept { return true; }
    virtual bool ok_opp_set2_num_gb(const Node* n) const noexcept { return true; }
    virtual bool ok_opp_set2_num_gw(const Node* n) const noexcept { return true; }
    virtual int set12_board_num_x1(const Node* n) const noexcept { return -1; }
    virtual int set12_board_num_x0(const Node* n) const noexcept { return -1; }
    virtual int sub_wb_two_nhand() const noexcept { return -1; }
    virtual int sub_wb(const Node* n) const noexcept { return -1; }
    virtual int opp_col() const noexcept { return -1; }
    virtual int get_hand_id1() const noexcept { return -1; }
    virtual int get_hand_id2() const noexcept { return -1; }

protected:
    Node* m_root;
    Node* construct_zdd(int nmove) noexcept;
    bool IsNextLeaf0(const Node* n, int d, int x, int nmove) const noexcept;

public:
    ZDD_base(int nmove) {}
    virtual ~ZDD_base() { /* destruct_zdd() */; }
    void out() noexcept { cout << nw_set12_min() << endl; }
    //3種類の経路長を求める関数
    void dfs(Node* n) noexcept {
        if(n->left->visited != 1) dfs(n->left);
        if(n->right->visited != 1) dfs(n->right);
        n->num = n->left->num + n->right->num;
        n->lengthmax = max(n->left->lengthmax, n->right->lengthmax) + 1;         
        n->lengthmin = min(n->left->lengthmin, n->right->lengthmin) + 1;
        n->lengthsum = n->left->lengthsum + n->right->lengthsum + n->num;
        n->visited = 1;
    }
    void out_info() const noexcept {
        cout << "root->num = " << m_root->num << endl;
        cout << "root->lengthmax = " << m_root->lengthmax << endl;
        cout << "root->lengthmin = " << m_root->lengthmin << endl;
        cout << "root->lengthave = " << (double)m_root->lengthsum / (double)m_root->num << endl;
    }
    void compute_array(unsigned long long int x, unsigned char array_objid[32]) const noexcept {
        const Node *r = m_root;
        for(int i = 0; i < 32; i++) {
            array_objid[i] = 7;
        }
        while (true) {
            assert(r);
            if(r->f == 30) break; // もし１葉なら
            assert(r->f != 20);
            assert(r->f == 0 || r->f == 1);
            if(r->left->num <= x) {
                x -= r->left->num;
                assert(r->locid >= 0 && r->locid <= 31);
                array_objid[r->locid] = r->loc_stateid;
                r = r->right;
            } else {
                r = r->left;
            }
        }
    }
    unsigned long long int compute_id(unsigned char array_objid[32]) const noexcept {
        const Node* r = m_root;
        unsigned long long int index = 0;
        while(r->f < 20){
            assert(r->locid >= 0 && r->locid <= 31);
            if(array_objid[r->locid] == r->loc_stateid) {
                index += r->left->num;
                r = r->right;
            } else {
                r = r->left;
            }
        }
        return index;
    }
};

class ZDD_Black : public ZDD_base {
private:
    bool ok_opp_set1_num_gb(const Node* n) const noexcept override { return n->nb_set1 == 1; }
    bool ok_opp_set1_num_gw(const Node* n) const noexcept override { return n->nb_set1 == 0; }
    bool ok_opp_set2_num_gb(const Node* n) const noexcept override { return n->nb_set2 == 1; }
    bool ok_opp_set2_num_gw(const Node* n) const noexcept override { return n->nb_set2 == 0; }
    int set12_board_num_x1(const Node* n) const noexcept override { return n->nw_board + n->nw_set1 + n->nw_set2; }
    int set12_board_num_x0(const Node* n) const noexcept override { return n->nw_board + n->nw_set1 + n->nw_set2; }
    int nw_set12_min() const noexcept override { return 3; }
    int nb_set12_min() const noexcept override { return 1; }
    int sub_wb_two_nhand() const noexcept override { return 9; }
    int sub_wb(const Node* n) const noexcept override { return n->nw_board - n->nb_board; }
    int opp_col() const noexcept override { return 1; }
    int get_hand_id1() const noexcept override { return 16; }
    int get_hand_id2() const noexcept override { return 15; }
public:
    ZDD_Black(int nmove) noexcept : ZDD_base(nmove) { m_root = construct_zdd(nmove); }
    virtual ~ZDD_Black() noexcept {}
};

class ZDD_White : public ZDD_base {
private:
    bool ok_opp_set1_num_gb(const Node* n) const noexcept override { return n->nw_set1 == 0; }
    bool ok_opp_set1_num_gw(const Node* n) const noexcept override { return n->nw_set1 == 1; }
    bool ok_opp_set2_num_gb(const Node* n) const noexcept override { return n->nw_set2 == 0; }
    bool ok_opp_set2_num_gw(const Node* n) const noexcept override { return n->nw_set2 == 1; }
    int set12_board_num_x1(const Node* n) const noexcept override { return n->nb_board + 2 + n->nb_set1 + n->nb_set2; }
    int set12_board_num_x0(const Node* n) const noexcept override { return n->nb_board + n->nb_set1 + n->nb_set2; }
    int nw_set12_min() const noexcept override { return 1; }
    int nb_set12_min() const noexcept override { return 3; }
    int sub_wb_two_nhand() const noexcept override { return 10; }
    int sub_wb(const Node* n) const noexcept override { return n->nb_board - n->nw_board; }
    int opp_col() const noexcept override { return 2; }
    int get_hand_id1() const noexcept override { return 15; }
    int get_hand_id2() const noexcept override { return 16; }
public:
    ZDD_White(int nmove) noexcept : ZDD_base(nmove) { m_root = construct_zdd(nmove); }
    virtual ~ZDD_White() noexcept {}
};

#endif