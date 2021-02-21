/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class AigGateV {
#define NEG1 0x1
public:
   AigGateV(CirGate* g, size_t phase, unsigned gid) : _gateV(size_t(g) + phase), _gid(gid) {}
   AigGateV(const AigGateV& gv) : _gateV(gv._gateV), _gid(gv._gid) {}
   AigGateV() {}
   CirGate* gate() const {
      return (CirGate*)(_gateV & ~size_t(NEG1));
   }
   unsigned getGid() const { return _gid; }
   size_t getGateV() const { return _gateV; }
   bool isInv() const {
      return (_gateV & NEG1);
   }
   bool operator == (const AigGateV& gv) const { return _gateV == gv._gateV; }

private:
   size_t _gateV;
   unsigned _gid;
};

class CirGate
{
public:
   CirGate() : _ref(0), _gid(0), _lineNo(0), _pattern(0) {}
   CirGate(unsigned gid, unsigned lineNo) : _ref(0), _gid(gid), _lineNo(lineNo) , _pattern(0) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const { return ""; }
   virtual GateType getType() const { return TOT_GATE; }
   virtual string getName() const { return ""; }
   virtual AigGateV getFanin(unsigned i) const { return AigGateV(0, 0, 0); }
   virtual AigGateV getFanout(unsigned i) const { return AigGateV(0, 0, 0); }
   virtual size_t getFanoutSize() const { return 0; }
   unsigned getGid() const { return _gid; }
   unsigned getLineNo() const { return _lineNo; }
   virtual bool isAig() const { return false; }
   bool isGlobalRef() const { return (_ref == _globalRef); }
   void setToGlobalRef() const { _ref = _globalRef; }
   static void setGlobalRef() { _globalRef++; }
   virtual bool checkFFanin() const { return false; } 
   virtual bool checkUnused() const { return false; }
   virtual void setFanin(const AigGateV& g1, const AigGateV& g2) {}
   virtual void setFanin(const AigGateV& g) {}
   virtual void setFanout(const AigGateV& g, size_t idx) {}
   virtual void replaceFanin(const AigGateV& og, const AigGateV& ng) {}
   virtual void addFanout(const AigGateV& g) {}
   virtual void removeFanout(const AigGateV& g) {}
   virtual void setName(string s) {}
   virtual void resizeFaninList(size_t n) {}
   virtual void reserveFaninList(size_t n) {}
   virtual void sortFanoutList() {}
   virtual bool hasConstFanin(unsigned id) const { return false; }
   virtual bool hasIdenticalFanin() const { return false; }
   virtual bool hasInvertedFanin() const { return false; }
   virtual void merge(CirGate* g, bool isInv) {
      for (size_t i = 0; i < 2; i++) {
         CirGate* fanin = g->getFanin(i).gate();
         bool isInvOld = g->getFanin(i).isInv();
         if (fanin)
            fanin->removeFanout(AigGateV(g, isInvOld, g->getGid()));
      }

      for (size_t i = 0; i < g->getFanoutSize(); i++) {
         CirGate* fanout = g->getFanout(i).gate();
         bool isInvOld = g->getFanout(i).isInv();
         bool isInvNew = (!isInv != !isInvOld);
         if (fanout) {
            fanout->replaceFanin(AigGateV(g, isInvOld, g->getGid()), AigGateV(this, isInvNew, _gid));
            addFanout(AigGateV(fanout, isInvNew, fanout->getGid()));
         }
      }
   }
   void setPattern(const ull& pattern) { _pattern = pattern; }
   ull getPattern() const { return _pattern; }
   virtual void simulation() {}
   virtual void setFecGrpIdx(const unsigned& i) {}
   virtual unsigned getFecGrpIdx() const { return UINT_MAX; }
   Var getVar() const { return _var; }
   void setVar(const Var& v) { _var = v; }


   // Printing functions
   virtual void printGate() const = 0;
   virtual void write(ostream&) const = 0;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;
   virtual void reportFanin(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid << "\n";
   }
   virtual void reportFanout(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid << "\n";
   }

private:

protected:
  static size_t _globalRef;
  mutable size_t _ref;
  unsigned _gid;
  unsigned _lineNo;
  ull _pattern;
  Var _var;

};

