// -*-Mode: C++;-*-
//
// topology builder for residue
//

#include <common.h>

#include "TopoBuilder.hpp"

#include "TopoDB.hpp"
#include "MolCoord.hpp"
#include "MolChain.hpp"
#include "MolResidue.hpp"
#include "ResidIterator.hpp"

#include "ResiToppar.hpp"

using namespace molstr;

TopoBuilder::TopoBuilder(TopoDB *pdic)
     : m_pTopDic(pdic)
{
  //
  // setup autogen matrix
  //

  // hydrogen molecule
  m_distmat[0][0] = 0.9;

  // hydrogen covalent bonds
  m_distmat[1][0] = 1.2;
  m_distmat[2][0] = 1.4;
  m_distmat[3][0] = 1.4;
  m_distmat[0][1] = 1.2;
  m_distmat[0][2] = 1.4;
  m_distmat[0][3] = 1.4;

  // light-atom covalent bonds
  m_distmat[1][1] = 1.6;

  // heavy/light-atom covalent bonds
  m_distmat[2][1] = 2.3;
  m_distmat[3][1] = 2.3;
  m_distmat[1][2] = 2.3;
  m_distmat[1][3] = 2.3;

  // heavy atom covalent bond
  // TO DO: check actual observations
  m_distmat[2][2] = 2.3;
  m_distmat[3][2] = 2.3;
  m_distmat[2][3] = 2.3;
  m_distmat[3][3] = 2.3;
}

TopoBuilder::~TopoBuilder()
{
}

void TopoBuilder::applyTopology()
{
  // erase non-persistent bonds (made by the previous applyTopology() call)
  m_pMol->removeNonpersBonds();

  ResidIterator iter(m_pMol);
  MolResiduePtr pPrevRes;
  for (iter.first(); iter.hasMore(); iter.next()) {
    MolResiduePtr pRes = iter.get();
    MB_ASSERT(!pRes.isnull());

//MB_DPRINTLN("Apply top: %s", pRes->toString().c_str());    
    const LString &resnam = pRes->getName();
    ResiToppar *ptop = m_pTopDic->get(resnam);
    if (ptop==NULL || ptop->getAtomNum()==0) {
//MB_DPRINTLN(" ** top for %s not found", resnam.c_str());    
      autogen(pRes, ptop);
      ptop = m_pTopDic->get(resnam);
      if (ptop==NULL) {
        pPrevRes = pRes;
        continue;
      }
    }

    appTopoResid(pRes, ptop);

    if (!pPrevRes.isnull() &&
        pPrevRes->getChainName()==pRes->getChainName())
      appTopo2Resids(pPrevRes, pRes, m_pTopDic);

    pPrevRes = pRes;
  }
}

namespace {
MolBond *bondAtomHelper(MolResiduePtr pRes, MolCoordPtr pMol, std::set<int> &bondedatoms,
                    const LString &nam1, char conf1, const LString &nam2, char conf2)
{
  int aid1 = pRes->getAtomID(nam1, conf1);
  int aid2 = pRes->getAtomID(nam2, conf2);
  if (aid1<0 || aid2<0)
    return NULL;

  MolBond *pMB = pMol->makeBond(aid1, aid2);
  if (pMB==NULL) {
    MB_DPRINTLN("bond %d, %d is already bonded!!", aid1, aid2);
    return NULL;
  }
  //pMB->setType(pBond->type);

  // pMol->bondAtoms(aid1, aid2);
  bondedatoms.insert(aid1);
  bondedatoms.insert(aid2);

  return pMB;
}
}

