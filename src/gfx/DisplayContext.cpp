// -*-Mode: C++;-*-
//
//  Abstract display context class
//
// $Id: DisplayContext.cpp,v 1.7 2011/02/11 06:56:57 rishitani Exp $

#include <common.h>

#include "DisplayContext.hpp"
#include "SolidColor.hpp"

using namespace gfx;

qsys::View *DisplayContext::getTargetView() const
{
  return NULL;
}

void DisplayContext::vertex(double x, double y, double z)
{
  vertex(Vector4D(x,y,z));
}

void DisplayContext::normal(double x, double y, double z)
{
  normal(Vector4D(x,y,z));
}

void DisplayContext::rotate(const qlib::LQuat &q)
{
  multMatrix(q.toRotMatrix());
}

void DisplayContext::color(double r, double g, double b, double a)
{
  color(SolidColor::createRGB(r,g,b,a));
}

void DisplayContext::color(double r, double g, double b)
{
  color(SolidColor::createRGB(r,g,b));
}

void DisplayContext::setMaterial(const LString &name)
{
  m_defMatName = name;
}

void DisplayContext::setAlpha(double a)
{
  m_defAlpha = a;
}

void DisplayContext::setStyleNames(const LString &name)
{
  m_styleNames = name;
}

void DisplayContext::setEdgeLineType(int n)
{
}

void DisplayContext::setEdgeLineWidth(double w)
{
}

void DisplayContext::setEdgeLineColor(const ColorPtr &c)
{
}

void DisplayContext::scale(const Vector4D &v)
{
  Matrix4D m;
  m.aij(1,1) = v.x();
  m.aij(2,2) = v.y();
  m.aij(3,3) = v.z();
  m.aij(4,4) = 1.0;
  multMatrix(m);
}

void DisplayContext::translate(const Vector4D &v)
{
  Matrix4D m = Matrix4D::makeTransMat(v);
//  m.aij(4,4) = 1.0;
  multMatrix(m);
}

void DisplayContext::loadIdent()
{
  Matrix4D m;
  loadMatrix(m);
}

void DisplayContext::drawString(const Vector4D &pos,
                                const qlib::LString &str)
{
}

void DisplayContext::drawPixels(const Vector4D &pos,
                                const PixelBuffer &data,
                                const ColorPtr &col)
{
}

/// Display unit sphere
void DisplayContext::sphere()
{
}

/// Display sphere with radius of r at position vec
void DisplayContext::sphere(double r, const Vector4D &vec)
{
  pushMatrix();
  translate(vec);
  scale(Vector4D(r,r,r));
  sphere();
  popMatrix();
}

void DisplayContext::setDetail(int n)
{
}

int DisplayContext::getDetail() const
{
  return 0;
}

void DisplayContext::cylinder(double r, const Vector4D &pos1, const Vector4D &pos2)
{
  cone(r, r, pos1, pos2, false);
}

void DisplayContext::cylinderCap(double r, const Vector4D &pos1, const Vector4D &pos2)
{
  cone(r, r, pos1, pos2, true);
}

void DisplayContext::cone(double r1, double r2,
                          const Vector4D &pos1, const Vector4D &pos2,bool bCap)
{
  return;
}

void DisplayContext::drawMesh(const Mesh &)
{
}

void DisplayContext::drawElem(const DrawElem &)
{
}

void DisplayContext::setLineWidth(double lw)
{
}

void DisplayContext::setLineStipple(unsigned short pattern)
{
}

void DisplayContext::setLighting(bool f)
{
}

void DisplayContext::setPointSize(double size)
{
}


//////////////////////////////////////////////////////////////////
// Display list (null implementation)

DisplayContext *DisplayContext::createDisplayList()
{
  return NULL;
}

bool DisplayContext::canCreateDL() const
{
  return false;
}

void DisplayContext::callDisplayList(DisplayContext *pdl)
{
}

bool DisplayContext::isCompatibleDL(DisplayContext *pdl) const
{
  return false;
}

bool DisplayContext::isDisplayList() const
{
  return false;
}

bool DisplayContext::recordStart()
{
  return false;
}

void DisplayContext::recordEnd()
{
}

//////////////////////////////////////////////////////////////////

void DisplayContext::startHit(qlib::uid_t rend_uid) {}
void DisplayContext::endHit() {}

void DisplayContext::loadName(int nameid) {}
void DisplayContext::pushName(int nameid) {}
void DisplayContext::popName() {}
void DisplayContext::drawPointHit(int nid, const Vector4D &pos) {}

void DisplayContext::startRender() {}
void DisplayContext::endRender() {}
void DisplayContext::startSection(const LString &section_name) {}
void DisplayContext::endSection() {}

