#include "defs.hpp"

// Aggressive rusher: close distance, prefer medium/short bands, jump to close gaps.
Action choose_action_B(const Tank& self, const vector<Enemy>& enemies,
                       const unordered_map<string,int>& cooldowns,
                       IsBlockedFn isBlocked)
{
    Action a; a.type = ActionType::IDLE;
    if(enemies.empty()){
        a.type=ActionType::MOVE; a.dir=Dir::RIGHT; return a;
    }
    // target nearest
    int best=-1, bestD=INT_MAX;
    for(int i=0;i<(int)enemies.size();++i){
        int d=manhattan(self.x,self.y,enemies[i].x,enemies[i].y);
        if(d<bestD){ bestD=d; best=i; }
    }
    const Enemy& tgt = enemies[best];

    int dx=tgt.x-self.x, dy=tgt.y-self.y;
    bool can8 = in_eight_dir(dx,dy);
    bool los = can8 && clear_line_8way(self.x,self.y,tgt.x,tgt.y,isBlocked);
    int md = manhattan(self.x,self.y,tgt.x,tgt.y);

    // prefer shoot if LoS, bias to medium/short
    if(los){
        const char* band = (md<=3? "short" : (md<=6? "medium" : "long"));
        if(cd_ready(cooldowns, band)){
            auto aim = aim_dir8(self.x,self.y,tgt.x,tgt.y);
            a.type=ActionType::SHOOT; a.bulletType=band; a.shoot_dx=aim.first; a.shoot_dy=aim.second; return a;
        }
    }

    // if blocked and md>=4, try jump closer along dominant axis
    if(md>=4){
        if(abs(dx)>=abs(dy)){
            int stepx = sgn(dx);
            int nx = self.x + 2*stepx, ny = self.y;
            if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=2*stepx; a.jdy=0; return a; }
        }else{
            int stepy = sgn(dy);
            int nx = self.x, ny = self.y + 2*stepy;
            if(!isBlocked(nx,ny)){ a.type=ActionType::JUMP; a.jdx=0; a.jdy=2*stepy; return a; }
        }
    }

    // otherwise greedy close
    Dir d = greedy_step_toward(self.x,self.y,tgt.x,tgt.y,isBlocked);
    if(d!=Dir::NONE){ a.type=ActionType::MOVE; a.dir=d; return a; }
    return a;
}
