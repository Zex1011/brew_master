#ifndef CFG_ARRAY_H
#define CFG_ARRAY_H

// máximo de curvas ativas
constexpr int MAX_CFG = 20;            

// arrays fixos
extern int temps[MAX_CFG];
extern int delays[MAX_CFG];

// quantidade de curvas ativas
extern int numCfg;                     

// callbacks acionados pelo statechart
int  getTemp  (int idx);
int  getDelay (int idx);
void setTemp  (int idx, int val);
void setDelay (int idx, int val);

int  getNumCfg();
int  pushCfg  (int t, int d);
int  popCfg   (int &t, int &d);
void clearCfgs();

#endif