bool TopoBuilder::appTopoResid(MolResiduePtr pRes, ResiToppar *ptop)
{
  MolCoordPtr pmol = m_pMol;

  MB_ASSERT(ptop!=NULL);
  MB_ASSERT(!pmol.isnull());

  LString pivname = ptop->getPivotAtom();
  // MB_DPRINTLN("AppTop> pivot for %s is %s", pRes->getName().c_str(), pivname.c_str());
  if (!pivname.isEmpty())
    pRes->setPivotAtomName(pivname.c_str());

  LString polytype = ptop->getType();
  // MB_DPRINTLN("AppTop> polymer type for %s is %s", pRes->getName().c_str(), polytype.c_str());
  if (!polytype.isEmpty())
    pRes->setType(polytype);

  //
  // build intra-residue bonds
  //
  ResiToppar::BondList *pBondList = ptop->getBondList();
  ResiToppar::BondList::const_iterator iter = pBondList->begin();
  ResiToppar::BondList::const_iterator bend = pBondList->end();
  std::set<int> bondedatoms;
  for ( ; iter!=bend; iter++) {
    TopBond *pb = *iter;

    std::set<char> conf1, conf2;
    pRes->getAltConfs(pb->a1->name, conf1);
    pRes->getAltConfs(pb->a2->name, conf2);

    if (conf1.empty() && conf2.empty()) {
      MolBond *pMB = bondAtomHelper(pRes, pmol, bondedatoms,
                                    pb->a1->name, 0,
                                    pb->a2->name, 0);
      if (pMB!=NULL) pMB->setType(pb->type);
      continue;
    }
    else if (!conf1.empty() && !conf2.empty()) {
      // merge the sets to conf1
      conf1.insert(conf2.begin(), conf2.end());
      std::set<char>::const_iterator iter = conf1.begin();
      for (; iter!=conf1.end(); ++iter) {
        char confid = *iter;
        MolBond *pMB = bondAtomHelper(pRes, pmol, bondedatoms,
                                      pb->a1->name, confid,
                                      pb->a2->name, confid);
        if (pMB!=NULL) pMB->setType(pb->type);
      }
    }
    else if (conf1.empty()) {
      std::set<char>::const_iterator iter = conf2.begin();
      for (; iter!=conf2.end(); ++iter) {
        char confid = *iter;
        MolBond *pMB = bondAtomHelper(pRes, pmol, bondedatoms,
                                      pb->a1->name, 0,
                                      pb->a2->name, confid);
        if (pMB!=NULL) pMB->setType(pb->type);
      }
    }
    else { // conf2 is empty
      std::set<char>::const_iterator iter = conf1.begin();
      for (; iter!=conf1.end(); ++iter) {
        char confid = *iter;
        MolBond *pMB = bondAtomHelper(pRes, pmol, bondedatoms,
                                      pb->a1->name, confid,
                                      pb->a2->name, 0);
        if (pMB!=NULL) pMB->setType(pb->type);
      }
    }
  }

  // apply atom props
  // bond non-bonded atoms, if apropriate.
  {
    MolResidue::AtomCursor iter = pRes->atomBegin();
    MolResidue::AtomCursor iend = pRes->atomEnd();
    for ( ; iter!=iend; iter++) {
      int aid = iter->second;
      MolAtomPtr pAtom = pmol->getAtom(aid);    
      TopAtom *pTopAtom = ptop->getAtom(pAtom->getName());
      if (pTopAtom!=NULL) {
        // apply atom props
        if (!pTopAtom->elem.isEmpty()) {
          int elem_id = ElemSym::str2SymID(pTopAtom->elem);
          if (elem_id!=ElemSym::XX)
            pAtom->setElement(elem_id);
        }
        pAtom->setCName(pTopAtom->name);
      }
      
      if (bondedatoms.find(aid)!=bondedatoms.end())
        continue;

      // aid is nonbonded atom
      MolResidue::AtomCursor iter2 = pRes->atomBegin();
      MolResidue::AtomCursor iend2 = pRes->atomEnd();
      for ( ; iter2!=iend2; iter2++) {
        int aid2 = iter2->second;
        if (aid==aid2) continue;

        MolAtomPtr pAtom2 = pmol->getAtom(aid2);
        // don't bond atoms in different conformations
        if (pAtom->getConfID()!=pAtom2->getConfID())
          continue;

        // Check topology again, with resolving atom alias name.
        TopAtom *pTopAtom2 = ptop->getAtom(pAtom2->getName());
        if (pTopAtom!=NULL &&
            pTopAtom2!=NULL &&
            ptop->getBond(pTopAtom, pTopAtom2)!=NULL) {
          pmol->makeBond(aid, aid2);
          continue;
        }

        // bond atoms by distance-based criteria
        if (chkBondDist(pAtom, pAtom2)) {
          pmol->makeBond(aid, aid2);
          continue;
        }
      }
    }
  }

  pRes->setTopologyObj(ptop);
  return true;
}

