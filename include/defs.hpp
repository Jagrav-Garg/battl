#pragma once
#include <bits/stdc++.h>
using namespace std;

// ---------- Small helpers ----------
enum class Dir { UP, DOWN, LEFT, RIGHT, NONE };
static inline const vector<pair<int,int>>& dirDelta() {
    static const vector<pair<int,int>> d = {{0,-1},{0,1},{-1,0},{1,0},{0,0}};
    return d;
}
static inline string dirToStr(Dir d){
    switch(d){
        case Dir::UP: return "UP";
        case Dir::DOWN: return "DOWN";
        case Dir::LEFT: return "LEFT";
        case Dir::RIGHT: return "RIGHT";
        default: return "NONE";
    }
}

// 8-direction check and helpers
static inline int sgn(int v){ return (v>0) - (v<0); }
static inline int manhattan(int x1,int y1,int x2,int y2){ return abs(x1-x2)+abs(y1-y2); }
static inline bool in_eight_dir(int dx,int dy){
    return (dx==0 || dy==0 || abs(dx)==abs(dy)) && !(dx==0 && dy==0);
}
template<typename IsBlocked>
static inline bool clear_line_8way(int x0,int y0,int x1,int y1, IsBlocked isBlocked){
    int dx = x1 - x0, dy = y1 - y0;
    if(!in_eight_dir(dx,dy)) return false;
    int stepx = sgn(dx), stepy = sgn(dy);
    int cx = x0 + stepx, cy = y0 + stepy;
    while(cx != x1 || cy != y1){
        if(isBlocked(cx, cy)) return false;
        if(cx != x1) cx += stepx;
        if(cy != y1) cy += stepy;
    }
    return true;
}
template<typename IsBlocked>
static inline Dir greedy_step_toward(int sx,int sy,int tx,int ty, IsBlocked isBlocked){
    int dx = tx - sx, dy = ty - sy;
    if(abs(dx) >= abs(dy)){
        int stepx = sgn(dx);
        if(stepx!=0 && !isBlocked(sx+stepx, sy)) return (stepx>0)?Dir::RIGHT:Dir::LEFT;
        int stepy = sgn(dy);
        if(stepy!=0 && !isBlocked(sx, sy+stepy)) return (stepy>0)?Dir::DOWN:Dir::UP;
    }else{
        int stepy = sgn(dy);
        if(stepy!=0 && !isBlocked(sx, sy+stepy)) return (stepy>0)?Dir::DOWN:Dir::UP;
        int stepx = sgn(dx);
        if(stepx!=0 && !isBlocked(sx+stepx, sy)) return (stepx>0)?Dir::RIGHT:Dir::LEFT;
    }
    return Dir::NONE;
}
template<typename IsBlocked>
static inline Dir greedy_step_away(int sx,int sy,int tx,int ty, IsBlocked isBlocked){
    int dx = sx - tx, dy = sy - ty;
    if(abs(dx) >= abs(dy)){
        int stepx = sgn(dx);
        if(stepx!=0 && !isBlocked(sx+stepx, sy)) return (stepx>0)?Dir::RIGHT:Dir::LEFT;
        int stepy = sgn(dy);
        if(stepy!=0 && !isBlocked(sx, sy+stepy)) return (stepy>0)?Dir::DOWN:Dir::UP;
    }else{
        int stepy = sgn(dy);
        if(stepy!=0 && !isBlocked(sx, sy+stepy)) return (stepy>0)?Dir::DOWN:Dir::UP;
        int stepx = sgn(dx);
        if(stepx!=0 && !isBlocked(sx+stepx, sy)) return (stepx>0)?Dir::RIGHT:Dir::LEFT;
    }
    return Dir::NONE;
}

// ---------- Action definitions ----------
enum class ActionType { MOVE, JUMP, SHOOT, FOG, STEALTH, IDLE, PASS };

struct Action {
    ActionType type = ActionType::IDLE;
    // MOVE
    Dir dir = Dir::NONE;
    // JUMP
    int jdx = 0, jdy = 0; // relative
    // SHOOT
    string bulletType;              // "short","medium","long","infinite"
    int shoot_dx = 0, shoot_dy = 0; // 8-way
    // FOG
    int fog_x = 0, fog_y = 0, fog_k = 0;
    // STEALTH
    int stealth_target_id = -1;
};

// ---------- Tank class ----------
struct Tank {
    int id;
    string name;
    int x, y;
    Dir facing = Dir::RIGHT;
    bool alive = true;
    int hp = 300;
    int ap = 100;
    int ap_bank_cap = 200;
    bool blind = false;

    unordered_map<string,int> cooldowns;
    unordered_map<string,int> baseCooldowns = {
        {"short",2},{"medium",3},{"long",4},{"infinite",10},
        {"stealth",10},{"fog",0},{"spin_top",15},{"nuke_farthest",5}
    };

    Action previousAction;
    unordered_set<string> powerups;

    int lastTickUpdated = 0;

    Tank(int id_, string n, int sx, int sy)
    : id(id_), name(std::move(n)), x(sx), y(sy) {
        for(auto& p: baseCooldowns) cooldowns[p.first] = 0;
    }

    struct Intent {
        bool move=false; Dir moveDir=Dir::NONE;
        bool jump=false; int jdx=0, jdy=0;
        bool shoot=false; string shootType; int sdx=0, sdy=0;
        bool fog=false; int fog_x=0,fog_y=0,fog_k=0;
        bool stealth=false; int stealth_target_id=-1;
        bool idle=false;
    };

