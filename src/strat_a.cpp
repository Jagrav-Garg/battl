#include "defs.hpp"

// Baseline: harass on LoS, kite when too close, disengage when low HP.
Action choose_action_A(const Tank& self, const vector<Enemy>& enemies,
                       const unordered_map<string,int>& cooldowns,
                       IsBlockedFn isBlocked)
{
    Action a; a.type = ActionType::IDLE;
    if(enemies.empty()){
        static std::mt19937 rng(1337);
        int r = std::uniform_int_distribution<int>(0,3)(rng);
        a.type=ActionType::MOVE;
        a.dir=(r==0)?Dir::UP:(r==1)?Dir::DOWN:(r==2)?Dir::LEFT:Dir::RIGHT;
        return a;
    }
    // nearest target
    int best=-1, bestD=INT_MAX;
    for(int i=0;i<(int)enemies.size();++i){
        int d=manhattan(self.x,self.y,enemies[i].x,enemies[i].y);
        if(d<bestD){ bestD=d; best=i; }
    }
    const Enemy& tgt = enemies[best];

    // thresholds
    const int kite_min_md = 4;
    const int low_hp = 100;

    if(self.hp <= low_hp){
        int awayx = sgn(self.x - tgt.x), awayy = sgn(self.y - tgt.y);
        if(abs(self.x - tgt.x) >= abs(self.y - tgt.y)){
            int nx = self.x + 2*awayx, ny = self.y;
            if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=2*awayx; a.jdy=0; return a; }
        }
        int nx = self.x, ny = self.y + 2*awayy;
        if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=0; a.jdy=2*awayy; return a; }
        Dir d = greedy_step_away(self.x,self.y,tgt.x,tgt.y,isBlocked);
        if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
        return a;
    }

    int dx=tgt.x-self.x, dy=tgt.y-self.y;
    bool can8 = in_eight_dir(dx,dy);
    bool los = can8 && clear_line_8way(self.x,self.y,tgt.x,tgt.y,isBlocked);
    if(los){
        int md = manhattan(self.x,self.y,tgt.x,tgt.y);
        const char* band = pick_band_by_dist(md);
        if(cd_ready(cooldowns, band)){
            auto aim = aim_dir8(self.x,self.y,tgt.x,tgt.y);
            a.type=ActionType::SHOOT; a.bulletType=band; a.shoot_dx=aim.first; a.shoot_dy=aim.second; return a;
        }
        if(md < kite_min_md){
            Dir d = greedy_step_away(self.x,self.y,tgt.x,tgt.y,isBlocked);
            if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
        }
        Dir d = greedy_step_toward(self.x,self.y,tgt.x,tgt.y,isBlocked);
        if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
        return a;
    }else{
        int md = manhattan(self.x,self.y,tgt.x,tgt.y);
        if(md < kite_min_md){
            Dir d = greedy_step_away(self.x,self.y,tgt.x,tgt.y,isBlocked);
            if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
            int awayx = sgn(self.x - tgt.x), awayy = sgn(self.y - tgt.y);
            int nx = self.x + 2*awayx, ny = self.y;
            if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=2*awayx; a.jdy=0; return a; }
            nx = self.x; ny = self.y + 2*awayy;
            if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=0; a.jdy=2*awayy; return a; }
            return a;
        }
        Dir d = greedy_step_toward(self.x,self.y,tgt.x,tgt.y,isBlocked);
        if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
        return a;
    }
}