class AigGate : public CirGate {
public:
   AigGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo), _fecGrpIdx(UINT_MAX) {}
   ~AigGate() {}
   
   string getTypeStr() const { return "AIG"; }
   GateType getType() const { return AIG_GATE; }
   AigGateV getFanin(unsigned i) const { return (i < 2 ? _fanin[i] : AigGateV(0, 0, 0)); }
   AigGateV getFanout(unsigned i) const { return _fanoutList[i]; }
   size_t getFanoutSize() const { return _fanoutList.size(); }
   bool isAig() const { return true; }

   bool checkFFanin() const { return (_fanin[0].gate())->getTypeStr() == "UNDEF" || (_fanin[1].gate())->getTypeStr() == "UNDEF"; } 
   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanin(const AigGateV& g1, const AigGateV& g2) {
      _fanin[0] = g1;
      _fanin[1] = g2;
   }
   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void replaceFanin(const AigGateV& og, const AigGateV& ng) {
      for (size_t i = 0; i < 2; i++) {
         if (_fanin[i] == og) {
            _fanin[i] = ng;
            return;
         }
      }
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }
   void removeFanout(const AigGateV& g) {
      auto it = find(_fanoutList.begin(), _fanoutList.end(), g);
      if (it != _fanoutList.end())
         _fanoutList.erase(it);
   }

   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }
   bool hasConstFanin(unsigned id) const {
      for (size_t i = 0; i < 2; i++) {
         if (_fanin[i].gate()->getType() == CONST_GATE && _fanin[i].isInv() == bool(id))
            return true;
      }
      return false;
   }
   bool hasIdenticalFanin() const {
      if (_fanin[0].gate() == _fanin[1].gate() && _fanin[0].isInv() == _fanin[1].isInv())
         return true;
      return false;
   }
   bool hasInvertedFanin() const {
      if (_fanin[0].gate() == _fanin[1].gate() && _fanin[0].isInv() != _fanin[1].isInv())
         return true;
      return false;
   }
   void simulation() {
      _pattern = (_fanin[0].isInv() ? ~_fanin[0].gate()->getPattern() : _fanin[0].gate()->getPattern()) & (_fanin[1].isInv() ? ~_fanin[1].gate()->getPattern() : _fanin[1].gate()->getPattern());
   }
   void setFecGrpIdx(const unsigned& i) { _fecGrpIdx = i; }
   unsigned getFecGrpIdx() const { return _fecGrpIdx; }

   void printGate() const {
      cout << "AIG " << _gid << " "
           << (_fanin[0].gate()->getTypeStr() == "UNDEF" ? "*" : "") << (_fanin[0].isInv() ? "!" : "") << (_fanin[0].gate())->getGid() << " "
           << (_fanin[1].gate()->getTypeStr() == "UNDEF" ? "*" : "") << (_fanin[1].isInv() ? "!" : "") << (_fanin[1].gate())->getGid() << "\n";
   }
   void write(ostream& outfile) const {
      outfile << _gid * 2 << " " 
              << _fanin[0].gate()->getGid() * 2 + unsigned(_fanin[0].isInv()) << " "
              << _fanin[1].gate()->getGid() * 2 + unsigned(_fanin[1].isInv()) << "\n";
   }
   void reportFanin(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (isGlobalRef())
            cout << " (*)\n";
         else {
            setToGlobalRef();
            cout << "\n";
            (_fanin[0].gate())->reportFanin(level - 1, nSpace + 2, _fanin[0].isInv());
            (_fanin[1].gate())->reportFanin(level - 1, nSpace + 2, _fanin[1].isInv());
         }
      }
      else 
         cout << "\n";
   }
   void reportFanout(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (!_fanoutList.empty()) {
            if (isGlobalRef())
               cout << " (*)\n";
            else {
               setToGlobalRef();
               cout << "\n";
               for (auto& gv : _fanoutList)
                  (gv.gate())->reportFanout(level - 1, nSpace + 2, gv.isInv());
            }
         }
         else 
            cout << "\n";
      }
      else 
         cout << "\n";
   }

private:
   AigGateV _fanin[2];
   vector<AigGateV> _fanoutList;
   unsigned _fecGrpIdx;

};

class PIGate : public CirGate {
public:
   PIGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo) {}
   ~PIGate() {}

   string getTypeStr() const { return "PI"; }
   GateType getType() const { return PI_GATE; }
   string getName() const { return _name; }
   AigGateV getFanout(unsigned i) const { return _fanoutList[i]; }
   size_t getFanoutSize() const { return _fanoutList.size(); }

   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }
   void removeFanout(const AigGateV& g) {
      auto it = find(_fanoutList.begin(), _fanoutList.end(), g);
      if (it != _fanoutList.end())
         _fanoutList.erase(it);
   }
   void setName(string s) { _name = s; }

   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }

   void printGate() const {
      cout << "PI  " << _gid << (_name == "" ? "\n" : (" (" + _name + ")\n")); 
   }
   void write(ostream& outfile) const {
      outfile << _gid * 2 << "\n";
   }
   void reportFanout(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (!_fanoutList.empty()) {
            if (isGlobalRef())
               cout << " (*)\n";
            else {
               setToGlobalRef();
               cout << "\n";
               for (auto& gv : _fanoutList)
                  (gv.gate())->reportFanout(level - 1, nSpace + 2, gv.isInv());
            }
         }
         else 
            cout << "\n";
      }
      else 
         cout << "\n";
   }

