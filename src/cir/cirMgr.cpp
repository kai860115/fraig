/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr
enum ParseType {
   AAG,
   NUM_OF_VAR,
   NUM_OF_PI,
   NUM_OF_LATCH,
   NUM_OF_PO,
   NUM_OF_AIG,
   PI,
   PO,
   PI_INDEX,
   PO_INDEX,
   AIG_G,
   AIG_IN,
   SYMBOLIC_NAME,
   NEWLINE,
   BREAK,
   SC
};

void
parseType2Str(ParseType pt, string& s) {
   switch (pt) {
      case AAG: s = "aag"; break;
      case NUM_OF_VAR: s = "number of variables"; break;
      case NUM_OF_PI: s = "number of PIs"; break;
      case NUM_OF_LATCH: s = "number of latches"; break;
      case NUM_OF_PO: s = "number of POs"; break;
      case NUM_OF_AIG: s = "number of AIGs"; break;
      case PI: s = "PI literal ID"; break;
      case PO: s = "PO literal ID"; break;
      case AIG_G: s = "AIG Gate literal ID"; break;
      case AIG_IN: s = "AIG Input literal ID"; break;
      case PI_INDEX: s = "symbol index"; break;
      case PO_INDEX: s = "symbol index"; break;

      default:
         break;
   }
}

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static size_t bufLen = 0;
static string errMsg;
static int errInt;
static CirGate *errGate;
static ParseType type;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