namespace {

bool getLinkAtomIDs(ResiPatch *pLink, MolResiduePtr pRes1, MolResiduePtr pRes2,
                           const char *a1nm, const char *a2nm, int &aid1, int &aid2)
{
  // get the atom of the previous residue side
  char chprev = pLink->getPrevPrefix();
  if (a1nm[0]==chprev)
    aid1 = pRes1->getAtomID(&a1nm[1]);
  else if (a2nm[0]==chprev)
    aid1 = pRes1->getAtomID(&a2nm[1]);
  
  // get the atom of this residue side
  char chthis = pLink->getNextPrefix();
  if (a1nm[0]==chthis)
    aid2 = pRes2->getAtomID(&a1nm[1]);
  else if (a2nm[0]==chthis)
    aid2 = pRes2->getAtomID(&a2nm[1]);
  
  if (aid1<0 || aid2<0)
    return false;

  return true;
}
}

//
// build the inter-residue bond pRes1->pRes2
//
bool TopoBuilder::appTopo2Resids(MolResiduePtr pRes1, MolResiduePtr pRes2, TopoDB *pLinkDict)
{
  // // residue must be owned by the same obj
  // if (pRes1->getParent().get()!=pRes2->getParent().get())
  // return false;
  MolCoordPtr pmol = m_pMol; //pRes1->getParent();

  // Find a link obj suitable for pRes1 and pRes2
  ResiPatch *pLink = pLinkDict->findLink(pRes1, pRes2);
  if (pLink==NULL) {
    // linkage data not exist
    return true;
  }
  
  // get bond obj in the linkage
  TopBond* pBond = pLink->getLinkBond();
  if (pBond==NULL)
    return false; // ERROR!!

  const char *a1nm = pBond->a1name.c_str();
  const char *a2nm = pBond->a2name.c_str();

  if (!pLink->isPolyLink()) {
    //MB_DPRINT("link atom %s, %s\n",
    //pBond->a1name.getConstPrefix(),
    //pBond->a2name.getConstPrefix());

    // Atom1 is a member of previous residue
    // Atom2 is a member of this residue
    int aid1=-1, aid2=-1;

    if (!getLinkAtomIDs(pLink, pRes1, pRes2, a1nm, a2nm, aid1, aid2)) {
      //MB_DPRINTLN("<%s>--<%s> inter residue link not found.",
      //pRes1->getName().c_str(), pRes2->getName().c_str());
      return false;
    }

    //////////
    // check the inter-residue bond distance
    // and determine whether the residues are linked

    double dl = pLink->getLinkDist();
    if (dl>0.0) {
      MolAtomPtr pAtom1 = pmol->getAtom(aid1);
      MolAtomPtr pAtom2 = pmol->getAtom(aid2);
      if (pAtom1.isnull() || pAtom2.isnull())
        return false;
      double d = ( (pAtom1->getPos()) - (pAtom2->getPos()) ).length();
      if (d>dl)
        return false;

      pRes1->setLinkNext(pRes2);
      pmol->makeBond(aid1, aid2);
    }
    else
      pRes1->setLinkNext(MolResiduePtr());

  }
  else {
    // new XML ff impl

    // bonding tolerance: 10.0 sigma
    const double dtol = 10.0;
    const double dl = pBond->r0;
    const double esd = pBond->esd;
    // const double dlow = dl - esd*dtol;
    const double dhigh = dl + esd*dtol;

    int aid1 = pRes1->getAtomID(a1nm);
    int aid2 = pRes2->getAtomID(a2nm);
    if (aid1<0 ||aid2<0)
      return false;

    MolAtomPtr pAtom1 = pmol->getAtom(aid1);
    MolAtomPtr pAtom2 = pmol->getAtom(aid2);
    if (pAtom1.isnull() || pAtom2.isnull())
      return false;
    double d = ( (pAtom1->getPos()) - (pAtom2->getPos()) ).length();
    if (d>dhigh)
      return false;

    pRes1->setLinkNext(pRes2);
    MolBond *pMB = pmol->makeBond(aid1, aid2);

    if (pMB==NULL) {
      MB_DPRINTLN("bond %d, %d is already bonded!!", aid1, aid2);
    }
    else {
      pMB->setType(pBond->type);
    }
  }
  
  return true;
}


