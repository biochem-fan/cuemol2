// -*-Mode: C++;-*-
//
// Generate/Render a mesh contour of ScalarObject
//
// $Id: MapMeshRenderer.hpp,v 1.7 2011/01/06 14:45:24 rishitani Exp $

#ifndef XTAL_MAP_MESH_RENDERER_HPP_INCLUDED
#define XTAL_MAP_MESH_RENDERER_HPP_INCLUDED

#include "xtal.hpp"
#include "MapRenderer.hpp"

#include <qlib/ByteMap.hpp>
#include <qsys/ScalarObject.hpp>
#include <qsys/ViewEvent.hpp>
#include <modules/molstr/molstr.hpp>
#include <modules/molstr/BSPTree.hpp>

class MapMeshRenderer_wrap;

namespace xtal {

using gfx::DisplayContext;
using qsys::ScalarObject;
using molstr::SelectionPtr;
using molstr::MolCoordPtr;
using molstr::BSPTree;
class DensityMap;

class MapMeshRenderer : public MapRenderer,
                        public qsys::ViewEventListener
{
  MC_SCRIPTABLE;
  MC_CLONEABLE;

  typedef MapRenderer super_t;
  friend class ::MapMeshRenderer_wrap;

  ///////////////////////////////////////////
  // properties

  /// Drawing line width (in pixel unit)
  double m_lw;

  /// Internal buffer size (default: 100x100x100 points)
  int m_nBufSize;

  /// Periodic boundary flag
  /// (default: false; set true, if map contains the entire of unit cell)
  bool m_bPBC;

  /// Automatically update the map center as view center
  /// (default: true)
  bool m_bAutoUpdate;

  /// Automatically update the map center as view center
  /// in both mouse-drag and mouse-up events
  /// (default: false)
  bool m_bDragUpdate;

private:

  ///////////////////////////////////////////
  // work area
  
  /// size of map (copy from m_pMap)
  int m_nMapColNo, m_nMapRowNo, m_nMapSecNo;

  /// size of section array
  int m_nColCrs, m_nRowCrs, m_nSecCrs;

  int m_nActCol, m_nActRow, m_nActSec;
  int m_nStCol, m_nStRow, m_nStSec;

  /// section array for x(column) direction
  qlib::ByteMap *m_pXCrsLst;

  /// section array for y direction
  qlib::ByteMap *m_pYCrsLst;

  /// section array for z direction
  qlib::ByteMap *m_pZCrsLst;

  /// delta
  double m_delta;

  // /// cached client obj ptr
  // ScalarObject *m_pMap;

public:

  ///////////////////////////////////////////
  // constructors / destructor

  /// default constructor
  MapMeshRenderer();

  /// destructor
  virtual ~MapMeshRenderer();

  ///////////////////////////////////////////

  virtual const char *getTypeName() const;

  //virtual void attachObj(qlib::uid_t obj_uid);
  virtual void setSceneID(qlib::uid_t nid);

  virtual qlib::uid_t detachObj();

  ///////////////////////////////////////////

  virtual void render(DisplayContext *pdl);
  virtual void preRender(DisplayContext *pdc);
  virtual void postRender(DisplayContext *pdc) {}

  virtual bool isTransp() const { return true; }

  ///////////////////////////////////////////////////////////////

  /// Generate contour level lines
  bool generate(ScalarObject *pMap, DensityMap *pXtal);

  /// Set internal buffer size
  bool setCrossArraySize(int ncol, int nrow, int nsec);
  /// Get internal buffer size (in col direction)
  int getColCrsSize() const { return m_nColCrs; }
  int getRowCrsSize() const { return m_nRowCrs; }
  int getSecCrsSize() const { return m_nSecCrs; }

  ///////////////////////////////////////////////////////////////

  void setLineWidth(double f) {
    m_lw = f;
    super_t::invalidateDisplayCache();
  }
  double getLineWidth() const { return m_lw; }

  double getMaxExtent() const;

  int getBufSize() const { return m_nBufSize; }
  void setBufSize(int nsize);

  ///////////////////////////////////////////////////////////////

  /*
  // display manager listener
  // (required for capturing the view-changed event and updating the map center)
  virtual void attachDisplayMgr(DisplayMgr *pMgr);
  virtual void detachDisplayMgr();
  virtual void dispMgrChanged(DispMgrEvent &ev);
    */
  
  virtual void viewChanged(qsys::ViewEvent &);

protected:
  // We must override firePropertyChanged() to avoid destructing the display list,
  // when only the color was changed.
  // virtual void firePropertyChanged(qlib::PropChgEvent &ev);

  
  ///////////////////////////////////////////////////////////////

private:
  
  unsigned char getContSec(unsigned int s0,
			   unsigned int s1,
			   unsigned int lv);

  /*
  double fmax(double a, double b) {
    if (a>b) return a; else return b;
  }

  double fmin(double a, double b) {
    if (a>b) return b; else return a;
  }
*/
  
  unsigned char getMap(ScalarObject *pMap, int x, int y, int z) const
  {
    /*
    if (m_bUseMolBndry) {
      if (!inMolBndry(pMap, x,y,z))
        return 0;
    }
     */
    if (m_bPBC) {
      const int xx = (x+10000*m_nMapColNo)%m_nMapColNo;
      const int yy = (y+10000*m_nMapRowNo)%m_nMapRowNo;
      const int zz = (z+10000*m_nMapSecNo)%m_nMapSecNo;
      return pMap->atByte(xx,yy,zz);
    }
    else {
      if (pMap->isInBoundary(x,y,z))
        return pMap->atByte(x,y,z);
      else
        return 0;
    }
  }

  ///////////////////////////////////////////////////////////////

public:
  LString getBndryMolName() const { return m_strBndryMol; }
  void setBndryMolName(const LString &s);

  SelectionPtr getBndrySel() const { return m_pSelBndry; }
  void setBndrySel(const SelectionPtr &pSel);

  double getBndryRng() const { return m_dBndryRng; }
  void setBndryRng(double d);

private:

  /// Boundary target mol name
  LString m_strBndryMol;

  /// Selection for mol boundary
  SelectionPtr m_pSelBndry;

  bool m_bUseMolBndry;
  
  BSPTree<int> m_boundary;

  double m_dBndryRng;

  void setupMolBndry();

  bool inMolBndry(ScalarObject *pMap, int nx, int ny, int nz) const
  {
    if (!m_bUseMolBndry)
      return true;
    Vector4D tv(nx, ny, nz);
    tv = pMap->convToOrth(tv);
    if (!m_boundary.collChk(tv, m_dBndryRng))
      return false;
    return true;
  }
  

  
};

}

#endif
