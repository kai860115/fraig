/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;
size_t CirGate::_globalRef = 0;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
   string gateInfo = getTypeStr() + "(" + to_string(_gid) + ")" + (getName() == "" ? "" : "\"" + getName() + "\"") + ", line " + to_string(_lineNo);
   cout << "================================================================================\n";
   cout << "= " << setw(77) << left << gateInfo << "\n";
   cout << "= FECs:";
   IdList* fecGrp = cirMgr->getFecGrp(getFecGrpIdx());
   if (fecGrp) {
      for (auto& id : (*fecGrp)) {
         if (id == _gid)
            continue;
         cout << " " << (cirMgr->getGate(id)->getPattern() == _pattern ? "" : "!") << id;
      }
   }
   cout << "\n";
   cout << "= Value: ";
   for (int i = 63; i >= 0; i--) 
      cout << (((i + 1) & 7) == 0 && i != 63 ? "_" : "") << ((_pattern >> i) & 1);
   cout << "\n";
   cout << "================================================================================\n";
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   reportFanin(level, 0, false);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   CirGate::setGlobalRef();
   reportFanout(level, 0, false);
}

