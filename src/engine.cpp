#include "defs.hpp"
#include "scor.hpp"

// Strategy entry points
Action choose_action_A(const Tank&, const vector<Enemy>&, const unordered_map<string,int>&, IsBlockedFn);
Action choose_action_B(const Tank&, const vector<Enemy>&, const unordered_map<string,int>&, IsBlockedFn);

// Simple projectile model
struct Bullet { int x,y, dx,dy, owner, range, dmg; };
struct BulletParams { int range; int dmg; };
static inline BulletParams paramsFor(int typeCode){
    switch(typeCode){ case 0: return {4,25}; case 1: return {7,20}; case 2: return {11,15}; default: return {50,10}; }
}

static StrategyFn pickStrategy(const string& name){
    if(name=="A"||name=="a") return &choose_action_A;
    if(name=="B"||name=="b") return &choose_action_B;
    return &choose_action_A;
}

static string actionToStr(const Action& a){
    ostringstream ss;
    switch(a.type){
        case ActionType::MOVE: ss<<"MOVE("<<dirToStr(a.dir)<<")"; break;
        case ActionType::JUMP: ss<<"JUMP("<<a.jdx<<","<<a.jdy<<")"; break;
        case ActionType::SHOOT: ss<<"SHOOT["<<a.bulletType<<"]("<<a.shoot_dx<<","<<a.shoot_dy<<")"; break;
        case ActionType::FOG: ss<<"FOG("<<a.fog_x<<","<<a.fog_y<<","<<a.fog_k<<")"; break;
        case ActionType::STEALTH: ss<<"STEALTH("<<a.stealth_target_id<<")"; break;
        case ActionType::IDLE: ss<<"IDLE"; break;
        default: ss<<"PASS"; break;
    }
    return ss.str();
}

struct CLI {
    string left="A", right="B";
    int ticks=200;
    Weights weights; // scoring weights
};
static CLI parseCLI(int argc, char** argv){
    CLI c;
    if(argc>=2) c.left = argv[1];
    if(argc>=3) c.right = argv[2];
    for(int i=3;i<argc;i++){
        string a = argv[i];
        auto eq = a.find('=');
        if(a.rfind("--ticks",0)==0){
            if(eq!=string::npos) c.ticks = max(1, atoi(a.substr(eq+1).c_str()));
        }else if(a.rfind("--weights",0)==0){
            if(eq!=string::npos) c.weights = parseWeights(a.substr(eq+1));
        }
    }
    return c;
}

