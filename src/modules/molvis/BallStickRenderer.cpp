// -*-Mode: C++;-*-
//
//  Ball & Stick model renderer class
//
//  $Id: BallStickRenderer.cpp,v 1.15 2011/03/29 11:03:44 rishitani Exp $

#include <common.h>
#include "molvis.hpp"

#include "BallStickRenderer.hpp"

#include <modules/molstr/MolCoord.hpp>
#include <modules/molstr/MolChain.hpp>
#include <modules/molstr/MolResidue.hpp>
#include <modules/molstr/ResiToppar.hpp>

using namespace molvis;
using namespace molstr;

using gfx::DisplayContext;
using gfx::ColorPtr;

BallStickRenderer::BallStickRenderer()
{
}

BallStickRenderer::~BallStickRenderer()
{
  MB_DPRINTLN("BallStickRenderer destructed %p", this);
}

const char *BallStickRenderer::getTypeName() const
{
  return "ballstick";
}

////////////

void BallStickRenderer::preRender(DisplayContext *pdc)
{
  pdc->setLighting(true);
}

void BallStickRenderer::postRender(DisplayContext *pdc)
{
  pdc->setLighting(false);
}

void BallStickRenderer::beginRend(DisplayContext *pdl)
{
  if (m_atoms.size()>0)
    m_atoms.erase(m_atoms.begin(), m_atoms.end());

  m_nDetailOld = pdl->getDetail();
  pdl->setDetail(m_nDetail);

}

void BallStickRenderer::endRend(DisplayContext *pdl)
{
  if ( m_fRing && !qlib::isNear4(m_tickness, 0.0) ) {
    drawRings(pdl);
    if (m_atoms.size()>0)
      m_atoms.clear(); //erase(m_atoms.begin(), m_atoms.end());
  }

  pdl->setDetail(m_nDetailOld);
  return;
}

bool BallStickRenderer::isRendBond() const
{
  return true;
}

void BallStickRenderer::rendAtom(DisplayContext *pdl, MolAtomPtr pAtom, bool)
{
  if (m_sphr>0.0) {
    pdl->color(ColSchmHolder::getColor(pAtom));
    pdl->sphere(m_sphr, pAtom->getPos());
  }
  
  checkRing(pAtom->getID());
}

void BallStickRenderer::rendBond(DisplayContext *pdl, MolAtomPtr pAtom1, MolAtomPtr pAtom2, MolBond *pMB)
{
  if (m_bondw>0.0)
    drawInterAtomLine(pAtom1, pAtom2, pdl);
}

void BallStickRenderer::drawInterAtomLine(MolAtomPtr pAtom1, MolAtomPtr pAtom2,
                                          DisplayContext *pdl)
{
  if (pAtom1.isnull() || pAtom2.isnull()) return;

  const Vector4D pos1 = pAtom1->getPos();
  const Vector4D pos2 = pAtom2->getPos();

  ColorPtr pcol1 = ColSchmHolder::getColor(pAtom1);
  ColorPtr pcol2 = ColSchmHolder::getColor(pAtom2);

  if ( pcol1->equals(*pcol2.get()) ) {
    pdl->color(pcol1);
    pdl->cylinder(m_bondw, pos1, pos2);
  }
  else {
    const Vector4D mpos = (pos1 + pos2).divide(2.0);
    pdl->color(pcol1);
    pdl->cylinder(m_bondw, pos1, mpos);
    pdl->color(pcol2);
    pdl->cylinder(m_bondw, pos2, mpos);
  }
}

////////////////////////////////////////////////
// Ring plate drawing

void BallStickRenderer::drawRings(DisplayContext *pdl)
{
  int i, j;
  MolCoordPtr pMol = getClientMol();

  while (m_atoms.size()>0) {
    std::set<int>::iterator iter = m_atoms.begin();
    int aid = *iter;
    m_atoms.erase(iter);

    MolAtomPtr pa = pMol->getAtom(aid);
    if (pa.isnull()) continue;

    MolResiduePtr pres = pa->getParentResidue();
    
    ResiToppar *ptop = pres->getTopologyObj();
    if (ptop==NULL)
      continue;

    // draw rings
    int nrings = ptop->getRingCount();
    for (i=0; i<nrings; i++) {
      const ResiToppar::RingAtomArray *pmembs = ptop->getRing(i);
      std::list<int> ring_atoms;

      // completeness flag of the ring
      bool fcompl = true;

      for (j=0; j<pmembs->size(); j++) {
        LString nm = pmembs->at(j);
        int maid = pres->getAtomID(nm);
        if (maid<=0) {
          fcompl = false;
          break;
        }

        std::set<int>::const_iterator miter = m_atoms.find(maid);
        if (miter==m_atoms.end()) {
          if (aid!=maid) {
            fcompl = false;
            break;
          }
          else {
            ring_atoms.push_back(aid);
            continue;
          }
        }

        ring_atoms.push_back(*miter);
      }

      if (fcompl)
        drawRingImpl(ring_atoms, pdl);
    }

    // remove drawn ring members from m_atoms
    for (i=0; i<nrings; i++) {
      const ResiToppar::RingAtomArray *pmembs = ptop->getRing(i);
      for (j=0; j<pmembs->size(); j++) {
        LString nm = pmembs->at(j);
        int maid = pres->getAtomID(nm);
        if (maid<=0)
          continue;

        std::set<int>::iterator miter = m_atoms.find(maid);
        if (miter==m_atoms.end())
          continue;

        m_atoms.erase(miter);
      }
    }
  }

}

