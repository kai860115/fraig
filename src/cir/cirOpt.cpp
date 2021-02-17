/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
   CirGate::setGlobalRef();
   for (auto& id : _POIds)
      dfsTraversal(_totGates[id]);

   for (size_t i = 0; i < _totGates.size(); i++) {
      if (_totGates[i] && !_totGates[i]->isGlobalRef() && _totGates[i]->getType() == AIG_GATE) {
         cout << "Sweeping: AIG(" << _totGates[i]->getGid() << ") removed...\n";
         _headerInfo[4]--;

         for (size_t j = 0; j < 2; j++) {
            AigGateV faninV = _totGates[i]->getFanin(j);
            if (_totGates[faninV.getGid()]) {
               _totGates[faninV.getGid()]->removeFanout(AigGateV(_totGates[i], faninV.isInv(), i));
            }
         }
         delete _totGates[i];
         _totGates[i] = 0;
      }
      else if (_totGates[i] && !_totGates[i]->isGlobalRef() && _totGates[i]->getType() == UNDEF_GATE) {
         cout << "Sweeping: UNDEF(" << _totGates[i]->getGid() << ") removed...\n";
         delete _totGates[i];
         _totGates[i] = 0;
      }
   }

}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
   CirGate::setGlobalRef();
   GateList dfsList;
   dfsList.reserve(_headerInfo[0] + _headerInfo[3] + 1);
   for (const auto& id : _POIds)
      dfsTraversal(_totGates[id], dfsList);

   for (auto& g : dfsList) {
      if (!g->isAig())
         continue;
      if (g->hasConstFanin(0)) {
         _totGates[0]->merge(g, 0);
         _headerInfo[4]--; 
         cout << "Simplifying: " << 0 << " merging " << g->getGid() << "...\n";
         delete _totGates[g->getGid()];
         _totGates[g->getGid()] = 0;
      }
      else if (g->hasIdenticalFanin()) {
         CirGate* mergeGate;
         bool isInv = false;
         mergeGate = g->getFanin(0).gate();
         isInv = g->getFanin(0).isInv();
         mergeGate->merge(g, isInv);
         _headerInfo[4]--;
         cout << "Simplifying: " << mergeGate->getGid() << " merging " << (isInv ? "!" : "") << g->getGid() << "...\n";
         delete _totGates[g->getGid()];
         _totGates[g->getGid()] = 0;
      }
      else if (g->hasInvertedFanin()) {
         _totGates[0]->merge(g, 0);
         _headerInfo[4]--;
         cout << "Simplifying: " << 0 << " merging " << g->getGid() << "...\n";
         delete _totGates[g->getGid()];
         _totGates[g->getGid()] = 0;
      }
      else if (g->hasConstFanin(1)) {
         CirGate* mergeGate;
         bool isInv = false;
         for (size_t i = 0; i < 2; i++) {
            if (g->getFanin(i) != AigGateV(_totGates[0], 1, 0)) {
               mergeGate = g->getFanin(i).gate();
               isInv = g->getFanin(i).isInv();
            }
         }
         mergeGate->merge(g, isInv);
         _headerInfo[4]--;
         cout << "Simplifying: " << mergeGate->getGid() << " merging " << (isInv ? "!" : "") << g->getGid() << "...\n";
         delete _totGates[g->getGid()];
         _totGates[g->getGid()] = 0;
      }
   }

   for (size_t i = 0; i < _headerInfo[0] + 1; i++) {
      if (_totGates[i] && _totGates[i]->getType() == UNDEF_GATE && _totGates[i]->getFanoutSize() == 0) {
         delete _totGates[i];
         _totGates[i] = 0;
      }
   }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