int main(int argc, char** argv){
    CLI cli = parseCLI(argc, argv);

    vector<string> grid = {
        "...............",
        "...###.........",
        "........#......",
        "....###........",
        "...............",
        "...#...........",
        "...#.....###...",
        "...#...........",
        "...............",
        "...###.....#...",
        "...............",
        "...............",
        "...............",
        "...............",
        "...............",
    };
    auto inb = [&](int x,int y){ return y>=0 && y<(int)grid.size() && x>=0 && x<(int)grid[0].size(); };
    auto isWall = [&](int x,int y){ return !inb(x,y) || grid[y][x]=='#'; };

    StrategyFn Ls = pickStrategy(cli.left);
    StrategyFn Rs = pickStrategy(cli.right);

    Tank L(1,"Left", 2,2);
    Tank R(2,"Right",12,10);

    Metrics ML; ML.startHP = L.hp;
    Metrics MR; MR.startHP = R.hp;

    cout<<"Start: "<<L.debugState()<<" | "<<R.debugState()<<"\n";

    vector<Bullet> bullets;
    auto onPickup = [&](int,int){};

    for(int tick=0; tick<cli.ticks; ++tick){
        vector<Enemy> eL = {{R.id,R.x,R.y,R.hp}};
        vector<Enemy> eR = {{L.id,L.x,L.y,L.hp}};

        Action aL = Ls(L, eL, L.cooldowns, isWall);
        Action aR = Rs(R, eR, R.cooldowns, isWall);

        auto prL = L.validateAndMakeIntent(aL);
        if(!prL.first){ aL = Action{}; aL.type=ActionType::IDLE; prL=L.validateAndMakeIntent(aL); }
        auto prR = R.validateAndMakeIntent(aR);
        if(!prR.first){ aR = Action{}; aR.type=ActionType::IDLE; prR=R.validateAndMakeIntent(aR); }

        auto emitEvent = [&](const string& ev, const vector<int>& pl){
            if(ev=="spawn_bullet" && pl.size()>=6){
                BulletParams P = paramsFor(pl[4]);
                int sx = pl[0]+pl[2], sy = pl[1]+pl[3];
                if(!isWall(sx,sy)){
                    bullets.push_back(Bullet{sx,sy,pl[2],pl[3],pl[5],P.range,P.dmg});
                    if(pl[5]==L.id) ++ML.shotsFired; else if(pl[5]==R.id) ++MR.shotsFired;
                }
                cout<<"  [tick "<<tick<<"] "<<ev<<" owner="<<pl[5]<<" dmg="<<P.dmg<<" rng="<<P.range<<"\n";
            }else{
                cout<<"  [tick "<<tick<<"] "<<ev<<" payload="<<pl.size()<<"\n";
            }
        };

        L.applyActionEffect(prL.second, tick, isWall, onPickup, emitEvent);
        R.applyActionEffect(prR.second, tick, isWall, onPickup, emitEvent);

        L.finalizeTickConsume(aL);
        R.finalizeTickConsume(aR);

        // bullets advance and resolve
        vector<Bullet> nextB;
        for(auto &b: bullets){
            int nx = b.x + b.dx, ny = b.y + b.dy;
            if(isWall(nx,ny)) continue;
            bool hit = false;
            if((b.owner!=L.id) && nx==L.x && ny==L.y){
                L.takeDamage(b.dmg); MR.hitsLanded++; MR.damageDealt += b.dmg; hit=true;
                if(!L.alive) MR.kills++;
            }
            if((b.owner!=R.id) && nx==R.x && ny==R.y){
                R.takeDamage(b.dmg); ML.hitsLanded++; ML.damageDealt += b.dmg; hit=true;
                if(!R.alive) ML.kills++;
            }
            if(hit) continue;
            Bullet nb=b; nb.x=nx; nb.y=ny; nb.range--;
            if(nb.range>0) nextB.push_back(nb);
        }
        bullets.swap(nextB);

        cout<<"Tick "<<tick<<": L "<<actionToStr(aL)<<" -> ("<<L.x<<","<<L.y<<")"
            <<"  |  R "<<actionToStr(aR)<<" -> ("<<R.x<<","<<R.y<<")"
            <<"  |  HP L="<<L.hp<<" R="<<R.hp<<"\n";

        if(!L.alive || !R.alive){
            cout<<"Winner: "<<(L.alive? "Left" : "Right")<<"\n";
            break;
        }
    }

    // final metrics and score
    ML.endHP = L.hp; ML.endAP = L.ap;
    MR.endHP = R.hp; MR.endAP = R.ap;

    double SL = scoreOf(ML, cli.weights);
    double SR = scoreOf(MR, cli.weights);

    cout<<"\n=== Match Summary ===\n";
    auto printM = [&](const char* name, const Metrics& m, double S){
        cout<<name<<": HP "<<m.startHP<<" -> "<<m.endHP
            <<", AP "<<m.endAP
            <<", Shots "<<m.shotsFired
            <<", Hits "<<m.hitsLanded
            <<", Damage "<<m.damageDealt
            <<", Kills "<<m.kills
            <<", Score "<<S<<"\n";
    };
    printM("Left ", ML, SL);
    printM("Right", MR, SR);

    cout<<"Result: ";
    if(SL>SR) cout<<"Left wins by score\n";
    else if(SR>SL) cout<<"Right wins by score\n";
    else cout<<"Draw by score\n";

    cout<<"End: "<<L.debugState()<<" | "<<R.debugState()<<"\n";
    return 0;
}
