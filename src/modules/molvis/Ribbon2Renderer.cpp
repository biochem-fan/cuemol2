// -*-Mode: C++;-*-
//
//  Ribbon type2 representation test class
//
//  $Id: Ribbon2Renderer.cpp,v 1.2 2010/11/07 13:10:14 rishitani Exp $

#include <common.h>
#include "molvis.hpp"

#include "Ribbon2Renderer.hpp"
#include "TubeSection.hpp"

#include <modules/molstr/MolCoord.hpp>
#include <modules/molstr/MolChain.hpp>
#include <modules/molstr/MolResidue.hpp>

#include <gfx/GradientColor.hpp>

using namespace molvis;
using namespace molvis::detail;
using namespace molstr;

/////////////////////////////

SecSplDat::SecSplDat(Ribbon2Renderer *pP)
     : m_pParent(pP)
{
  m_bBnormSpl = false;
  m_bStartExtend = false;
  m_bEndExtend = false;
  m_nResDelta = 0;
  m_bFixStart = false;
  m_bFixEnd = false;
  m_nStartId = 0;
}

bool SecSplDat::generate()
{
  int nsize = m_posvec.size();

  m_spl.setStartId(m_nStartId);
  m_spl.setSize(nsize);

  for (int i=0; i<nsize; ++i)
    m_spl.setPoint(i+m_nStartId, m_posvec[i]);

  if (!m_spl.generate())
    return false;

  return true;
}

bool SecSplDat::generateHelix(double wrho)
{
  if (!generate())
    return false;

  //////////
  // Generate width interpolator

  int nsize = m_posvec.size();
  m_wspl.setSize(nsize);
  m_wspl.setRho(wrho);
  double wsum = 0.0;
  for (int i=0; i<nsize; ++i) {
    Vector4D orig = m_spl.getPoint(i);
    Vector4D intp;
    m_spl.interpolate(i, &intp);
    double len = (orig-intp).length();
    m_wspl.setValue(i, len);
    wsum += len;
  }

  if (nsize>0)
    m_dWidthAver = wsum/double(nsize);
  else
    m_dWidthAver = 1.0; // XXX

  if (!m_wspl.generate())
    return false;
  
  return true;
}

bool SecSplDat::generateSheet()
{
  m_bBnormSpl = false;
  m_bnorm_ave = Vector4D();
  
  m_spl.setUseWeight(true);
  if (!generate())
    return false;

  //////////
  // Generate binormal vector interpolator

  Vector4D prev_bnorm, bnorm_ave;
  int nsize = m_posvec.size();

  int ist = 1, ien = nsize-1;

  if (m_bStartExtend)
    ist += 1;
  if (m_bEndExtend)
    ien -= 1;

  if (ien-ist+1<=0) {
    ist = 1;
    ien = nsize-1;    
  }

  if ( ist<1 || ien>=(nsize-1) )
    return false;

  m_bnspl.setSize(ien-ist+1);

  int ind=0;
  for (int i=ist; i<=ien; ++i) {
    Vector4D p0 = m_posvec[i-1];
    Vector4D p1 = m_posvec[i];
    Vector4D p2 = m_posvec[i+1];

    // calc binormal vector
    Vector4D bnorm = (p1 - p0).cross(p2 - p1);
  
    // normalization
    double len = bnorm.length();
    if (len>=F_EPS4)
      bnorm = bnorm.scale(1.0/len);
    else
    // singularity case: cannot determine binomal vec.
      bnorm = Vector4D(1.0, 0.0, 0.0);
    
    // Preserve consistency of the direction
    if (!prev_bnorm.isZero()) {
      double costh = bnorm.dot(prev_bnorm);
      if (costh<0)
        bnorm = -bnorm;
    }

    //m_bnspl.setPoint(i-1, p1+bnorm);
    m_bnspl.setPoint(ind, bnorm);
    bnorm_ave += bnorm;
    prev_bnorm = bnorm;
    ++ind;
  }

  m_bnorm_ave = bnorm_ave.normalize();

  if (!m_bnspl.generate())
    return false;

  m_bBnormSpl = true;

  return true;
}

Vector4D SecSplDat::getBnormVec(double t)
{
  Vector4D rval;

  if (!m_bBnormSpl) {
    if (m_bnorm_ave.isZero3D(F_EPS4)) {
      // singularity case: bnorm vec wasn't determined correctly
      return Vector4D(1,0,0);
    }
    else {
      return m_bnorm_ave;
    }
  }

  double par = t-1;
  if (m_bStartExtend)
    par -= 1;
  m_bnspl.interpolate(par, &rval);
  return rval.normalize();

/*
  const double len = rval.length();
  if (qlib::isNear4(len, 0.0)) {
    // ERROR!!
    return Vector4D(1,0,0);
  }
  return rval.divide(len);
*/
}

