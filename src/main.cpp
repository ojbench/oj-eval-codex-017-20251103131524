#include <bits/stdc++.h>

// NOTE: Per problem, avoid STL containers except std::string and algorithm.
// Here we will restrict ourselves: use only std::string and minimal C APIs.
// To stay safe, we'll implement simple fixed-size arrays and a tiny hash.

using std::string;

// Simple utilities
static inline void trim(string &s) {
    while (!s.empty() && (s.back()=='\n' || s.back()=='\r' || s.back()==' ' || s.back()=='\t')) s.pop_back();
    size_t i=0; while (i<s.size() && (s[i]==' '||s[i]=='\t')) ++i; if (i) s.erase(0,i);
}

static inline int parse_int(const string &s) {
    int x=0; bool neg=false; size_t i=0; if (!s.empty() && s[0]=='-'){neg=true;i=1;}
    for (; i<s.size(); ++i) { char c=s[i]; if (c<'0'||c>'9') break; x = x*10 + (c-'0'); }
    return neg? -x : x;
}

// Simple split by space (keep tokens)
static inline int split(const string &s, string *out, int maxn) {
    int n=0; size_t i=0, L=s.size();
    while (i<L && n<maxn) {
        while (i<L && s[i]==' ') ++i;
        if (i>=L) break;
        size_t j=i; while (j<L && s[j] != ' ') ++j;
        out[n++] = s.substr(i, j-i);
        i=j;
    }
    return n;
}

// Split by delimiter '|'
static inline int split_by(const string &s, char delim, string *out, int maxn) {
    int n=0; size_t i=0, L=s.size();
    size_t start=0;
    while (start<=L && n<maxn) {
        size_t pos = s.find(delim, start);
        if (pos==string::npos) {
            out[n++] = s.substr(start);
            break;
        } else {
            out[n++] = s.substr(start, pos-start);
            start = pos+1;
        }
    }
    return n;
}

// Date/time helpers
struct Date { int m; int d; };
struct HM { int h; int m; };

static inline Date parse_date(const string &s) { // mm-dd
    Date res{0,0};
    if (s.size()>=5) {
        res.m = (s[0]-'0')*10 + (s[1]-'0');
        res.d = (s[3]-'0')*10 + (s[4]-'0');
    }
    return res;
}
static inline HM parse_hm(const string &s) { // hh:mi
    HM r{0,0};
    if (s.size()>=5) {
        r.h=(s[0]-'0')*10+(s[1]-'0');
        r.m=(s[3]-'0')*10+(s[4]-'0');
    }
    return r;
}