    int apCostForAction(const Action &a) const {
        switch(a.type){
            case ActionType::MOVE: return 0;
            case ActionType::JUMP: return 2;
            case ActionType::SHOOT:
                if(a.bulletType=="short") return 1;
                if(a.bulletType=="medium") return 2;
                if(a.bulletType=="long") return 3;
                if(a.bulletType=="infinite") return 5;
                return 2;
            case ActionType::FOG: return (10 + a.fog_k) * 3;
            case ActionType::STEALTH: return 5;
            case ActionType::IDLE: return 0;
            default: return 0;
        }
    }
    bool isCooldownReady(const string& name) const {
        auto it = cooldowns.find(name);
        return it==cooldowns.end() || it->second<=0;
    }

    pair<bool, Intent> validateAndMakeIntent(const Action &a){
        Intent it;
        if(!alive){ it.idle=true; return {false,it}; }
        if(apCostForAction(a) > ap) return {false,it};

        if(a.type == ActionType::SHOOT){
            string t = a.bulletType.empty()? "short" : a.bulletType;
            if(!isCooldownReady(t)) return {false,it};
            it.shoot=true; it.shootType=t; it.sdx=a.shoot_dx; it.sdy=a.shoot_dy;
            return {true,it};
        }else if(a.type == ActionType::MOVE){
            if(a.dir==Dir::NONE) return {false,it};
            it.move=true; it.moveDir=a.dir; return {true,it};
        }else if(a.type == ActionType::JUMP){
            if(abs(a.jdx)+abs(a.jdy)==2 || (abs(a.jdx)==2 && a.jdy==0) || (abs(a.jdy)==2 && a.jdx==0)){
                it.jump=true; it.jdx=a.jdx; it.jdy=a.jdy; return {true,it};
            }else return {false,it};
        }else if(a.type == ActionType::FOG){
            if(a.fog_k<1 || a.fog_k>10) return {false,it};
            it.fog=true; it.fog_x=a.fog_x; it.fog_y=a.fog_y; it.fog_k=a.fog_k; return {true,it};
        }else if(a.type == ActionType::STEALTH){
            if(!isCooldownReady("stealth")) return {false,it};
            it.stealth=true; it.stealth_target_id=a.stealth_target_id; return {true,it};
        }else if(a.type == ActionType::IDLE){
            it.idle=true; return {true,it};
        }
        return {false,it};
    }

    // encode bullet type for event payload
    static int bulletTypeCode(const string& t){
        if(t=="short") return 0;
        if(t=="medium") return 1;
        if(t=="long") return 2;
        return 3; // infinite or other
    }

    void applyActionEffect(const Intent &intent, int currentTick,
                           function<bool(int,int)> isCellBlocked,
                           function<void(int,int)> onPickup,
                           function<void(const string&, const vector<int>&)> emitEvent)
    {
        lastTickUpdated = currentTick;
        if(intent.move){
            auto d = dirDelta()[static_cast<int>(intent.moveDir)];
            int nx = x + d.first, ny = y + d.second;
            if(!isCellBlocked(nx,ny)){ x=nx; y=ny; onPickup(x,y); }
            facing = intent.moveDir;
        }else if(intent.jump){
            int nx=x+intent.jdx, ny=y+intent.jdy;
            if(!isCellBlocked(nx,ny)){ x=nx; y=ny; onPickup(x,y); }
        }
        if(intent.shoot){
            // payload: x,y,dx,dy,typeCode,ownerId
            vector<int> payload = {x,y,intent.sdx,intent.sdy,bulletTypeCode(intent.shootType),id};
            emitEvent("spawn_bullet", payload);
        }
        if(intent.fog){
            vector<int> payload = {intent.fog_x,intent.fog_y,intent.fog_k,id};
            emitEvent("spawn_fog", payload);
        }
        if(intent.stealth){
            vector<int> payload = {intent.stealth_target_id,id};
            emitEvent("apply_stealth", payload);
        }
    }

    void finalizeTickConsume(const Action &acceptedAction){
        int cost = apCostForAction(acceptedAction);
        ap = max(0, ap - cost);
        if(ap > ap_bank_cap) ap = ap_bank_cap;

        if(acceptedAction.type == ActionType::SHOOT){
            string t = acceptedAction.bulletType.empty()? "short" : acceptedAction.bulletType;
            cooldowns[t] = (baseCooldowns.count(t)? baseCooldowns[t] : 1);
        }else if(acceptedAction.type == ActionType::STEALTH){
            cooldowns["stealth"] = baseCooldowns["stealth"];
        }
        for(auto& p: cooldowns) if(p.second>0) p.second = max(0, p.second-1);
        previousAction = acceptedAction;
    }

    void takeDamage(int d){
        if(!alive) return;
        hp -= d;
        if(hp<=0){ hp=0; alive=false; }
    }

    string debugState() const {
        ostringstream ss;
        ss << "[Tank " << id << " '" << name << "' pos=(" << x << "," << y << ") hp=" << hp
           << " ap=" << ap << " facing=" << dirToStr(facing) << " alive=" << (alive?1:0) << "]";
        return ss.str();
    }
};

// Public minimal enemy view for strategies
struct Enemy { int id, x, y, hp; };

// Strategy function signature (implemented in strategy files)
using IsBlockedFn = function<bool(int,int)>;
using StrategyFn = Action(*)(const Tank& self, const vector<Enemy>& enemies,
                             const unordered_map<string,int>& cooldowns,
                             IsBlockedFn isBlocked);

// Convenience aim and band
static inline pair<int,int> aim_dir8(int sx,int sy,int tx,int ty){
    return { sgn(tx - sx), sgn(ty - sy) };
}
static inline const char* pick_band_by_dist(int d){
    if(d<=3) return "short";
    if(d<=6) return "medium";
    if(d<=10) return "long";
    return "infinite";
}
static inline bool cd_ready(const unordered_map<string,int>& cds, const string& k){
    auto it = cds.find(k);
    return it==cds.end() || it->second<=0;
}