bool SecSplDat::generateCoil()
{
  const double def_w = 1.0;
  
  int nsize = m_posvec.size();
  m_spl.setSize(nsize);
  m_spl.setUseWeight(true);
  if (m_bFixStart)
    m_spl.setFixStart(m_vStartD1);
  if (m_bFixEnd)
    m_spl.setFixEnd(m_vEndD1);
  
  if (!generate())
    return false;

  return true;
}

//////////////////////////////////////////

Ribbon2Renderer::Ribbon2Renderer()
     : super_t(),
       m_ptsHelix(MB_NEW TubeSection()),
       m_ptsSheet(MB_NEW TubeSection()),
       m_ptsCoil(MB_NEW TubeSection()),
       m_pSheetHead(MB_NEW JctTable())
{

  m_dHelixSmo = 3.0;
  m_dAxExt = 0.5;
  m_dWidthPlus = 0.35;
  m_dWidthRho = 1.0;
  m_nHelixWidthMode = HWIDTH_AVER;
  m_dHelixWidth = 2.3;

  m_dSheetSmo = 2.0;
  m_dSheetWsmo = 5.0;

  m_dCoilSmo = 0.0;

  m_bDumpCurv = false;

  // setup sub properties (call LScrObjBase::setupParentData())
  super_t::setupParentData("helix");
  super_t::setupParentData("sheet");
  super_t::setupParentData("coil");
  super_t::setupParentData("sheethead");
}

Ribbon2Renderer::~Ribbon2Renderer()
{
  clearHelixData();
  clearSheetData();
  clearCoilData();
}

const char *Ribbon2Renderer::getTypeName() const
{
  return "cartoon";
}

void Ribbon2Renderer::beginRend(DisplayContext *pdl)
{
  m_diffvecs.clear();
  //super_t::beginRend(pdl);
}

void Ribbon2Renderer::endRend(DisplayContext *pdl)
{
}

//////////

void Ribbon2Renderer::beginSegment(DisplayContext *pdl, MolResiduePtr pRes)
{
  m_resvec.clear();
}

void Ribbon2Renderer::rendResid(DisplayContext *pdl, MolResiduePtr pRes)
{
  m_resvec.push_back(pRes);
}

void Ribbon2Renderer::endSegment(DisplayContext *pdl, MolResiduePtr pEndRes)
{
  m_indvec.resize( m_resvec.size() );
  buildHelixData();
  buildSheetData();
  buildCoilData();

  m_ptsHelix->setWidth(1.0);
  m_ptsHelix->setupSectionTable();
  renderHelix(pdl);

  m_ptsSheet->setupSectionTable();
  m_pSheetHead->setup(getAxialDetail()+1, m_ptsSheet.get(), m_ptsCoil.get(), true);

  BOOST_FOREACH (SecSplDat *pElem, m_sheets) {
    renderSheet(pdl, pElem);
  }
  
  m_ptsCoil->setupSectionTable();
  renderCoil(pdl);

  if (m_bDumpCurv)
    curvature();

  // update diffvecs table
  updateDiffVecs();

  m_indvec.clear();
  m_resvec.clear();
}

/////////////////////////////////////////////////////////////////
// Helix rendering routines

static inline bool isHelix(const LString &sec)
{
  return sec.startsWith("H") || sec.startsWith("G") || sec.startsWith("I");
}

static inline bool isSheet(const LString &sec)
{
  return sec.startsWith("E");
}

static inline bool isCoil(const LString &sec)
{
  //return !sec.equals("E") &&
  //!sec.equals("H") && !sec.equals("G") && !sec.equals("I");
  return !isSheet(sec) && !isHelix(sec);
}

void Ribbon2Renderer::clearHelixData()
{
  std::for_each(m_cylinders.begin(),
                m_cylinders.end(),
                qlib::delete_ptr<SecSplDat*>());
  m_cylinders.clear();
}