private:
   string _name;
   vector<AigGateV> _fanoutList;

};

class POGate : public CirGate {
public:
   POGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo) {}
   ~POGate() {}

   string getTypeStr() const { return "PO"; }
   GateType getType() const { return PO_GATE; }
   string getName() const { return _name; }
   AigGateV getFanin(unsigned i) const { return (i < 1 ? _fanin : AigGateV(0, 0, 0)); }
   
   bool checkFFanin() const { return (_fanin.gate())->getTypeStr() == "UNDEF"; } 

   void setFanin(const AigGateV& g) { _fanin = g; }
   void replaceFanin(const AigGateV& og, const AigGateV& ng) {
      if (_fanin == og) {
         _fanin = ng;
      }
   }
   void setName(string s) { _name = s; }
   void simulation() {
      _pattern = (_fanin.isInv() ? ~_fanin.gate()->getPattern() : _fanin.gate()->getPattern());
   }
   
   void printGate() const {
      cout << "PO  " << _gid << " "
           << (_fanin.gate()->getTypeStr() == "UNDEF" ? "*" : "") << (_fanin.isInv() ? "!" : "") << (_fanin.gate())->getGid()
           << (_name == "" ? "\n" : (" (" + _name + ")\n"));
   }
   void write(ostream& outfile) const {
      outfile << _fanin.gate()->getGid() * 2 + unsigned(_fanin.isInv()) << "\n";
   }
   void reportFanin(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (isGlobalRef())
            cout << " (*)\n";
         else {
            setToGlobalRef();
            cout << "\n";
            (_fanin.gate())->reportFanin(level - 1, nSpace + 2, _fanin.isInv());
         }
      }
      else 
         cout << "\n";
   }

private:
   string _name;
   AigGateV _fanin;

};

class CONST0Gate : public CirGate {
public:
   CONST0Gate() : CirGate() {}
   ~CONST0Gate() {}
   
   string getTypeStr() const { return "CONST"; }
   GateType getType() const { return CONST_GATE; }
   AigGateV getFanout(unsigned i) const { return _fanoutList[i]; }
   size_t getFanoutSize() const { return _fanoutList.size(); }

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }
   void removeFanout(const AigGateV& g) {
      auto it = find(_fanoutList.begin(), _fanoutList.end(), g);
      if (it != _fanoutList.end())
         _fanoutList.erase(it);
   }

   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }
   void setFecGrpIdx(const unsigned& i) { _fecGrpIdx = i; }
   unsigned getFecGrpIdx() const { return _fecGrpIdx; }
   
   void printGate() const {
      cout << "CONST0\n";
   }
   void write(ostream& outfile) const {}
   void reportFanout(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (!_fanoutList.empty()) {
            if (isGlobalRef())
               cout << " (*)\n";
            else {
               setToGlobalRef();
               cout << "\n";
               for (auto& gv : _fanoutList)
                  (gv.gate())->reportFanout(level - 1, nSpace + 2, gv.isInv());
            }
         }
         else 
            cout << "\n";
      }
      else 
         cout << "\n";
   }

private:
   vector<AigGateV> _fanoutList;
   unsigned _fecGrpIdx;

};

class UNDEFGate : public CirGate {
public:
   UNDEFGate(unsigned gid) : CirGate(gid, 0) {}
   ~UNDEFGate() {}
   
   string getTypeStr() const { return "UNDEF"; }
   GateType getType() const { return UNDEF_GATE; }
   AigGateV getFanout(unsigned i) const { return _fanoutList[i]; }
   size_t getFanoutSize() const { return _fanoutList.size(); }
   
   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }
   void removeFanout(const AigGateV& g) {
      auto it = find(_fanoutList.begin(), _fanoutList.end(), g);
      if (it != _fanoutList.end())
         _fanoutList.erase(it);
   }
   
   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }
   
   void printGate() const {
      cout << "UNDEF" << _gid << "\n";
   }
   void write(ostream& outfile) const {}
   void reportFanout(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (!_fanoutList.empty()) {
            if (isGlobalRef())
               cout << " (*)\n";
            else {
               setToGlobalRef();
               cout << "\n";
               for (auto& gv : _fanoutList)
                  (gv.gate())->reportFanout(level - 1, nSpace + 2, gv.isInv());
            }
         }
         else 
            cout << "\n";
      }
      else 
         cout << "c\n";
   }

private:
   vector<AigGateV> _fanoutList;

};

#endif // CIR_GATE_H