// Convert base date (mm-dd) + minutes offset to absolute mm-dd hh:mm across months June-August 2021
static inline void add_minutes(Date base, int base_h, int base_mi, long long add, int &out_m, int &out_d, int &out_h, int &out_min) {
    static const int MD[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    long long total_min = base_h*60 + base_mi + add;
    long long days = total_min / 60 / 24;
    int rem = (int)(total_min - days*24*60);
    if (rem<0){ rem += 24*60; days--; }
    int h = rem/60; int mi = rem%60;
    int m = base.m; int d = base.d;
    // advance days respecting month lengths, assume months 06..08 valid
    while (days>0) {
        int mdays = MD[m];
        if (d < mdays) { d++; days--; }
        else { // d==mdays, go to next month
            m++; d=1; days--; }
    }
    while (days<0) {
        if (d>1) { d--; days++; }
        else { // go to prev month
            m--; if (m<1) m=1; d = MD[m]; days++; }
    }
    out_m=m; out_d=d; out_h=h; out_min=mi;
}

static inline void print_time_md_hm(int m, int d, int h, int mi) {
    auto two=[&](int x){ if(x<10) putchar('0'); printf("%d", x); };
    two(m); putchar('-'); two(d); putchar(' ');
    two(h); putchar(':'); two(mi);
}

// USERS
struct User {
    bool in_use=false;
    string username, password, name, mail;
    int privilege=0;
    bool logged=false;
};

static const int MAX_USERS = 20000;
static User users[MAX_USERS];

// Very simple hash for usernames -> index, open addressing
static const int UHASH = 1<<15; // 32768
static int user_hash_to_idx[UHASH];

static unsigned int strhash(const string &s) {
    unsigned int h=2166136261u;
    for (char c: s) { h ^= (unsigned char)c; h *= 16777619u; }
    return h;
}

static void init_user_hash(){ for(int i=0;i<UHASH;i++) user_hash_to_idx[i]=-1; }

static int find_user(const string &uname) {
    unsigned int h=strhash(uname)&(UHASH-1);
    for (int step=0; step<UHASH; ++step) {
        int idx = user_hash_to_idx[(h+step)&(UHASH-1)];
        if (idx==-1) return -1;
        if (users[idx].in_use && users[idx].username==uname) return idx;
    }
    return -1;
}

static int alloc_user_slot(const string &uname) {
    // Check exist
    if (find_user(uname)!=-1) return -2; // duplicate
    // find free user slot
    int free_i=-1;
    for (int i=0;i<MAX_USERS;i++) if (!users[i].in_use){ free_i=i; break; }
    if (free_i==-1) return -3;
    // insert into hash table: find first empty bucket or tombstone
    unsigned int h=strhash(uname)&(UHASH-1);
    for (int step=0; step<UHASH; ++step) {
        int &bucket = user_hash_to_idx[(h+step)&(UHASH-1)];
        if (bucket==-1) { bucket=free_i; return free_i; }
    }
    return -3;
}

static void clear_users(){ for(int i=0;i<MAX_USERS;i++){ users[i]=User(); } init_user_hash(); }

// TRAINS
struct Train {
    bool in_use=false;
    bool released=false;
    string trainID;
    int stationNum=0;
    int seatNum=0;
    string type;
    string stations[105];
    int price_between[105]; // up to 100 segments
    int travel[105];
    int stopover[105]; // for stations 2..n-1 (we store size n-1, ignore 0 and last)
    HM start; // daily start time
    Date sale_l, sale_r;
};

static const int MAX_TRAINS = 8000;
static Train trains[MAX_TRAINS];
static const int THASH = 1<<15;
static int train_hash_to_idx[THASH];

static void init_train_hash(){ for(int i=0;i<THASH;i++) train_hash_to_idx[i]=-1; }

static int find_train(const string &tid){
    unsigned int h=strhash(tid)&(THASH-1);
    for(int step=0; step<THASH; ++step){
        int idx = train_hash_to_idx[(h+step)&(THASH-1)];
        if (idx==-1) return -1;
        if (trains[idx].in_use && trains[idx].trainID==tid) return idx;
    }
    return -1;
}

static int alloc_train_slot(const string &tid){
    if (find_train(tid)!=-1) return -2;
    int free_i=-1; for(int i=0;i<MAX_TRAINS;i++) if(!trains[i].in_use){ free_i=i; break; }
    if (free_i==-1) return -3;
    unsigned int h=strhash(tid)&(THASH-1);
    for(int step=0; step<THASH; ++step){
        int &bucket = train_hash_to_idx[(h+step)&(THASH-1)];
        if (bucket==-1){ bucket=free_i; return free_i; }
    }
    return -3;
}

static void clear_trains(){ for(int i=0;i<MAX_TRAINS;i++){ trains[i]=Train(); } init_train_hash(); }

// Parse arguments of form -k value
struct ArgPair { string key; string val; };
static int parse_kv(const string &line, string &cmd, ArgPair *out, int maxn){
    // tokenize by spaces, but values don't contain spaces per spec
    string toks[300]; int n=split(line, toks, 300);
    if (n<=0) return 0; cmd=toks[0];
    int m=0;
    for (int i=1;i<n;i++){
        if (!toks[i].empty() && toks[i][0]=='-' && (i+1)<n){
            out[m].key = toks[i];
            out[m].val = toks[i+1];
            ++m; ++i;
        }
    }
    return m;
}

static bool getv(ArgPair *a, int n, const char *k, string &out){
    string ks="-"; ks+=k;
    for(int i=0;i<n;i++) if (a[i].key==ks){ out=a[i].val; return true; }
    return false;
}

// Command handlers
static void handle_add_user(ArgPair *a, int n){
    string cu, u, p, name, mail, g;
    bool has_c=getv(a,n,"c",cu);
    bool has_u=getv(a,n,"u",u);
    bool has_p=getv(a,n,"p",p);
    bool has_n=getv(a,n,"n",name);
    bool has_m=getv(a,n,"m",mail);
    bool has_g=getv(a,n,"g",g);
    int uid_cnt=0; for(int i=0;i<MAX_USERS;i++) if (users[i].in_use) { uid_cnt++; if(uid_cnt>0) break; }
    bool first_user = (uid_cnt==0);
    if (!has_u||!has_p||!has_n||!has_m) { puts("-1"); return; }
    int slot = alloc_user_slot(u);
    if (slot<0) { puts("-1"); return; }
    int priv=0;
    if (first_user) {
        priv = 10;
    } else {
        if (!has_c||!has_g){ puts("-1");
            users[slot]=User();
            return; }
        int cidx = find_user(cu);
        if (cidx==-1 || !users[cidx].logged) { puts("-1"); users[slot]=User(); return; }
        priv = parse_int(g);
        if (!(priv < users[cidx].privilege)) { puts("-1"); users[slot]=User(); return; }
    }
    users[slot].in_use=true;
    users[slot].username=u; users[slot].password=p; users[slot].name=name; users[slot].mail=mail; users[slot].privilege=priv; users[slot].logged=false;
    puts("0");
}

static void handle_login(ArgPair *a, int n){
    string u,p; if(!getv(a,n,"u",u)||!getv(a,n,"p",p)){ puts("-1"); return; }
    int idx = find_user(u); if (idx==-1) { puts("-1"); return; }
    if (users[idx].password!=p) { puts("-1"); return; }
    if (users[idx].logged) { puts("-1"); return; }
    users[idx].logged=true; puts("0");
}

static void handle_logout(ArgPair *a, int n){
    string u; if(!getv(a,n,"u",u)){ puts("-1"); return; }
    int idx=find_user(u); if(idx==-1) { puts("-1"); return; }
    if (!users[idx].logged){ puts("-1"); return; }
    users[idx].logged=false; puts("0");
}

static void print_user_info(const User &u){
    printf("%s %s %s %d\n", u.username.c_str(), u.name.c_str(), u.mail.c_str(), u.privilege);
}

static void handle_query_profile(ArgPair*a,int n){
    string cu,u; if(!getv(a,n,"c",cu)||!getv(a,n,"u",u)){ puts("-1"); return; }
    int cidx=find_user(cu), uidx=find_user(u);
    if (cidx==-1||uidx==-1||!users[cidx].logged) { puts("-1"); return; }
    if (!( (users[cidx].privilege>users[uidx].privilege) || (cu==u) )) { puts("-1"); return; }
    print_user_info(users[uidx]);
}

static void handle_modify_profile(ArgPair*a,int n){
    string cu,u; if(!getv(a,n,"c",cu)||!getv(a,n,"u",u)){ puts("-1"); return; }
    int cidx=find_user(cu), uidx=find_user(u);
    if (cidx==-1||uidx==-1||!users[cidx].logged){ puts("-1"); return; }
    if (!((users[cidx].privilege>users[uidx].privilege) || (cu==u))){ puts("-1"); return; }
    string p,name,mail,g; bool hp=getv(a,n,"p",p); bool hn=getv(a,n,"n",name); bool hm=getv(a,n,"m",mail); bool hg=getv(a,n,"g",g);
    if (hg){ int ng=parse_int(g); if (!(ng < users[cidx].privilege)) { puts("-1"); return; } users[uidx].privilege=ng; }
    if (hp) users[uidx].password=p; if (hn) users[uidx].name=name; if (hm) users[uidx].mail=mail;
    print_user_info(users[uidx]);
}

static void handle_add_train(ArgPair*a,int n){
    string id, snum, seat, stations, prices, startx, travel, stopover, sdate, type;
    if (!getv(a,n,"i",id)||!getv(a,n,"n",snum)||!getv(a,n,"m",seat)||!getv(a,n,"s",stations)||!getv(a,n,"p",prices)||!getv(a,n,"x",startx)||!getv(a,n,"t",travel)||!getv(a,n,"o",stopover)||!getv(a,n,"d",sdate)||!getv(a,n,"y",type)) { puts("-1"); return; }
    int slot = alloc_train_slot(id); if (slot<0){ puts("-1"); return; }
    Train &tr = trains[slot]; tr.in_use=true; tr.trainID=id; tr.released=false; tr.type=type;
    tr.stationNum = parse_int(snum); tr.seatNum = parse_int(seat);
    HM sx = parse_hm(startx); tr.start = sx;
    string sd[2]; int dc=split_by(sdate, '|', sd, 2); if (dc!=2) { puts("-1"); tr=Train(); return; }
    tr.sale_l = parse_date(sd[0]); tr.sale_r = parse_date(sd[1]);

    string stv[105]; int sc=split_by(stations,'|',stv,105);
    if (sc!=tr.stationNum){ puts("-1"); tr=Train(); return; }
    for (int i=0;i<sc;i++) tr.stations[i]=stv[i];

    string pv[105]; int pc=split_by(prices,'|',pv,105);
    if (pc!=tr.stationNum-1){ puts("-1"); tr=Train(); return; }
    for (int i=0;i<pc;i++) tr.price_between[i]=parse_int(pv[i]);

    string tv[105]; int tc=split_by(travel,'|',tv,105);
    if (tc!=tr.stationNum-1){ puts("-1"); tr=Train(); return; }
    for (int i=0;i<tc;i++) tr.travel[i]=parse_int(tv[i]);

    if (tr.stationNum==2) {
        // stopover is underscore per spec
        // do nothing
    } else {
        string ov[105]; int oc=split_by(stopover,'|',ov,105);
        if (oc!=tr.stationNum-2){ puts("-1"); tr=Train(); return; }
        for (int i=0;i<oc;i++) tr.stopover[i]=parse_int(ov[i]);
    }
    puts("0");
}

static void handle_release_train(ArgPair*a,int n){
    string id; if(!getv(a,n,"i",id)){ puts("-1"); return; }
    int idx=find_train(id); if (idx==-1){ puts("-1"); return; }
    if (trains[idx].released){ puts("-1"); return; }
    trains[idx].released=true; puts("0");
}

static void handle_query_train(ArgPair*a,int n){
    string id, d; if(!getv(a,n,"i",id)||!getv(a,n,"d",d)){ puts("-1"); return; }
    int idx=find_train(id); if (idx==-1){ puts("-1"); return; }
    Train &tr=trains[idx];
    Date dep = parse_date(d);
    // Output header
    printf("%s %s\n", tr.trainID.c_str(), tr.type.c_str());
    // cumulative times
    int nst = tr.stationNum;
    int cum_price=0;
    long long cur_dep_min_offset=0; // from base dep day dep time at station1
    // station 1
    {
        // arrival xx-xx xx:xx; leaving base
        printf("%s xx-xx xx:xx -> ", tr.stations[0].c_str());
        int mm,dd,hh,mi; add_minutes(dep, tr.start.h, tr.start.m, 0, mm,dd,hh,mi);
        print_time_md_hm(mm,dd,hh,mi);
        printf(" %d %d\n", 0, tr.seatNum);
    }
    // middle stations 2..n-1
    long long running = 0; // minutes after departure from station1
    for (int i=0;i<nst-1;i++){
        running += tr.travel[i];
        // arrive station i+1
        int arr_m,arr_d,arr_h,arr_min; add_minutes(dep, tr.start.h, tr.start.m, running, arr_m,arr_d,arr_h,arr_min);
        if (i+1==nst-1){
            // terminal station
            printf("%s ", tr.stations[i+1].c_str());
            print_time_md_hm(arr_m,arr_d,arr_h,arr_min);
            printf(" -> xx-xx xx:xx ");
            cum_price += tr.price_between[i];
            printf("%d x\n", cum_price);
            break;
        } else {
            // leave station i+1 after stopover
            int dep_m,dep_d,dep_h,dep_min;
            int stp = tr.stopover[i];
            add_minutes(dep, tr.start.h, tr.start.m, running + stp, dep_m,dep_d,dep_h,dep_min);
            printf("%s ", tr.stations[i+1].c_str());
            print_time_md_hm(arr_m,arr_d,arr_h,arr_min);
            printf(" -> ");
            print_time_md_hm(dep_m,dep_d,dep_h,dep_min);
            cum_price += tr.price_between[i];
            printf(" %d %d\n", cum_price, tr.seatNum);
            running += stp;
        }
    }
}

static void handle_delete_train(ArgPair*a,int n){
    string id; if(!getv(a,n,"i",id)){ puts("-1"); return; }
    int idx=find_train(id); if (idx==-1) { puts("-1"); return; }
    if (trains[idx].released){ puts("-1"); return; }
    trains[idx]=Train(); // mark free (hash left as-is)
    puts("0");
}

int main(){
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    init_user_hash(); init_train_hash();
    string line;
    while (std::getline(std::cin, line)){
        trim(line); if (line.empty()) continue;
        // Some datasets may prefix index like [1] command; strip leading [..]
        if (!line.empty() && line[0]=='['){
            size_t r = line.find(']'); if (r!=string::npos && r+1<line.size()) line = line.substr(r+1);
            trim(line);
        }
        string cmd; ArgPair kv[256]; int kvc = parse_kv(line, cmd, kv, 256);
        if (cmd=="add_user") handle_add_user(kv,kvc);
        else if (cmd=="login") handle_login(kv,kvc);
        else if (cmd=="logout") handle_logout(kv,kvc);
        else if (cmd=="query_profile") handle_query_profile(kv,kvc);
        else if (cmd=="modify_profile") handle_modify_profile(kv,kvc);
        else if (cmd=="add_train") handle_add_train(kv,kvc);
        else if (cmd=="release_train") handle_release_train(kv,kvc);
        else if (cmd=="query_train") handle_query_train(kv,kvc);
        else if (cmd=="delete_train") handle_delete_train(kv,kvc);
        else if (cmd=="clean") { clear_users(); clear_trains(); puts("0"); }
        else if (cmd=="exit") { puts("bye"); break; }
        else {
            // unsupported commands return -1
            puts("-1");
        }
    }
    return 0;
}