void Ribbon2Renderer::buildHelixData()
{
  clearHelixData();

  int nCylInd = 0;
  int nresvec = m_resvec.size();
  SecSplDat *pCyl = NULL;

  for (int i=0; i<nresvec; ++i) {
    MolResiduePtr pRes = m_resvec[i];
    MolResiduePtr pPrevRes, pNextRes;
    if (i>=1) pPrevRes = m_resvec[i-1];
    if (i<=nresvec-2) pNextRes = m_resvec[i+1];
    LString sec, pfx;
    pRes->getPropStr("secondary2", sec);
    if (sec.length()>=2)
      pfx= sec.substr(1,1);

    if ( !(isHelix(sec)) )
      continue;

    MB_DPRINTLN("BuildHelix> %s %s", pRes->getStrIndex().c_str(), sec.c_str());
    m_indvec[i] = nCylInd;

    // Default: along the way of helix
    int nsw = 0;

    if (pfx.equals("s")) {
      // start of helix
      nsw = 1;
    }
    else if (pfx.equals("e")) {
      if (pCyl==NULL) {
        // singular helix (truncated case)
        // --> ignore
        continue;
      }
      // end of helix
      nsw = 2;
    }
    else if (pCyl==NULL)
      nsw = 1; // Truncated helix (by selection?)

    if (nsw==1) {
      // start
      MB_ASSERT(pCyl==NULL);
      pCyl = MB_NEW SecSplDat(this);
      pCyl->m_spl.setRho(m_dHelixSmo);
      pCyl->m_nResDelta = i;
      if (!pPrevRes.isnull()) {
        pCyl->m_bStartExtend = true;
        pCyl->addPoint( getPivotPos(pPrevRes) );
        pCyl->m_nResDelta = i-1;
      }
      pCyl->addPoint( getPivotPos(pRes) );
    }
    else if (nsw==2) {
      // end
      MB_ASSERT(pCyl!=NULL);
      pCyl->addPoint( getPivotPos(pRes) );
      if (!pNextRes.isnull()) {
        pCyl->m_bEndExtend = true;
        pCyl->addPoint( getPivotPos(pNextRes) );
      }
      bool res = pCyl->generateHelix(m_dWidthRho);
      m_cylinders.push_back(pCyl);
      pCyl = NULL;
      ++nCylInd;
    }
    else {
      if (pCyl!=NULL)
        pCyl->addPoint( getPivotPos(pRes) );
    }

  } // for

  if (pCyl!=NULL) {
    // truncated helix (by selection?)
    pCyl->generateHelix(m_dWidthRho);
    m_cylinders.push_back(pCyl);
  }

}

gfx::ColorPtr Ribbon2Renderer::calcColor(double at, SecSplDat *pCyl)
{
  double t = at;
  const double tend = pCyl->m_spl.getPoints()-1.0;

  if (pCyl->m_bStartExtend && at<1.0)
    t = 1.0;
  else if (pCyl->m_bEndExtend && at>tend-1.0)
    t = tend-1.0;

  int nprev = int(::floor(t));
  int nnext = int(::ceil(t));
  double rho = t - double(nprev);

  nprev += pCyl->m_nResDelta;
  nnext += pCyl->m_nResDelta;

  MolResiduePtr pPrev, pNext;
  if (0<=nprev && nprev<m_resvec.size())
    pPrev= m_resvec[nprev];
  if (0<=nnext && nnext<m_resvec.size())
    pNext= m_resvec[nnext];

  gfx::ColorPtr pCol1, pCol2;
  
  return super_t::calcColor(rho, isSmoothColor(), pPrev, pNext);
}

void Ribbon2Renderer::renderHelix(DisplayContext *pdl)
{
  // std::deque<Vector4D> vtmp;

  const int naxdet = getAxialDetail();
  int ncyls = m_cylinders.size();
  for (int i=0; i<ncyls; ++i) {
    SecSplDat *pC = m_cylinders[i];

    double tstart=0.0;
    if (pC->m_bStartExtend)
      tstart = 1.0-m_dAxExt;

    double tend = pC->m_spl.getPoints()-1.0;
    if (pC->m_bEndExtend)
      tend -= 1.0-m_dAxExt;

    int ndelta = (int) ::floor( (tend-tstart)* naxdet );
    if (ndelta<=0) {
      // degenerated (single point)
      // TO DO: impl
      continue;
    }
    const double fdelta = (tend-tstart)/double(ndelta);

    Vector4D p0 = pC->m_spl.getPoint( pC->m_bStartExtend?1:0 );
    Vector4D e11, e12;
    
    Vector4D f1, f2, vpt;
    Vector4D bnorm_base;

    // Color objects used in the axial-loop
    ColorPtr pCol;

    double width, dwidth;

    m_ptsHelix->startTess();
    
    // axial step loop
    for (int j=0; j<=ndelta; ++j) {
      double t = tstart + double(j)*fdelta; ///double(naxdet);
      pCol = calcColor(t, pC);
      pC->m_spl.interpolate(t, &f1, &vpt);
      // if (!m_bWidthAver) {
      if (m_nHelixWidthMode==HWIDTH_WAVY) {
        pC->m_wspl.interpolate(t, &width, &dwidth);
      }
      else if (m_nHelixWidthMode==HWIDTH_AVER) {
        width = pC->m_dWidthAver;
        dwidth = 0.0;
      }
      else {
        // HWIDTH_CONST
        width = m_dHelixWidth;
        dwidth = 0.0;
      }
      width += m_dWidthPlus;
      if (j==0) {
        bnorm_base = f1-p0;
      }
      
      e11 = bnorm_base.cross(vpt).normalize().scale(width);
      e12 = ( vpt.cross(e11) ).normalize().scale(width);

      
      if (j==0) {
        // make the tube cap.
        pdl->color(pCol);
        m_ptsHelix->makeCap(pdl, true, getStartCapType(), f1, vpt, e11, e12);
      }
      
      m_ptsHelix->doTess(pdl, f1, pCol, isSmoothColor(), e11, e12, vpt.scale(-dwidth));

      if (j==ndelta) {
        // make cap at the end point.
        pdl->color(pCol);
        m_ptsHelix->makeCap(pdl, false, getEndCapType(), f1, vpt, e11, e12);
      }

      bnorm_base = e12;
      
      //vtmp.push_back(f1);
      //vtmp.push_back(f1+e11.scale(2.0));
      //vtmp.push_back(f1);
      //vtmp.push_back(f1+e12.scale(2.0));
    }

    m_ptsHelix->endTess();
  }

/*
  pdl->startLines();
  BOOST_FOREACH (const Vector4D &elem, vtmp) {
    pdl->vertex(elem);
  }
  pdl->end();
 */
}

