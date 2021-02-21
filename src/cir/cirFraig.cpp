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
   GateList dfsList;
   dfsList.reserve(_headerInfo[0] + _headerInfo[3] + 1);
   updateDfsList(dfsList);

   while (!_fecGrps.empty()) {
      size_t count = 0;
      GateList mergeBase(_fecGrps.size(), 0);
      if (_totGates[0]->getFecGrpIdx() < _fecGrps.size())
         mergeBase[_totGates[0]->getFecGrpIdx()] = _totGates[0];
      vector<unsigned*> mergeList;
      vector<ull> pat(_PIIds.size(), 0);
      SatSolver* s = new SatSolver;
      s->initialize();
      genProofModel(s, dfsList);
      for (auto& g : dfsList) {
         if (g->getType() != AIG_GATE || g->getFecGrpIdx() >= _fecGrps.size())
            continue;
         
         if (mergeBase[g->getFecGrpIdx()] == 0) {
            mergeBase[g->getFecGrpIdx()] = g;
            continue;
         }

         bool result = proofFec(s, mergeBase[g->getFecGrpIdx()], g);
         if (result) {
            for (size_t i = 0; i < _PIIds.size(); i++) {
               pat[i] |= ull(s->getValue(_totGates[_PIIds[i]]->getVar())) << count;
            }
            count++;
            if (_totGates[0]->getFecGrpIdx() != g->getFecGrpIdx())
               mergeBase[g->getFecGrpIdx()] = g;
         }
         else {
            IdList* fecGrp = _fecGrps[g->getFecGrpIdx()];
            auto it = find(fecGrp->begin(), fecGrp->end(), g->getGid());
            _fecGrps[g->getFecGrpIdx()]->erase(it);
            mergeList.push_back(new unsigned[2] {mergeBase[g->getFecGrpIdx()]->getGid(), g->getGid()});
         }
         if ((count & 63) == 0 && count > 0) 
            break;
      }
      if (!mergeList.empty()) {
         mergeFec(mergeList);
         updateDfsList(dfsList);
         for (auto& fecGrp : _fecGrps)
            fecGrp->clear();

         simulate(dfsList, 64);

         if (_totGates[0]->getFecGrpIdx() < _fecGrps.size())
            _fecGrps[_totGates[0]->getFecGrpIdx()]->push_back(_totGates[0]->getGid());
         for (auto& g : dfsList) {
            if (g->isAig()) {
               if (g->getFecGrpIdx() < _fecGrps.size())
                  _fecGrps[g->getFecGrpIdx()]->push_back(g->getGid());
            }
         }
         identifyFec();
         setFecGrpIdx();
         cout << "Updating by UNSAT... Total #FEC Group = " << _fecGrps.size() << "\n";
      }

      if (count) {
         for (size_t i = 0; i < _PIIds.size(); i++) 
            _totGates[_PIIds[i]]->setPattern(pat[i]);
         fill(pat.begin(), pat.end(), 0);
         simulate(dfsList, count);
         identifyFec();
         setFecGrpIdx();
         count = 0;
         cout << "Updating by SAT... Total #FEC Group = " << _fecGrps.size() << "\n";
      }

      delete s;
   }
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::genProofModel(SatSolver*& s, GateList& dfsList)
{
   for (auto& g : dfsList) {
      if (g->getType() == AIG_GATE || g->getType() == PI_GATE || g->getType() == CONST_GATE) {
         Var v = s->newVar();
         g->setVar(v);
      }
   }
   for (auto& g : dfsList) {
      if (g->getType() == AIG_GATE) {
         s->addAigCNF(g->getVar(), g->getFanin(0).gate()->getVar(), g->getFanin(0).isInv(), g->getFanin(1).gate()->getVar(), g->getFanin(1).isInv());
      }
   }
}

bool
CirMgr::proofFec(SatSolver*& s, CirGate* g1, CirGate* g2)
{
   Var newV = s->newVar();
   bool isInv = (~(g1->getPattern()) == g2->getPattern());
   s->addXorCNF(newV, g1->getVar(), false, g2->getVar(), isInv);
   s->assumeRelease();
   s->assumeProperty(newV, true);
   s->assertProperty(_totGates[0]->getVar(), false);
   cout << "Proving (" << g1->getGid() << ", " << (isInv ? "!" : "") << g2->getGid() << ")..." << flush;
   bool result = s->assumpSolve();
   cout << (result ? "SAT" : "UNSAT") << "!!" << flush << "\r" << setw(40) << " " << "\r";
   return result;
}

void
CirMgr::mergeFec(vector<unsigned*>& mergeList)
{
   cout << "\n";
   for (auto& l : mergeList) {
      bool isInv = (~(_totGates[l[0]]->getPattern()) == _totGates[l[1]]->getPattern());
      _totGates[l[0]]->merge(_totGates[l[1]], isInv);
      _headerInfo[4]--;
      delete _totGates[l[1]];
      _totGates[l[1]] = 0;
      cout << "Fraig: " << l[0] << " merging " << (isInv ? "!" : "") << l[1] << "...\n";
      delete[] l;
   }
}