/**
  returns atom class.
    0 : hydrogen,
    1 : boron, carbon, nitrogen, oxygen, fluorine
        (light, nonmetal atoms)
    2 : Si, P, S, Cl (heavy nonmetal atoms)
    3 : other heavy atoms
 */
static
int getAtomClass(MolAtomPtr pa)
{
  if (pa->getElement()==ElemSym::H)
    return 0;
  if (ElemSym::B<=pa->getElement() &&
      pa->getElement()<=ElemSym::F)
    return 1;
  if (ElemSym::Si<=pa->getElement() &&
      pa->getElement()<=ElemSym::Cl)
    return 2;
  if (pa->getElement()<=ElemSym::Cl)
    return -1;

  return 3;
}

/** auto-generate and apply topology to pRes */
void TopoBuilder::autogen(MolResiduePtr pRes, ResiToppar *pTop)
{
  if (pRes->getAtomSize()<=1)
    return;

  MolCoordPtr pmol = m_pMol; //pRes->getParent();
  ResiToppar *pNewTop;
  if (pTop==NULL) {
    pNewTop = MB_NEW ResiToppar();
    pNewTop->setName(pRes->getName());
    m_pTopDic->put(pNewTop);
  }
  else {
    MB_ASSERT(pTop->getName().equals(pRes->getName()));
    pNewTop = pTop;
  }
  
  MolResidue::AtomCursor iter, iter2;

  iter = pRes->atomBegin();
  for ( ; iter!=pRes->atomEnd(); iter++) {
    MolAtomPtr pAtom = pmol->getAtom(iter->second);
    pNewTop->addAtom(pAtom->getName(), LString(),
                     0.1, 2.9/1.122, 0.1, 2.6/1.122);
  }

  //

  iter = pRes->atomBegin();
  for ( ; iter!=pRes->atomEnd(); ++iter) {
    MolAtomPtr pAtom = pmol->getAtom(iter->second);

    //iter2 = pRes->atomBegin();
    iter2 = iter;
    ++iter2;
    for ( ; iter2!=pRes->atomEnd(); ++iter2) {
      MolAtomPtr pAtom2 = pmol->getAtom(iter2->second);
      if (pAtom.get()==pAtom2.get())
        continue;
      TopAtom *pra1 = pNewTop->getAtom(pAtom->getName());
      TopAtom *pra2 = pNewTop->getAtom(pAtom2->getName());
      if (pra1==NULL || pra2==NULL)
        continue;
      if (pNewTop->getBond(pra1, pra2)!=NULL)
        continue; // already bonded!
      
      if (!chkBondDist(pAtom, pAtom2))
        continue;

      double dist = (pAtom->getPos() - pAtom2->getPos()).length();
      pNewTop->addBond(pra1->name, pra2->name, 6000.0, dist);
    }
  }

}

bool TopoBuilder::chkBondDist(MolAtomPtr pAtom, MolAtomPtr pAtom2)
{
  double dist = (pAtom->getPos() - pAtom2->getPos()).length();
  if (dist>m_distmat[3][3])
    return false; // too far to bond
  
  int a1t=getAtomClass(pAtom);
  if (a1t<0)
    return false;
  int a2t=getAtomClass(pAtom2);
  if (a2t<0)
    return false;
  if (dist>=m_distmat[a1t][a2t])
    return false;

  return true;
}