////////////////////////////////////////////////////////////////////////////////
// Sheet routines

double Ribbon2Renderer::getAnchorWgt(MolResiduePtr pRes) const
{
  if (m_pAnchorSel.isnull()||m_pAnchorSel->toString().isEmpty())
    return 1.0;
  MolAtomPtr pAtom = getPivotAtom(pRes);
  if (m_pAnchorSel->isSelected(pAtom))
    return m_dAnchorWgt;
  return 1.0;
}

void Ribbon2Renderer::clearSheetData()
{
  std::for_each(m_sheets.begin(),
                m_sheets.end(),
                qlib::delete_ptr<SecSplDat*>());
  m_sheets.clear();
}

void Ribbon2Renderer::buildSheetData()
{
  clearSheetData();

  int nCylInd = 0;
  int nresvec = m_resvec.size();
  LString prev_ss;
  SecSplDat *pSh = NULL;
  MolResiduePtr pPrevRes;

  for (int i=0; i<nresvec; ++i) {
    MolResiduePtr pRes = m_resvec[i];

    LString sec;
    pRes->getPropStr("secondary2", sec);
    //MB_DPRINTLN("Cart.buildSh> %s %s", pRes->getStrIndex().c_str(), sec.c_str());

    if (!isSheet(sec)) {
      if (isSheet(prev_ss)) {
        // end of sheet (E x)
        MB_ASSERT(pSh!=NULL);
        pSh->addPoint( getPivotPos(pRes), getAnchorWgt(pRes) );
        pSh->m_bEndExtend = true;

        bool res = pSh->generateSheet();
        m_sheets.push_back(pSh);
        pSh = NULL;
        ++nCylInd;
      }
    }
    else {

      // set sheet index
      m_indvec[i] = nCylInd;

      if (!isSheet(prev_ss)) {
        // start of sheet (x E) or (. E)

        MB_ASSERT(pSh==NULL);
        pSh = MB_NEW SecSplDat(this);
        pSh->m_spl.setRho(m_dSheetSmo);
        pSh->m_bnspl.setRho(m_dSheetWsmo);
        pSh->m_nResDelta = i;
        if (!pPrevRes.isnull()) {
          pSh->m_bStartExtend = true;
          pSh->addPoint( getPivotPos(pPrevRes), getAnchorWgt(pRes) );
          pSh->m_nResDelta = i-1;
        }
        pSh->addPoint( getPivotPos(pRes), getAnchorWgt(pRes) );
      }
      else {
        // mid of sheet (E E)
        if (pSh!=NULL)
          pSh->addPoint( getPivotPos(pRes), getAnchorWgt(pRes) );
      }
    }

    prev_ss = sec;
    pPrevRes = pRes;
  } // for

  if (pSh!=NULL) {
    // the last residue is sheet (E E .)
    // --> build the last sheet
    bool res = pSh->generateSheet();
    m_sheets.push_back(pSh);
    pSh = NULL;
  }

}

