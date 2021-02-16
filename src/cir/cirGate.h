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
   AigGateV(CirGate* g, size_t phase) : _gateV(size_t(g) + phase) {}
   AigGateV(const AigGateV& gv) : _gateV(gv._gateV) {}
   AigGateV() {}
   CirGate* gate() const {
      return (CirGate*)(_gateV & ~size_t(NEG1));
   }
   bool isInv() const {
      return (_gateV & NEG1);
   }

private:
   size_t _gateV;
};

class CirGate
{
public:
   CirGate() : _ref(0), _gid(0), _lineNo(0) {}
   CirGate(unsigned gid, unsigned lineNo) : _ref(0), _gid(gid), _lineNo(lineNo) {}
   virtual ~CirGate() {}

   // Basic access methods
   virtual string getTypeStr() const { return ""; }
   virtual string getName() const { return ""; }
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
   virtual void addFanout(const AigGateV& g) {}
   virtual void setName(string s) {}
   virtual void dfsTravelsal(GateList& dfsList) { 
      if (isGlobalRef())
         return;
      setToGlobalRef(); 
      dfsList.push_back(this); 
   }
   virtual void resizeFaninList(size_t n) {}
   virtual void reserveFaninList(size_t n) {}
   virtual void sortFanoutList() {}


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

};

class AigGate : public CirGate {
public:
   AigGate(unsigned gid, unsigned lineNo, AigGateV fanin1, AigGateV fanin2) : CirGate(gid, lineNo), _fanin1(fanin1), _fanin2(fanin2) {}
   AigGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo) {}
   ~AigGate() {}
   
   string getTypeStr() const { return "AIG"; }

   bool checkFFanin() const { return (_fanin1.gate())->getTypeStr() == "UNDEF" || (_fanin2.gate())->getTypeStr() == "UNDEF"; } 
   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanin(const AigGateV& g1, const AigGateV& g2) {
      _fanin1 = g1;
      _fanin2 = g2;
   }
   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }

   void dfsTravelsal(GateList& dfsList) {
      if (isGlobalRef())
         return;
      setToGlobalRef();
      _fanin1.gate()->dfsTravelsal(dfsList);
      _fanin2.gate()->dfsTravelsal(dfsList);
      dfsList.push_back(this);
   }
   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }

   void printGate() const {
      cout << "AIG " << _gid << " "
           << (_fanin1.gate()->getTypeStr() == "UNDEF" ? "*" : "") << (_fanin1.isInv() ? "!" : "") << (_fanin1.gate())->getGid() << " "
           << (_fanin2.gate()->getTypeStr() == "UNDEF" ? "*" : "") << (_fanin2.isInv() ? "!" : "") << (_fanin2.gate())->getGid() << "\n";
   }
   void write(ostream& outfile) const {
      outfile << _gid * 2 << " " 
              << _fanin1.gate()->getGid() * 2 + unsigned(_fanin1.isInv()) << " "
              << _fanin2.gate()->getGid() * 2 + unsigned(_fanin2.isInv()) << "\n";
   }
   void reportFanin(int level, int nSpace, bool inv) const {
      cout << string(nSpace, ' ') << (inv ? "!" : "") << getTypeStr() << " " << _gid;
      if (level > 0) {
         if (isGlobalRef())
            cout << " (*)\n";
         else {
            setToGlobalRef();
            cout << "\n";
            (_fanin1.gate())->reportFanin(level - 1, nSpace + 2, _fanin1.isInv());
            (_fanin2.gate())->reportFanin(level - 1, nSpace + 2, _fanin2.isInv());
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
   AigGateV _fanin1;
   AigGateV _fanin2;
   vector<AigGateV> _fanoutList;

};

class PIGate : public CirGate {
public:
   PIGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo) {}
   ~PIGate() {}

   string getTypeStr() const { return "PI"; }
   string getName() const { return _name; }

   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
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
   POGate(unsigned gid, unsigned lineNo, AigGateV fanin) : CirGate(gid, lineNo), _fanin(fanin) {}
   POGate(unsigned gid, unsigned lineNo) : CirGate(gid, lineNo) {}
   ~POGate() {}

   string getTypeStr() const { return "PO"; }
   string getName() const { return _name; }
   
   bool checkFFanin() const { return (_fanin.gate())->getTypeStr() == "UNDEF"; } 

   void setFanin(const AigGateV& g) { _fanin = g; }
   void setName(string s) { _name = s; }
   
   void dfsTravelsal(GateList& dfsList) {
      if (isGlobalRef())
         return;
      setToGlobalRef();
      _fanin.gate()->dfsTravelsal(dfsList);
      dfsList.push_back(this);
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

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }

   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }
   
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

};

class UNDEFGate : public CirGate {
public:
   UNDEFGate(unsigned gid) : CirGate(gid, 0) {}
   ~UNDEFGate() {}
   
   string getTypeStr() const { return "UNDEF"; }
   
   bool checkUnused() const { return _fanoutList.empty(); }

   void setFanout(const AigGateV& g, size_t idx) {
      _fanoutList[idx] = g;
   }
   void addFanout(const AigGateV& g) {
      _fanoutList.push_back(g);
   }
   
   void sortFanoutList() {
      sort(_fanoutList.begin(), _fanoutList.end(), [](const AigGateV& g1, const AigGateV& g2) {
         return g1.gate()->getGid() * 2 + unsigned(g1.isInv()) < g2.gate()->getGid() * 2 + unsigned(g2.isInv());
      });
   }
   void resizeFaninList(size_t n) { _fanoutList.resize(n); }
   void reserveFaninList(size_t n) { _fanoutList.reserve(n); }

   void dfsTravelsal(GateList& dfsList) { setToGlobalRef(); }
   
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
