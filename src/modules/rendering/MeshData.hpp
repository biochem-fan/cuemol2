// -*-Mode: C++;-*-
//
//  File display context implementation class
//

#ifndef REND_MESH_DATA_HPP_INCLUDED
#define REND_MESH_DATA_HPP_INCLUDED

#include "render.hpp"

#include <gfx/DisplayContext.hpp>
#include <gfx/ColorTable.hpp>
#include <qlib/LString.hpp>
#include <qlib/LStream.hpp>

namespace qlib {
  class PrintStream;
}

namespace render {

  using qlib::LString;
  using qlib::Vector4D;
  using gfx::ColorTable;
  using gfx::ColorPtr;

  ///////////////////////////////////////////////////

  struct MeshVert
  {
    Vector4D v, n;

    ColorTable::elem_t c;

    MeshVert()
    {
    }

    MeshVert(const Vector4D &av, const Vector4D &an, const ColorTable::elem_t &ac)
         : v(av), n(an), c(ac)
    {
    }
  };

  ///////////////////////////////////////////////////

  /// mode of the face (for silhouette line rendering)
  //  0: nomal mode
  //  1: ridge triangle without silhouette line (e.g. at cylinder termini)
  //  2: generated by conv_sphere
  //  3: generated by conv_cyl (with termini)
  enum {
    MFMOD_MESH = 0,
    MFMOD_OPNCYL = 1,
    MFMOD_SPHERE = 2,
    MFMOD_CLSCYL = 3
  };

  struct MeshFace
  {
    /// index of verteces
    int iv1, iv2, iv3;

    /// mode of the face (for silhouette line rendering)
    int nmode;

    /// ctor
    MeshFace(int ai1, int ai2, int ai3, int anmode)
         : iv1(ai1), iv2(ai2), iv3(ai3), nmode(anmode)
    {
    }
  };

  ///////////////////////////////////////////////////

  struct Mesh
  {
    bool m_bLighting;

    // RendIntData *m_pPar;

    typedef std::deque<MeshFace> MeshFaceSet;
    typedef MeshFaceSet::iterator FIter;
    typedef MeshFaceSet::const_iterator FCIter;
    MeshFaceSet m_faces;

    typedef std::deque<MeshVert*> MeshVertSet;
    typedef MeshVertSet::const_iterator VCIter;
    MeshVertSet m_verts;

    ~Mesh()
    {
      clear();
    }

    int getVertexSize() const { return m_verts.size(); }
    int getFaceSize() const { return m_faces.size(); }

    int addVertex(const Vector4D &v, const Vector4D &n, const ColorTable::elem_t &c)
    {
      int rval = m_verts.size();
      m_verts.push_back(MB_NEW MeshVert(v,n,c));
      return rval;
    }

    int addVertex(const Vector4D &v, const Vector4D &n, const ColorTable::elem_t &c, const Matrix4D &xfm)
    {
      Vector4D xv = v;
      xv.w() = 1.0;
      xfm.xform4D(xv);

      Vector4D xn = n;
      xn.w() = 0.0;
      xfm.xform4D(xn);

      return addVertex(xv, xn, c);
    }

    /*int addVertex(const Vector4D &v, const Vector4D &n, const ColorPtr &c)
    {
      return addVertex(v, n, m_pPar->convCol(c));
    }*/

    int addVertex(MeshVert *p)
    {
      int rval = m_verts.size();
      m_verts.push_back(p);
      return rval;
    }

    int copyVertex(MeshVert *pOrig)
    {
      return addVertex(pOrig->v, pOrig->n, pOrig->c);
    }

    void addFace(int iv0, int iv1, int iv2, int nmode = 0)
    {
      m_faces.push_back(MeshFace(iv0, iv1, iv2, nmode));
    }

    void clear()
    {
      std::for_each(m_verts.begin(), m_verts.end(), qlib::delete_ptr<MeshVert *>());
      m_verts.clear();
      m_faces.clear();
    }
  };

}

#endif
