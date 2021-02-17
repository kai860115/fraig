/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
   CirGate::setGlobalRef();
   GateList dfsList;
   dfsList.reserve(_headerInfo[0] + _headerInfo[3] + 1);
   for (const auto& id : _POIds)
      dfsTraversal(_totGates[id], dfsList);
   
   HashMap<HashKey, CirGate*> hash(_headerInfo[0] + 1);
   for (auto& g : dfsList) {
      if (!g->isAig())
         continue;
      
      HashKey k(g->getFanin(0).getGateV(), g->getFanin(1).getGateV());
      CirGate* mergeGate;
      if (hash.query(k, mergeGate)) {
         mergeGate->merge(g, 0);
         _headerInfo[4]--;
         delete _totGates[g->getGid()];
         _totGates[g->getGid()] = 0;
         cout << "Strashing: " << mergeGate->getGid() << " merging " << g->getGid() << "...\n";
      }
      else 
         hash.insert(k, g);
   }

}

void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