void Ribbon2Renderer::renderSheet(DisplayContext *pdl, detail::SecSplDat *pC)
{
  Vector4D zerovec(0,0,0);

  const int naxdet = getAxialDetail();
  double tstart=0.0;
  const double ext = 0.5;
  if (pC->m_bStartExtend)
    tstart = 1.0-ext;
  
  double tend = pC->m_spl.getPoints()-1.0;
  if (pC->m_bEndExtend)
    tend -= 1.0-ext;
  
  int ndelta = (int) ::floor( ((tend-1.0)-tstart)* naxdet );
  if (ndelta<=0) {
    // degenerated (single point)
    // TO DO: impl
    //continue;
    return;
  }
  const double fdelta = ((tend-1.0)-tstart)/double(ndelta);
  
  Vector4D e11, e12, bntmp;
  Vector4D f1, f2, vpt;
  
  // Color objects used in the axial-loop
  ColorPtr pCol;
  
  //MB_DPRINTLN("Sheet %d (t: %f->%f)", i, tstart, tend);
  
  // Draw sheet body

  pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL_NOEGLN);
  //pdl->setPolygonMode(gfx::DisplayContext::POLY_LINE);
  m_ptsSheet->startTess();
  
  // axial step loop
  for (int j=0; j<=ndelta; ++j) {
    const double t = tstart + double(j)*fdelta; // /double(naxdet);
    pCol = calcColor(t, pC);
    pC->m_spl.interpolate(t, &f1, &vpt);
    vpt = vpt.normalize();
    
    bntmp = pC->getBnormVec(t);
    if (qlib::isNear4(bntmp.length(), 0.0)) {
      MB_DPRINTLN("bntmp zero error!!");
    }
    e11 = bntmp.cross(vpt).normalize();
    e12 = vpt.cross(e11);
    
    if (j==0) {
      // make the tube cap.
      pdl->color(pCol);
      m_ptsSheet->makeCap(pdl, true, getStartCapType(), f1, vpt, e11, e12);
    }
    
    m_ptsSheet->doTess(pdl, f1, pCol, isSmoothColor(), e11, e12, zerovec );
    
    if (j==ndelta) {
      // update the tstart for head drawing
      tstart = t;
    }
  }
  
  m_ptsSheet->endTess();
  
  /////////////////////////////////////////////

  // Draw arrow head
  ndelta = m_pSheetHead->getSize();

  Vector4D escl, prev_escl, xe11, xe12;
  double dpar;
  double prev_dpar = -1.0;

  // initialize previous E scale
  m_pSheetHead->get(0, dpar, prev_escl);

  pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL_NOEGLN);
  m_ptsSheet->startTess();

  for (int i=0; i<ndelta; i++) {
    // Get E-scale value
    m_pSheetHead->get(i, dpar, escl);
    const double t = tstart + dpar;

    pCol = calcColor(t, pC);
    pC->m_spl.interpolate(t, &f1, &vpt);
    const Vector4D ev = vpt.normalize();
    
    bntmp = pC->getBnormVec(t);
    e11 = bntmp.cross(ev).normalize();
    e12 = ev.cross(e11);

    xe11 = e11.scale(escl.x());
    xe12 = e12.scale(escl.y());

    if ( qlib::isNear4(dpar,prev_dpar) ) {
      // uncontinuous point of arrow head
      // make partition polygons
      m_ptsSheet->endTess();

      pdl->color(pCol);
      m_ptsSheet->makeDisconJct(pdl, f1, ev, e11, e12, prev_escl, escl);
      pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL_NOEGLN);
      m_ptsSheet->startTess();
    }
    
    m_ptsSheet->doTess(pdl, f1, pCol, isSmoothColor(), xe11, xe12,
                       escl, vpt);

    ////////////////

    prev_escl = escl;
    prev_dpar = dpar;
  }

  m_ptsSheet->endTess();
  pdl->setPolygonMode(gfx::DisplayContext::POLY_FILL);

  // make cap at the end point.
  pdl->color(pCol);
  m_ptsSheet->makeCap(pdl, false, getEndCapType(), f1, vpt, xe11, xe12);

}

////////////////////////////////////////////////////////////////////////////////
// Coil routines

gfx::ColorPtr Ribbon2Renderer::calcColor2(double at, SecSplDat *pCyl)
{
  double t = at;
  double tstart = 0.0;
  double tend = pCyl->m_nRealSize;

  if (pCyl->m_bStartExtend)
    tstart += 1.0;
  if (pCyl->m_bEndExtend)
    tend -= 1.0;

  // truncate between tstart ~ tend-1.0
  t = qlib::trunc(t, tstart, tend-1.0);
  
  int nprev = int(::floor(t));
  int nnext = nprev+1; //int(::ceil(t));
  const double rho = t - double(nprev);

  nprev += pCyl->m_nResDelta;
  nnext += pCyl->m_nResDelta;

  MolResiduePtr pPrev, pNext;
  if (0<=nprev && nprev<m_resvec.size())
    pPrev= m_resvec[nprev];
  if (0<=nnext && nnext<m_resvec.size())
    pNext= m_resvec[nnext];

  gfx::ColorPtr pCol1, pCol2;
  
  return super_t::calcColor(rho, isSmoothColor(), pPrev, pNext);
}

