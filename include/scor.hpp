#pragma once
#include <bits/stdc++.h>
using namespace std;

struct Metrics {
    int startHP = 300;
    int endHP = 300;
    int endAP = 0;
    int shotsFired = 0;
    int hitsLanded = 0;
    int damageDealt = 0;
    int kills = 0;
};

struct Weights {
    // default weights; override via CLI --weights hp=...,ap=...,dmg=...,hits=...,shots=...,kill=...
    double w_hp = 0.0;      // weight for remaining HP
    double w_ap = 0.0;      // weight for remaining AP
    double w_dmg = 1.0;     // weight for total damage dealt
    double w_hits = 0.0;    // weight for number of hits landed
    double w_shots = 0.0;   // weight for number of shots fired
    double w_kill = 500.0;  // bonus per kill
};

inline Weights parseWeights(const string& s){
    Weights w;
    if(s.empty()) return w;
    auto setkv = [&](string k, string v){
        double x = atof(v.c_str());
        if(k=="hp") w.w_hp = x;
        else if(k=="ap") w.w_ap = x;
        else if(k=="dmg" || k=="damage") w.w_dmg = x;
        else if(k=="hits") w.w_hits = x;
        else if(k=="shots") w.w_shots = x;
        else if(k=="kill" || k=="kills") w.w_kill = x;
    };
    string tmp = s;
    for(char& c: tmp) if(c==';') c = ',';
    stringstream ss(tmp);
    string tok;
    while(getline(ss, tok, ',')){
        auto p = tok.find('=');
        if(p==string::npos) continue;
        string k = tok.substr(0,p), v = tok.substr(p+1);
        // trim
        auto trim = [&](string& t){
            while(!t.empty() && isspace((unsigned char)t.back())) t.pop_back();
            size_t i=0; while(i<t.size() && isspace((unsigned char)t[i])) ++i;
            t = t.substr(i);
        };
        trim(k); trim(v);
        if(!k.empty() && !v.empty()) setkv(k,v);
    }
    return w;
}

inline double scoreOf(const Metrics& m, const Weights& w){
    return w.w_hp*m.endHP + w.w_ap*m.endAP + w.w_dmg*m.damageDealt +
           w.w_hits*m.hitsLanded + w.w_shots*m.shotsFired + w.w_kill*m.kills;
}
