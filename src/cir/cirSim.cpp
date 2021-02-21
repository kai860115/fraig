/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "cirDef.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
class SimValue
{
public:
   SimValue(ull pattern) : _pattern(pattern) {}

   size_t operator() () const { return (_pattern > ~_pattern ? _pattern : ~_pattern); }

   bool operator == (const SimValue& k) const {
      return (k._pattern == _pattern) || (k._pattern == ~_pattern);
   }

private:
   ull _pattern;
};


/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
   GateList dfsList;
   dfsList.reserve(_headerInfo[0] + _headerInfo[3] + 1);
   updateDfsList(dfsList);

   if (!_initFec) {
      initFecGrps(dfsList);
      _initFec = true;
   }
   
   size_t limit = log(dfsList.size() + 1) * 5;
   size_t failTimes = 0;
   size_t count = 0;

   while (failTimes < limit) {
      count++;
      size_t oldFecGrpSize = _fecGrps.size();
      for (const auto& id : _PIIds) {
         _totGates[id]->setPattern(((size_t(rnGen(INT_MAX)) << 32) | size_t(rnGen(INT_MAX))));
      }
      simulate(dfsList, 64);
      identifyFec();
      cout << "\rTotal #FEC Group = " << _fecGrps.size() << flush;
      if (_fecGrps.size() == 0)
         break;

      if (oldFecGrpSize == _fecGrps.size())
         failTimes++;
      else 
         failTimes = 0;
   }

   cout << "\r" << count * 64 << " patterns simulated.\n";

   for (auto& fecGrp : _fecGrps) 
      sort(fecGrp->begin(), fecGrp->end());
   sort(_fecGrps.begin(), _fecGrps.end(), [](IdList* a, IdList* b) { return a->front() < b->front(); });

   setFecGrpIdx();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
   GateList dfsList;
   dfsList.reserve(_headerInfo[0] + _headerInfo[3] + 1);
   updateDfsList(dfsList);

   if (!_initFec)
      initFecGrps(dfsList);

   string line = "";
   size_t nPat = 0;
   bool error = false;
   vector<ull> pat(_PIIds.size(), 0);
   while (patternFile >> line) {
      if (line.empty())
         continue;
      nPat++;
      if (line.length() != _PIIds.size()) {
         cerr << "\nError: Pattern(" << line << ") length(" << line.length() << ") does not match the number of inputs(" << _PIIds.size() << ") in a circuit!!\n";
         nPat = (nPat >> 6) << 6;
         error = true;
         break;
      }
      for (size_t i = 0; i < _PIIds.size(); i++) {
         if (line[i] != '0' && line[i] != '1') {
            cerr << "\nError: Pattern(" << line << ") contains a non-0/1 character(\'"<< line[i] << "\').\n";
            nPat = (nPat >> 6) << 6;
            error = true;
            break;
         }
         if (line[i] == '1')
            pat[i] |= ull(1) << ((nPat & 63) - 1);
      }
      if (error)
         break;
      if ((nPat & 63) == 0) {
         for (size_t i = 0; i < _PIIds.size(); i++) 
            _totGates[_PIIds[i]]->setPattern(pat[i]);
         fill(pat.begin(), pat.end(), 0);
         simulate(dfsList, 64);
         identifyFec();
         cout << "\rTotal #FEC Group = " << _fecGrps.size() << flush;
      }
   }

   if (!_initFec && nPat == 0) {
      for (auto& fecGrp : _fecGrps) {
         delete fecGrp;
         fecGrp = 0;
      }
      _fecGrps.clear();
   }

   if ((nPat & 63) != 0) {
      for (size_t i = 0; i < _PIIds.size(); i++) {
         _totGates[_PIIds[i]]->setPattern(pat[i]);
      } 
      fill(pat.begin(), pat.end(), 0);
      simulate(dfsList, (nPat & 63));
      identifyFec();
      cout << "\rTotal #FEC Group = " << _fecGrps.size() << flush;
   }

   if (nPat / 64)
      _initFec = true;

   cout << "\r" << nPat << " patterns simulated.\n";

   for (auto& fecGrp : _fecGrps) 
      sort(fecGrp->begin(), fecGrp->end());
   sort(_fecGrps.begin(), _fecGrps.end(), [](IdList* a, IdList* b) { return a->front() < b->front(); });

   setFecGrpIdx();
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::initFecGrps(GateList& dfsList)
{
   IdList* newFecGrp = new IdList;
   newFecGrp->reserve(dfsList.size());
   newFecGrp->push_back(0);
   for (auto& g : dfsList) {
      if (g && g->getType() == AIG_GATE) {
         newFecGrp->push_back(g->getGid());
      }
   }
   _fecGrps.push_back(newFecGrp);
}

void
CirMgr::simulate(GateList& dfsList, size_t size)
{
   for (auto& g : dfsList) 
      g->simulation();
   if (_simLog) {
      for (size_t i = 0; i < size; i++) {
         for (auto& id : _PIIds) 
            (*_simLog) << (_totGates[id]->getPattern() >> i & 1);
         (*_simLog) << " ";
         for (auto& id : _POIds) 
            (*_simLog) << (_totGates[id]->getPattern() >> i & 1);
         (*_simLog) << "\n";
      }
   }
}

void
CirMgr::identifyFec()
{
   vector<IdList*> newFecGrps;
   newFecGrps.reserve(_headerInfo[4]);
   for (auto& fecGrp : _fecGrps) {
      HashMap<SimValue, IdList*> fecGrpsMap(getHashSize(fecGrp->size()));
      for (auto& id : (*fecGrp)) {
         IdList* newFecGrp;
         SimValue s(_totGates[id]->getPattern());
         if (fecGrpsMap.query(s, newFecGrp)) {
            newFecGrp->push_back(id);
         }
         else {
            newFecGrp = new IdList(1, id);
            fecGrpsMap.insert(_totGates[id]->getPattern(), newFecGrp);
         }
      }

      for (HashMap<SimValue, IdList*>::iterator it = fecGrpsMap.begin(); it != fecGrpsMap.end(); it++) {
         if ((*it).second->size() > 1) 
            newFecGrps.push_back((*it).second);
         else 
            delete (*it).second;
      }
   }
   _fecGrps.swap(newFecGrps);
   for (auto& fecGrp : newFecGrps) {
      if (fecGrp)
         delete fecGrp;
   }
}

void
CirMgr::setFecGrpIdx() 
{
   for (auto& g : _totGates) {
      if (g)
         g->setFecGrpIdx(UINT_MAX);
   }
   for (size_t i = 0; i < _fecGrps.size(); i++) {
      for (auto& id : (*(_fecGrps[i]))) 
         _totGates[id]->setFecGrpIdx(i);
   }
}