void Ribbon2Renderer::clearCoilData()
{
  std::for_each(m_coils.begin(),
                m_coils.end(),
                qlib::delete_ptr<SecSplDat*>());
  m_coils.clear();
}

void Ribbon2Renderer::extendSheetCoil(detail::SecSplDat *pCoil, int nPrevInd)
{
  int isheet = m_indvec[nPrevInd];
  SecSplDat *pPrevSheet = m_sheets[isheet];
  double tend = pPrevSheet->m_spl.getPoints()-1.0;
  //if (pPrevSheet->m_bEndExtend)
  //tend -= 1.0; //-m_dAxExt;

  Vector4D f1, vpt;

  pPrevSheet->m_spl.interpolate(tend-1.5, &f1, &vpt);
  pCoil->addPoint( f1, m_dAnchorWgt );

  pCoil->setStart();

  pPrevSheet->m_spl.interpolate(tend-0.5, &f1, &vpt);
  pCoil->addPoint( f1, m_dAnchorWgt );

  //pCoil->m_bFixStart = true;
  pCoil->m_bFixStart = false;
  pCoil->m_vStartD1 = vpt;
}

void Ribbon2Renderer::extendCoilSheet(detail::SecSplDat *pCoil, int nNextInd)
{
  int isheet = m_indvec[nNextInd];
  SecSplDat *pNextSheet = m_sheets[isheet];
  double tstart = 0.0;
  Vector4D f1, vpt;

  pNextSheet->m_spl.interpolate(0.5, &f1, &vpt);
  pCoil->addPoint( f1, m_dAnchorWgt );

  pCoil->setEnd();

  pNextSheet->m_spl.interpolate(1.5, &f1, &vpt);
  pCoil->addPoint( f1, m_dAnchorWgt );

  //pCoil->m_bFixEnd = true;
  pCoil->m_bFixEnd = false;
  pCoil->m_vEndD1 = vpt;
}

void Ribbon2Renderer::buildCoilData()
{
  clearCoilData();

  int nresvec = m_resvec.size();
  LString prev_ss = "H"; // dummy for starting coil

  SecSplDat *pCoil = NULL;
  MolResiduePtr pPrevRes;
  for (int i=0; i<nresvec; ++i) {
    MolResiduePtr pRes = m_resvec[i];

    LString sec;
    pRes->getPropStr("secondary2", sec);
    sec = sec.substr(0,1);

    if ( isCoil(sec) ) {
      if ( !isCoil(prev_ss) ) {
        // Start of coil (x C) or (. C)
        MB_ASSERT(pCoil==NULL);
        pCoil = MB_NEW SecSplDat(this);
        pCoil->m_spl.setRho(m_dCoilSmo);
        pCoil->m_nResDelta = i;
        if (pPrevRes.isnull()) {
          // No previous residue (may be N-terminus; i.e. [. C])
          //  --> no extension
          pCoil->setStart(); // nstart should be zero...
          pCoil->addPoint( getPivotPos(pRes) , getAnchorWgt(pRes));
        }
        else {
          if (isSheet(prev_ss)) {
            extendSheetCoil(pCoil, i-1);
            pCoil->addPoint( getPivotPos(pRes), 1.0);
          }
          else {
            pCoil->setStart();
            pCoil->addPoint( getPivotPos(pPrevRes), m_dAnchorWgt);
            pCoil->addPoint( getPivotPos(pRes), m_dAnchorWgt);
          }
          pCoil->m_bStartExtend = true;
          pCoil->m_nResDelta = i-1;
        }
      }
      else {
        // mid of coil (C C)
        if (pCoil!=NULL) {
          const double wgt = getAnchorWgt(pRes);
          pCoil->addPoint( getPivotPos(pRes) , wgt);
        }
      }
    }
    else {
      // non-coil
      if ( isCoil(prev_ss) ) {
        // end of coil (C x)
        MB_ASSERT(pCoil!=NULL);
        if (isSheet(sec)) {
          // coil-sheet junction (C E)
          extendCoilSheet(pCoil, i);
        }
        else {
          // coil-helix junction (C H)
          pCoil->m_posvec.back().w() = m_dAnchorWgt;
          pCoil->addPoint( getPivotPos(pRes) , m_dAnchorWgt);
          pCoil->setEnd();
        }
        pCoil->m_bEndExtend = true;
        bool res = pCoil->generateCoil();
        m_coils.push_back(pCoil);
        pCoil = NULL;
      }
    }

    prev_ss = sec;
    pPrevRes = pRes;
  } // for

  if (pCoil!=NULL) {
    // the last residue is coil (C C .)
    // --> build the last coil
    if (pCoil->m_posvec.size()>0)
      pCoil->m_posvec.back().w() = m_dAnchorWgt;
    pCoil->setEnd();
    bool res = pCoil->generateCoil();
    m_coils.push_back(pCoil);
    pCoil = NULL;
  }

}

