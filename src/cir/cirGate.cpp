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
   cout << "==================================================\n";
   cout << "= " << setw(47) << left << gateInfo << "=\n";
   cout << "==================================================\n";
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

