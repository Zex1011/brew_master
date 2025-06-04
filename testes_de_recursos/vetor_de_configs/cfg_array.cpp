#include "cfg_array.h"

// Inicializando os Arrays
int temps [MAX_CFG] = {0};
int delays[MAX_CFG] = {0};
int numCfg          = 0;

// Funções de operação dos Arrays
int getTemp(int idx)
{
    return (idx >= 0 && idx < numCfg) ? temps[idx] : -1;
}
int getDelay(int idx)
{
    return (idx >= 0 && idx < numCfg) ? delays[idx] : -1;
}
void setTemp(int idx, int val)
{
    if (idx >= 0 && idx < numCfg) temps[idx] = val;
}
void setDelay(int idx, int val)
{
    if (idx >= 0 && idx < numCfg) delays[idx] = val;
}
int getNumCfg()
{
    return numCfg;
}
int pushCfg(int t, int d)
{
    if (numCfg >= MAX_CFG) return -1;          // overflow
    temps [numCfg] = t;
    delays[numCfg] = d;
    ++numCfg;
    return 0;
}
int popCfg(int &t, int &d)
{
    if (numCfg <= 0) return -1;                // underflow
    --numCfg;
    t = temps [numCfg];
    d = delays[numCfg];
    return 0;
}
void clearCfgs()
{
    numCfg = 0;                                // não precisa zerar arrays
}