bool parseError(CirParseError err, string& s, int& num, char& ch)
{
   switch (err)
   {
      case MISSING_NEWLINE:
         break;
      case ILLEGAL_SYMBOL_NAME:
         errInt = ch;
         break;
      case EXTRA_SPACE:
         break;
      case MISSING_IDENTIFIER:
         errMsg = (type == SYMBOLIC_NAME ? "symbolic name" : "aag");
         break;
      case ILLEGAL_IDENTIFIER:
         errMsg = s;
         break;
      case MISSING_NUM:
         parseType2Str(type, errMsg);
         break;
      case ILLEGAL_SYMBOL_TYPE:
         errMsg = ch;
         break;
      case ILLEGAL_WSPACE:
         errInt = ch;
         break;
      case ILLEGAL_NUM:
         parseType2Str(type, errMsg);
         errMsg += "(" + s + ")";
         break;
      case NUM_TOO_BIG:
         errInt = num;
         errMsg = (type == PI_INDEX ? "PI index" : "PO index");
         break;
      case MAX_LIT_ID:
         errInt = num;
         break;
      case REDEF_CONST:
         errInt = num;
         break;
      case CANNOT_INVERTED:
         errMsg = (type == PI ? "PI" : "AIG Gate");
         errInt = num;
         break;
      default:
         break;
   }
   return parseError(err);
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool 
parse(string& s, unsigned& id, char endChar = 0) {
   s = "";
   id = 0;
   size_t begin = colNo;
   int num = 0;
   char c;
   while (colNo <= bufLen) {
      c = buf[colNo];
      if (type == NEWLINE) {
         if (c != 0)
            return parseError(MISSING_NEWLINE);
      }
      else if (type == BREAK)
         break;
      else if (type == SYMBOLIC_NAME) {
         while (c) {
            if (!isprint(c)) {
               return parseError(ILLEGAL_SYMBOL_NAME, s, num, c);
            }
            s += c;
            c = buf[++colNo];
         }
         colNo--;
         type = BREAK;
      }
      else {
         if (s.empty() && (isspace(c) || !c)) {
            if (c == ' ')
               return parseError(EXTRA_SPACE);
            else if (c == 0) {
               if (type == SYMBOLIC_NAME) {
                  return parseError(MISSING_IDENTIFIER, s, num, c);
               }
               else if (type >= 1 && type <= 5) {
                  return parseError(MISSING_NUM, s, num, c);
               }
               else if (type == AAG) {
                  return parseError(MISSING_IDENTIFIER, s, num, c); 
               }
               else if (type == SC) {
                  return parseError(ILLEGAL_SYMBOL_TYPE, s, num, c);
               }
               else {
                  return parseError(MISSING_NUM, s, num, c);
               } 
            }
            else {
               return parseError(ILLEGAL_WSPACE, s, num, c);
            }
         }
         if (type == SC) {
            if (c != 'i' && c != 'o' && c != 'c') {
               return parseError(ILLEGAL_SYMBOL_TYPE, s, num, c);
            }
            else if (c == 'c') 
               type = NEWLINE;
            else 
               type = BREAK;
            s += c;

         }
         else {
            if (isspace(c) || c == 0) {
               // aag: "aag"
               if (type == AAG) {
                  if (s != "aag") {
                     errMsg = s;
                     return parseError(ILLEGAL_IDENTIFIER);
                  }
               }
               // PI | AIG gate | AIG input | PO | PI index | PO index | number of ... : num
               else {
                  // not num
                  if (!myStr2Int(s, num) || num < 0) {
                     return parseError(ILLEGAL_NUM, s, num, c);
                  }
                  // PI index | PO index too big || ID too big
                  if (num > errInt && (type >= 6 && type <= 11)) {
                     colNo = begin;
                     if (type == PI_INDEX || type == PO_INDEX) 
                        return parseError(NUM_TOO_BIG, s, num, c);
                     else 
                        return parseError(MAX_LIT_ID, s, num, c);
                  }
                  // PI | AIG gate: odd and >= 2
                  if (type == PI || type == AIG_G) {
                     if (num < 2) {
                        colNo = begin;
                        return parseError(REDEF_CONST, s, num, c);
                     }
                     if (num % 2) {
                        colNo = begin;
                        return parseError(CANNOT_INVERTED, s, num, c);
                     }
                  }
                  id = unsigned(num);
               }
               
               if (endChar == 0 && c != endChar)         // newline end
                  return parseError(MISSING_NEWLINE);
               else if (c != endChar)                    // missing space
                  return parseError(MISSING_SPACE);
               type = BREAK;
            }
            else {
               while (!isspace(c) && c) {
                  s += c;
                  c = buf[++colNo];
               }
               colNo--;
            }
         }
      }
      colNo++;
   }
   if (s.empty()) {
      if (type == SYMBOLIC_NAME || type == AAG) 
         return parseError(MISSING_IDENTIFIER, s, num, c);
      else 
         return parseError(MISSING_NUM, s, num, c);
   }
   
   return true;
}

bool
CirMgr::readCircuit(const string& fileName)
{
   lineNo = 0;
   colNo = 0;
   ifstream ifs(fileName.c_str());
   if (!ifs) {
      cerr << "Cannot open design \"" + fileName + "\"!!" << endl;
      return false; 
   }
   string s;
   s.reserve(1024);
   unsigned id;

   // Header
   ifs.getline(buf, 1024);
   bufLen = strlen(buf);
   type = AAG;
   if (!parse(s, id, ' '))
      return false;
   if (s != "aag") {
      errMsg = s;
      return parseError(ILLEGAL_IDENTIFIER);
   }
   for (size_t i = 0; i < 5; i++) {
      switch (i) {
         case 0: type = NUM_OF_VAR; break;
         case 1: type = NUM_OF_PI; break;
         case 2: type = NUM_OF_LATCH; break;
         case 3: type = NUM_OF_PO; break;
         case 4: type = NUM_OF_AIG; break;
         default: break;
      }
      char endChar = (i == 4 ? 0 : ' ');
      if (!parse(s, id, endChar))
         return false;
      
      if (s.empty()) {
         return parseError(MISSING_NUM);
      }
      _headerInfo[i] = id;
   }
   if (_headerInfo[0] < _headerInfo[1] + _headerInfo[2] + _headerInfo[4]) {
      errMsg = "Number of variables";
      errInt = _headerInfo[0];
      return parseError(NUM_TOO_SMALL);
   }
   if (_headerInfo[2] > 0) {
      errMsg = "latches";
      return parseError(ILLEGAL_NUM);
   }

   colNo = 0;
   lineNo++;
   _totGates = GateList(_headerInfo[0] + _headerInfo[3] + 1, 0);
   _PIIds.reserve(_headerInfo[1]);
   _POIds.reserve(_headerInfo[3]);
   vector<size_t> fanoutCapacity(_headerInfo[0] + 1, 1);

   // CONST0
   _totGates[0] = new CONST0Gate();

   // Inputs
   errInt = 2 * _headerInfo[0] + 1;
   for (size_t i = 0; i < _headerInfo[1]; i++) {
      type = PI;
      if (!ifs.getline(buf, 1024) || ifs.eof())
         break;
      bufLen = strlen(buf);

      if (!parse(s, id, 0))
         return false;
      
      if (_totGates[id / 2]) {
         errGate = _totGates[id / 2];
         errInt = id;
         return parseError(REDEF_GATE);
      }

      _totGates[id / 2] = new PIGate(id / 2, lineNo + 1);
      _PIIds.push_back(id / 2);
      colNo = 0;
      lineNo++;
   }
   if (_PIIds.size() < _headerInfo[1]) {
      errMsg = "PI";
      return parseError(MISSING_DEF);
   }

   // Latches (skip)

   // Outputs
   vector<unsigned> POInfo(_headerInfo[3], 0);
   for (size_t i = 0; i < _headerInfo[3]; i++) {
      type = PO;
      if (!ifs.getline(buf, 1024) || ifs.eof())
         break;
      bufLen = strlen(buf);

      if (!parse(s, id, 0))
         return false;
      
      _totGates[_headerInfo[0] + i + 1] = new POGate(_headerInfo[0] + i + 1, lineNo + 1);
      _POIds.push_back(_headerInfo[0] + i + 1);
      fanoutCapacity[id / 2]++;
      POInfo[i] = id;
      colNo = 0;
      lineNo++;
   }
   if (_POIds.size() < _headerInfo[3]){
      errMsg = "PO";
      return parseError(MISSING_DEF);
   };

   // Ands
   vector<vector<unsigned>> AigInfo(_headerInfo[4], vector<unsigned>(3, 0));
   size_t nAigCheck = 0;
   for (size_t i = 0; i < _headerInfo[4]; i++) {
      if (!ifs.getline(buf, 1024) || ifs.eof())
         break; 
      bufLen = strlen(buf);

      for (size_t j = 0; j <= 2; j++) {
         type = (j == 0 ? AIG_G : AIG_IN);
         char endChar = (j == 2 ? 0 : ' '); 
         if (!parse(s, id, endChar))
            return false;
         
         AigInfo[i][j] = id;
         if (j == 0) {
            if (_totGates[id / 2]) {
               errGate = _totGates[id / 2];
               errInt = id;
               return parseError(REDEF_GATE);
            }
            _totGates[id / 2] = new AigGate(id / 2, lineNo + 1);
            nAigCheck++;
         }
         else 
            fanoutCapacity[id / 2]++;
      }
      
      colNo = 0;
      lineNo++;
   }
   if (nAigCheck < _headerInfo[4]) {
      errMsg = "AIG";
      return parseError(MISSING_DEF);
   }

   // Symbols and Comments
   while (ifs.getline(buf, 1024) || !ifs.eof()) {
      bufLen = strlen(buf);
      type = SC;
      if (!parse(s, id))
         return false;

      if (s == "c")
         break;
         
      // index
      string s_copy = s;
      type = (s == "i" ? PI_INDEX : PO_INDEX);
      errInt = (s == "i" ? _headerInfo[1] : _headerInfo[3]);
      IdList& l = (s == "i" ? _PIIds : _POIds);

      if (!parse(s, id, ' '))
         return false; 
      
      CirGate* g = _totGates[l[id]];
      unsigned id_copy = id;
      // name
      type = SYMBOLIC_NAME;
      if (!parse(s, id))
         return false;
      
      if (g->getName() != "") {
         errMsg = s_copy;
         errInt = id_copy;
         return parseError(REDEF_SYMBOLIC_NAME);
      }

      g->setName(s);
      
      colNo = 0;
      lineNo++;
   }

   ifs.close();

   for (size_t i = 0; i < fanoutCapacity.size(); i++) {
      if (_totGates[i])
         _totGates[i]->reserveFaninList(fanoutCapacity[i]);
   }

   for (size_t i = 0; i < POInfo.size(); i++) {
      size_t gid = _headerInfo[0] + i + 1;
      size_t fanin_gid = POInfo[i];
      if (!_totGates[fanin_gid / 2]) {
         _totGates[fanin_gid / 2] = new UNDEFGate(fanin_gid / 2);
         _totGates[fanin_gid / 2]->reserveFaninList(fanoutCapacity[fanin_gid / 2]);
      }
      _totGates[gid]->setFanin(AigGateV(_totGates[fanin_gid / 2], fanin_gid % 2, fanin_gid / 2));
      _totGates[fanin_gid / 2]->addFanout(AigGateV(_totGates[gid], fanin_gid % 2, gid));
   }
   for (size_t i = 0; i < AigInfo.size(); i++) {
      size_t gid = AigInfo[i][0];
      for (size_t j = 1; j <= 2; j++) {
         size_t fanin_gid = AigInfo[i][j];
         if (!_totGates[fanin_gid / 2]) {
            _totGates[fanin_gid / 2] = new UNDEFGate(fanin_gid / 2);
            _totGates[fanin_gid / 2]->reserveFaninList(fanoutCapacity[fanin_gid / 2]);
         }
         _totGates[fanin_gid / 2]->addFanout(AigGateV(_totGates[gid / 2], fanin_gid % 2, gid / 2));
      }
      _totGates[gid / 2]->setFanin(AigGateV(_totGates[AigInfo[i][1] / 2], AigInfo[i][1] % 2, AigInfo[i][1] / 2), AigGateV(_totGates[AigInfo[i][2] / 2], AigInfo[i][2] % 2, AigInfo[i][2] / 2));
   }
   
   for (auto& g : _totGates) {
      if (g)
         g->sortFanoutList();
   }

   return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl
        << "Circuit Statistics" << endl
        << "==================" << endl
        << "  PI   " << setw(9) << right << _headerInfo[1] << endl
        << "  PO   " << setw(9) << right << _headerInfo[3] << endl
        << "  AIG  " << setw(9) << right << _headerInfo[4] << endl
        << "------------------" << endl
        << "  Total" << setw(9) << right << _headerInfo[1] + _headerInfo[3] + _headerInfo[4] << endl;
}

void
CirMgr::printNetlist() const
{
   cout << "\n";
   CirGate::setGlobalRef();
   GateList dfsList;
   for (const auto& id : _POIds) {
      dfsTraversal(_totGates[id], dfsList);
   }
   for (size_t i = 0; i < dfsList.size(); i++) {
      cout << "[" << i << "] ";
      dfsList[i]->printGate();
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (const auto& id : _PIIds) 
      cout << " " << id;
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (const auto& id : _POIds) 
      cout << " " << id;
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   IdList fFaninId;
   IdList unusedId;
   for (const auto& g : _totGates) {
      if (g != 0) {
         if (g->checkFFanin()) {
            fFaninId.push_back(g->getGid());
         }
         if (g->checkUnused()) {
            unusedId.push_back(g->getGid());
         }
      }
   }
   if (!fFaninId.empty()) {
      cout << "Gates with floating fanin(s):"; 
      for (const auto& id : fFaninId) 
         cout << " " << id;
      cout << endl;
   }
   if (!unusedId.empty()) {
      cout << "Gates defined but not used  :"; 
      for (const auto& id : unusedId) 
         cout << " " << id;
      cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
   if (_fecGrps.empty())
      return;
   for (size_t i = 0; i < _fecGrps.size(); i++) {
      cout << "[" << i << "]"; 
      for (const auto& id : (*_fecGrps[i])) {
         cout << " " << (_totGates[id]->getPattern() == _totGates[(*_fecGrps[i])[0]]->getPattern() ? "" : "!") << _totGates[id]->getGid();
      }
      cout << "\n";
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   CirGate::setGlobalRef();
   GateList dfsList;
   for (const auto& id : _POIds) {
      dfsTraversal(_totGates[id], dfsList);
   }
   GateList aigList;
   aigList.reserve(_headerInfo[4]);
   for (const auto& g : dfsList) {
      if (g->getTypeStr() == "AIG")
         aigList.push_back(g);
   }
   outfile << "aag";
   for (size_t i = 0; i < 4; i++) 
      outfile << " " << _headerInfo[i];
   outfile << " " << aigList.size();
   outfile << "\n";
   
   for (const auto& id : _PIIds)
      _totGates[id]->write(outfile);
   for (const auto& id : _POIds)
      _totGates[id]->write(outfile);
   for (const auto& g : aigList)
      g->write(outfile);

   for (size_t i = 0; i < _PIIds.size(); i++) {
      if (_totGates[_PIIds[i]]->getName() != "")
         outfile << "i" << i << " " << _totGates[_PIIds[i]]->getName() << "\n";
   }
   for (size_t i = 0; i < _POIds.size(); i++) {
      if (_totGates[_POIds[i]]->getName() != "")
         outfile << "o" << i << " " << _totGates[_POIds[i]]->getName() << "\n";
   }
   outfile << "c\n";
   outfile << "AAG output by Yu-Kai Ling\n";
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}

void 
CirMgr::dfsTraversal(CirGate* g) const {
   if (g->isGlobalRef())
      return;
   g->setToGlobalRef();
   for (size_t i = 0; i < 2; i++) {
      CirGate* fanin = g->getFanin(i).gate();
      if (fanin) 
         dfsTraversal(fanin);
   }
}

void 
CirMgr::dfsTraversal(CirGate* g, GateList& dfsList) const {
   if (g->isGlobalRef())
      return;
   g->setToGlobalRef();
   for (size_t i = 0; i < 2; i++) {
      CirGate* fanin = g->getFanin(i).gate();
      if (fanin != 0)
         dfsTraversal(fanin, dfsList);
   }
   if (g->getTypeStr()[0] != 'U')
      dfsList.push_back(g);
}
