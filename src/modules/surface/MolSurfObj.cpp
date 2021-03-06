// -*-Mode: C++;-*-
//
// molecular surface object
//
// $Id: MolSurfObj.cpp,v 1.3 2011/04/03 08:08:46 rishitani Exp $

#include <common.h>
#include "MolSurfObj.hpp"
#include "QdfSurfWriter.hpp"

/*
static std::deque<std::pair<Vector3D, LString> > *pdbg;
static void dbgmsg(const Vector3D &v, const LString &str)
{
  pdbg->push_back(std::pair<Vector3D, LString>(v, str));
}
*/

using namespace surface;

MolSurfObj::~MolSurfObj()
{
  clean();
}

/** cleanup all data */
void MolSurfObj::clean()
{
  if (m_pVerts!=NULL) {
    delete [] m_pVerts;
    m_pVerts = NULL;
  }
  m_nVerts = 0;

  if (m_pFaces!=NULL) {
    delete [] m_pFaces;
    m_pFaces = NULL;
  }
  m_nFaces = 0;

}

bool MolSurfObj::isEmpty() const
{
  return m_nVerts==0 && m_nFaces==0;
}
  
void MolSurfObj::deleteSelected()
{
  clean();
}

//////////

LString MolSurfObj::getDataChunkReaderName() const
{
  return LString("qdfsurf");
}

void MolSurfObj::writeDataChunkTo(qlib::LDom2OutStream &oos) const
{
  QdfSurfWriter writer;
  writer.setEncType(oos.getQdfEncType());

  MolSurfObj *pthis = const_cast<MolSurfObj *>(this);
  writer.attach(qsys::ObjectPtr(pthis));
  writer.write(oos);
  writer.detach();
}

