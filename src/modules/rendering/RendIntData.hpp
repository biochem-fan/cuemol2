// -*-Mode: C++;-*-
//
//  File display context implementation class
//

#ifndef REND_INTERNAL_DATA_HPP_INCLUDED
#define REND_INTERNAL_DATA_HPP_INCLUDED

#include "render.hpp"

#include <gfx/DisplayContext.hpp>
#include <gfx/ColorTable.hpp>
#include <qlib/LString.hpp>
#include <qlib/LStream.hpp>

#include "MeshData.hpp"

namespace qlib {
  class PrintStream;
}

namespace render {

  using qlib::LString;
  using qlib::Vector4D;
  using qlib::OutStream;
  using gfx::ColorTable;
  using gfx::ColorPtr;
  using qlib::PrintStream;

  class FileDisplayContext;

  /// Internal data structure for FileDisplayContext
  class RENDER_API RendIntData
  {
    //private:
  public:

    typedef ColorTable::elem_t ColIndex;

    /// output pov file
    OutStream *m_pPovOut;

    /// output inc file
    OutStream *m_pIncOut;

    /// target name
    LString m_name;

    /// Output texture type
    bool m_fUseTexBlend;

    /// Color table
    ColorTable m_clut;

    /// Clipping plane (negative value: no clipping)
    double m_dClipZ;

    /// style name list of Renderer used for this rendering
    LString m_styleNames;

    /// Parent display context
    FileDisplayContext *m_pdc;

    //////////

    /// Line object
    struct Line
    {
      /// Line verteces
      Vector4D v1, v2;

      /// Line color
      ColIndex col;

      /// Line width in pixel unit
      double w;
    };

    std::deque<Line *> m_lines;

    //////////

    /// Cylinder object
    struct Cyl
    {
      /// location of termini
      Vector4D v1, v2;

      /// color
      ColIndex col;

      /// width of termini
      double w1, w2;

      /// terminal cap flag
      bool bcap;

      /// detail level for tesselation
      int ndetail;

      /// transformation matrix
      Matrix4D *pTransf;

      /// ctor
      Cyl() : w1(1.0), w2(1.0), bcap(false), ndetail(1), pTransf(NULL) {}

      ///dtor
      ~Cyl() {
        if (pTransf!=NULL) delete pTransf;
      }
    };

    typedef std::deque<Cyl *> CylList;

    CylList m_cylinders;

    //////////

    /// Sphere object
    struct Sph {
      Vector4D v1;
      ColIndex col;
      double r;
      int ndetail;
    };

    typedef std::deque<Sph *> SphList;

    /// sphere data
    SphList m_spheres;

    /// point data
    SphList m_dots;

    //////////

    /// Mesh object
    Mesh m_mesh;

    /// Mesh pivot
    int m_nMeshPivot;

  public:
    RendIntData(FileDisplayContext *pdc);
    virtual ~RendIntData();

    void start(OutStream *fp, OutStream *ifp, const char *name);
    void end();

    /// Append line segment
    void line(const Vector4D &v1, const Vector4D &v2, double w, const ColorPtr &col = ColorPtr());

    /// Append point
    void dot(const Vector4D &v1, double w, const ColorPtr &col = ColorPtr());

    /// Append cylinder
    void cylinder(const Vector4D &v1, const Vector4D &v2,
                  double w1, double w2, bool bcap,
                  int ndet, const Matrix4D *ptrf,
                  const ColorPtr &col = ColorPtr());

    /// Append sphere
    void sphere(const Vector4D &v1, double w, int ndet, const ColorPtr &col = ColorPtr());

    //////////
    // Mesh drawing operations

    void meshStart();
    void meshEndTrigs();
    void meshEndTrigStrip();
    void meshEndFan();

    /// Mesh generation for trigs & trigstrip
    void meshVertex(const Vector4D &v1, const Vector4D &n1, const ColorPtr &col)
    {
      m_mesh.addVertex(v1, n1, convCol(col));
    }

    // / mesh generation for trigfan
    // void mesh_fan(const Vector4D &v1, const Vector4D &n1, const LColor &c1, bool bMakeTri);

    void mesh(const Matrix4D &mat, const gfx::Mesh &mesh);

    /// Convert color to internal representation
    ColIndex convCol();

    /// Convert color to internal representation (2)
    ColIndex convCol(const ColorPtr &col);

    /// convert line to cylinder
    void convLines();

    /// convert dot to sphere
    void convDots();

    /// convert spheres to mesh
    void convSpheres();

    /// convert cylinders to mesh
    void convCylinders();

    void eraseLines() {
      m_lines.erase(m_lines.begin(), m_lines.end());
    }

    void eraseCyls() {
      m_cylinders.erase(m_cylinders.begin(), m_cylinders.end());
    }
    
    void eraseSpheres() {
      m_spheres.erase(m_spheres.begin(), m_spheres.end());
    }
    
  private:
    void convSphere(Sph *);
    void convCyl(Cyl *);
    int selectTrig(int j, int k, int j1, int k1);

    /////////////////////////////////////////////////

  private:
    MeshVert *cutEdge(MeshVert *pv1, MeshVert *pv2);

  public:
    /// Mesh clipping operation
    Mesh *calcMeshClip();

  private:
    static bool isVertNear(const MeshVert &p1, const MeshVert &p2, int nmode);
    
  public:
    /// Mesh simplification
    // nmode==0: vertex compare
    // nmode==1: vertex&norm compare
    // nmode==2: vertex&norm&color compare (default)
    Mesh *simplifyMesh(Mesh *pMesh, int nmode=2);
    

    const LString &getStyleNames() const {
      return m_styleNames;
    }

    inline bool isEmpty() const {
      if (m_cylinders.size()<=0 &&
          m_dots.size()<=0 &&
          m_spheres.size()<=0 &&
          m_mesh.getVertexSize()<=0 &&
          m_mesh.getFaceSize()<=0 &&
          m_lines.size()<=0 &&
          m_clut.size()<=0)
        return true;
      else
        return false;
    }

  };

}

#endif

