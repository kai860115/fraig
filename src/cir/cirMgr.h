/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include "cirGate.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr() : _initFec(false), _fecGrps(0) {}
   ~CirMgr() {
      for (size_t i = 0; i < _totGates.size(); i++) {
         if (_totGates[i]) {
            delete _totGates[i];
            _totGates[i] = 0;
         }
      }
      for (auto& fecGrp : _fecGrps) {
         if (fecGrp)
            delete fecGrp;
         fecGrp = 0;
      }
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { 
      return (gid >= _totGates.size() ? 0 : _totGates[gid]);
   }
   IdList* getFecGrp(const size_t& id) { return (id < _fecGrps.size() ? _fecGrps[id] : 0); }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Travelsal
   void dfsTraversal(CirGate*) const;
   void dfsTraversal(CirGate*, GateList&) const;

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;
   IdList _PIIds;
   IdList _POIds;
   GateList _totGates;
   unsigned _headerInfo[5];
   bool _initFec;
   vector<IdList*> _fecGrps;
   void initFecGrps(GateList&);
   void simulate(GateList&, size_t);
   void identifyFec();
   void updateDfsList(GateList&) const;
   void genProofModel(SatSolver*&, GateList&);
   bool proofFec(SatSolver*&, CirGate*, CirGate*);
   void mergeFec(vector<unsigned*>&);
   void setFecGrpIdx(GateList&);

};

#endif // CIR_MGR_H