void BallStickRenderer::drawRingImpl(const std::list<int> atoms, DisplayContext *pdl)
{
  MolCoordPtr pMol = getClientMol();

  double len;
  int i, nsize = atoms.size();
  Vector4D *pvecs = MB_NEW Vector4D[nsize];
  Vector4D cen;
  std::list<int>::const_iterator iter = atoms.begin();
  std::list<int>::const_iterator eiter = atoms.end();
  MolAtomPtr pPivAtom, pAtom;
  for (i=0; iter!=eiter; ++iter, i++) {
    MolAtomPtr pAtom = pMol->getAtom(*iter);
    if (pAtom.isnull()) return;
    MolResiduePtr pres = pAtom->getParentResidue();
    MolChainPtr pch = pAtom->getParentChain();
    MB_DPRINTLN("RING %s %s", pres->toString().c_str(), pAtom->getName().c_str());
    pvecs[i] = pAtom->getPos();
    cen += pvecs[i];
    if (pPivAtom.isnull() && pAtom->getElement()==ElemSym::C)
      pPivAtom = pAtom;
  }

  if (pPivAtom.isnull())
    pPivAtom = pAtom; // no carbon atom --> last atom becomes pivot

  cen = cen.divide(nsize);

  // calculate the normal vector
  Vector4D norm;
  for (i=0; i<nsize; i++) {
    int ni = (i+1)%nsize;
    Vector4D v1 = pvecs[ni] - pvecs[i];
    Vector4D v2 = cen - pvecs[i];
    Vector4D ntmp;
    ntmp = v1.cross(v2);
    len = ntmp.length();
    if (len<=F_EPS8) {
      LOG_DPRINTLN("BallStick> *****");
      return;
    }
    //ntmp.scale(1.0/len);
    ntmp = ntmp.divide(len);
    norm += ntmp;
  }
  len = norm.length();
  norm = norm.divide(len);
  Vector4D dv = norm.scale(m_tickness);

  ColorPtr col = evalMolColor(m_ringcol, ColSchmHolder::getColor(pPivAtom));

  /*
  ColorPtr col = m_ringcol;

  // check molcol reference
  gfx::MolColorRef *pMolCol = dynamic_cast<gfx::MolColorRef *>(col.get());
  if (pMolCol!=NULL) {
    // molcol ref case --> resolve the pivot's color
    col = ColSchmHolder::getColor(pPivAtom);
  }
  */
  
  pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL_NOEGLN);
  pdl->startTriangleFan();
  pdl->normal(norm);
  pdl->color(col);
  pdl->vertex(cen+dv);
  for (i=0; i<=nsize; i++) {
    pdl->vertex(pvecs[i%nsize]+dv);
  }
  pdl->end();

  pdl->startTriangleFan();
  pdl->normal(-norm);
  pdl->color(col);
  pdl->vertex(cen-dv);
  for (i=nsize; i>=0; i--) {
    pdl->vertex(pvecs[i%nsize]-dv);
  }
  pdl->end();
  pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL);
  
  delete [] pvecs;

}

//Renderer *BallStickRenderer::create()
//{
//  return MB_NEW BallStickRenderer();
//}


void BallStickRenderer::propChanged(qlib::LPropEvent &ev)
{
  if (ev.getName().equals("bondw") ||
      ev.getName().equals("sphr") ||
      ev.getName().equals("detail") ||
      ev.getName().equals("ring") ||
      ev.getName().equals("thickness") ||
      ev.getName().equals("ringcolor")) {
    invalidateDisplayCache();
  }
  else if (ev.getParentName().equals("coloring")||
      ev.getParentName().startsWith("coloring.")) {
    invalidateDisplayCache();
  }

  MolAtomRenderer::propChanged(ev);
}