void Ribbon2Renderer::renderCoil(DisplayContext *pdl)
{
  Vector4D zerovec(0,0,0);

  const int naxdet = getAxialDetail();
  int ncyls = m_coils.size();
  for (int i=0; i<ncyls; ++i) {
    SecSplDat *pC = m_coils[i];

    double tstart = 0.0;
    //double tend = pC->m_spl.getPoints()-1.0;
    double tend = pC->m_nRealSize -1.0;

    int ndelta = (int) ::floor( (tend-tstart)* naxdet );
    if (ndelta<=0) {
      // degenerated (single point)
      // TO DO: impl
      continue;
    }
    const double fdelta = (tend-tstart)/double(ndelta);

    Vector4D bntmp;
    Vector4D e11, e12, e21, e22;
    Vector4D f1, f2, vpt;

    // Color objects used in the axial-loop
    ColorPtr pCol;

    //MB_DPRINTLN("Sheet %d (t: %f->%f)", i, tstart, tend);

    m_ptsCoil->startTess();
    //pdl->startLineStrip();
    
    // axial step loop
    for (int j=0; j<=ndelta; ++j) {
      double t = tstart + double(j) * fdelta; // /double(naxdet);
      //double col_t = t;
      pCol = calcColor2(t, pC);
      pC->m_spl.interpolate(t, &f1, &vpt);
      //vpt = vpt.normalize();

      if (j==0) {
        bntmp = Vector4D(1,0,0);
      }

      Vector4D e11 = bntmp.cross(vpt).normalize();
      Vector4D e12 = vpt.cross(e11).normalize();

      if (j==0) {
        // make the tube cap.
        pdl->color(pCol);
        m_ptsCoil->makeCap(pdl, true, getStartCapType(), f1, vpt, e11, e12);
      }

      m_ptsCoil->doTess(pdl, f1, pCol, isSmoothColor(), e11, e12, zerovec );
      //pdl->color(pCol);
      //pdl->vertex(f1);

      if (j==ndelta) {
        // make cap at the end point.
        pdl->color(pCol);
        m_ptsCoil->makeCap(pdl, false, getEndCapType(), f1, vpt, e11, e12);
      }

      bntmp = e12;
    }
    
    m_ptsCoil->endTess();
    //pdl->end();
  }
  
}


//////////////////////////////////////////////////

//virtual
void Ribbon2Renderer::setAxialDetail(int nlev)
{
  m_nAxialDetail = nlev;
  invalidateSplineCoeffs();
}

void Ribbon2Renderer::propChanged(qlib::LPropEvent &ev)
{
  if (ev.getParentName().equals("coloring")||
      ev.getParentName().startsWith("coloring.")) {
    invalidateDisplayCache();
  }
  else if (ev.getName().startsWith("helix") ||
           ev.getName().startsWith("sheet") ||
           ev.getName().startsWith("coil")) {
    invalidateSplineCoeffs();
    invalidateDisplayCache();
  }
  else if (ev.getParentName().startsWith("helix") ||
           ev.getParentName().startsWith("sheet") ||
           ev.getParentName().startsWith("coil")) {
    invalidateSplineCoeffs();
    invalidateDisplayCache();
  }
  
  super_t::propChanged(ev);
}

void Ribbon2Renderer::objectChanged(qsys::ObjectEvent &ev)
{
  if (ev.getType()==qsys::ObjectEvent::OBE_CHANGED) {
    invalidateSplineCoeffs();
    invalidateDisplayCache();
    return;
  }
  
  super_t::objectChanged(ev);
}

void Ribbon2Renderer::invalidateSplineCoeffs()
{
  // m_scs.cleanup();
  invalidateDisplayCache();
  m_diffvecs.clear();
}

