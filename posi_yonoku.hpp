#ifndef POSI_YONOKU_H
#define POSI_YONOKU_H

struct LocInfo {
    unsigned int height;
    char cube[2];
        //unsigned int idx;
};

struct Action {
    int po; // 0, 1, 2, ..., 14, 15
    //char color1; // B, W
    int loc1; // 22, ..., 25 , ..... , 36, ... , 40
    int loc2; // 22, ..., 25 , ..... , 36, ... , 40
    //char color2; // B, W
    int loc3; // 22, ..., 25 , ..... , 36, ... , 40
};

class Posi {
private:
    LocInfo m_locs_info[46];
public:
    Posi() noexcept;
    Posi(unsigned long long int x, const ZDD_base& zdd) noexcept;
    void make_posi(unsigned long long int x, const ZDD_base& zdd) noexcept;
    void make_posi_n(unsigned char zdd_code[12][46], int n) noexcept;
    void make_posi_opponent() noexcept;
    void make_posi_myself() noexcept;
    void print() const noexcept;
    int compute_actions(Action actions[1000], int vision, int turn) noexcept;
    unsigned long long int getzddnum(const ZDD_base& zdd) const noexcept;
    int getobjnum(const LocInfo& a) noexcept;
    int getunknowninfo(unsigned char zdd_code[12][46], int vision, int turn) noexcept;
    bool is_opp_uninfo(unsigned char board_belief[12][46], int vision, int iteration) noexcept;
    void setunknown(int a) noexcept;
    bool exist_action() const noexcept;
    int FourCheck(int x, int y, int z) noexcept;
    int BoardCheck(int a, int b) noexcept;
    bool TegomaCheck() noexcept;
    bool check_ok() noexcept;
    int make_action(const Action & action, int vision, int turn) noexcept;
    //void unmake_action(const Action & action) noexcept;
    int BoardCheck2(int vision, int turn) noexcept;
};

#endif