void Ribbon2Renderer::dumpCyls(SecSplDat *pC)
{
  Vector4D f1, vpt, cuv;

  double tstart=0.0;
  //if (pC->m_bStartExtend)
  //tstart = 1.0-m_dAxExt;
  
  double tend = pC->m_spl.getPoints()-1.0;
  //if (pC->m_bEndExtend)
  //tend -= 1.0-m_dAxExt;
  
  int ndelta = (int) ::floor( (tend-tstart)* 1 );
  if (ndelta<=0) {
    MB_DPRINTLN("Degen helix");
    return;
  }
  const double fdelta = (tend-tstart)/double(ndelta);
  
  for (int j=0; j<=ndelta; ++j) {
    double t = tstart + double(j)*fdelta;
    pC->m_spl.interpolate(t, &f1, &vpt, &cuv);

    int nres = int(::floor(t));
    nres += pC->m_nResDelta;

    MolResiduePtr pRes;
    if (0<=nres && nres<m_resvec.size())
      pRes= m_resvec[nres];
    if (!pRes.isnull())
      LOG_DPRINT("%s ", pRes->toString().c_str());
    
    LOG_DPRINTLN("nres=%d t=%f cv=%f", nres, t, cuv.length());
  }
}

void Ribbon2Renderer::curvature()
{
  MB_DPRINTLN("Rib2Rend> curvature() called");

  try {
    int ncyls = m_cylinders.size();
    LOG_DPRINTLN("=== cyls : %d ===", ncyls);
    for (int i=0; i<ncyls; ++i) {
      SecSplDat *pC = m_cylinders[i];
      LOG_DPRINTLN("elem %d", i);
      dumpCyls(pC);
      LOG_DPRINTLN("elem %d END", i);
    }
    
    ncyls = m_sheets.size();
    LOG_DPRINTLN("=== sheets : %d ===", ncyls);
    for (int i=0; i<ncyls; ++i) {
      SecSplDat *pC = m_sheets[i];
      LOG_DPRINTLN("elem %d", i);
      dumpCyls(pC);
      LOG_DPRINTLN("elem %d END", i);
    }

    /*
    ncyls = m_coils.size();
    MB_DPRINTLN("=== coils : %d ===", ncyls);
    for (int i=0; i<ncyls; ++i) {
      SecSplDat *pC = m_coils[i];
      dumpCyls(pC);
    }
    */
  }
  catch (...) {
    MB_DPRINTLN("*** exception occured");
    throw;
  }
}

void Ribbon2Renderer::updateDiffVecsImpl(SecSplDat *pC)
{
  Vector4D f1, vpt, cuv;

  double tstart=0.0;
  
  double tend = pC->m_spl.getPoints()-1.0;
  
  int ndelta = (int) ::floor( (tend-tstart)* 1 );
  if (ndelta<=0) {
    MB_DPRINTLN("Degenerated");
    return;
  }
  const double fdelta = (tend-tstart)/double(ndelta);
  
  for (int j=0; j<=ndelta; ++j) {
    double t = tstart + double(j)*fdelta;
    pC->m_spl.interpolate(t, &f1, &vpt, &cuv);

    int nres = int(::floor(t));
    nres += pC->m_nResDelta;

    MolResiduePtr pRes;
    if (!(0<=nres && nres<m_resvec.size()))
      continue;
    
    pRes= m_resvec[nres];
    if (pRes.isnull())
      continue;

    MB_DPRINTLN("DiffVec %f %d (%s)", t, nres, pRes->toString().c_str());
    m_diffvecs.insert(DiffVecMap::value_type(pRes.get(), DiffVecMap::mapped_type(f1, vpt)));
  }
}

void Ribbon2Renderer::updateDiffVecs()
{
  int nsize;
  
  nsize = m_coils.size();
  for (int i=0; i<nsize; ++i) {
    SecSplDat *pC = m_coils[i];
    updateDiffVecsImpl(pC);
  }

  nsize = m_cylinders.size();
  for (int i=0; i<nsize; ++i) {
    SecSplDat *pC = m_cylinders[i];
    updateDiffVecsImpl(pC);
  }
  
  nsize = m_sheets.size();
  for (int i=0; i<nsize; ++i) {
    SecSplDat *pC = m_sheets[i];
    updateDiffVecsImpl(pC);
  }

}

bool Ribbon2Renderer::getDiffVec(MolResiduePtr pRes, Vector4D &rpos, Vector4D &rvec)
{
  if (m_diffvecs.empty()) {
    return false;
  }

  DiffVecMap::const_iterator iter = m_diffvecs.find(pRes.get());
  if (iter==m_diffvecs.end())
    return false;

  rpos = iter->second.first;
  rvec = iter->second.second;
  return true;
}

