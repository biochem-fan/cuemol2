// -*-Mode: C++;-*-
//
// Scene (qsc XML file) writer
//

#include <common.h>

#include "SceneXMLWriter.hpp"
#include "StreamManager.hpp"
#include "SceneEvent.hpp"
#include "ObjWriter.hpp"
#include "style/AutoStyleCtxt.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include <qlib/LDOM2Stream.hpp>
#include <qlib/FileStream.hpp>
#include <qlib/StringStream.hpp>
#include <qlib/LByteArray.hpp>

using namespace qsys;
using qlib::LDataSrcContainer;

SceneXMLWriter::SceneXMLWriter()
{
  //m_bForceEmbedAll = true;

  // Default settings
  m_bForceEmbedAll = false;
  m_bBase64 = false;
  m_nCompMode = COMP_NONE;
}

SceneXMLWriter::~SceneXMLWriter()
{
}

int SceneXMLWriter::getCatID() const
{
  return IOH_CAT_SCEWRITER;
}

/// attach to and lock the target object
void SceneXMLWriter::attach(ScenePtr pScene)
{
  // TO DO: lock scene
  m_pClient = pScene;
}
    
/// detach from the target object
ScenePtr SceneXMLWriter::detach()
{
  // TO DO: unlock scene
  ScenePtr p = m_pClient;
  m_pClient = ScenePtr();
  return p;
}

/// Get name of the writer
const char *SceneXMLWriter::getName() const
{
  //return "qsc_xmlwriter";
  return "qsc_xml";
}

/// Get file-type description
const char *SceneXMLWriter::getTypeDescr() const
{
  return "CueMol Scene (*.qsc)";
}

/// Get file extension
const char *SceneXMLWriter::getFileExt() const
{
  return "*.qsc";
}

int SceneXMLWriter::getCompressMode() const
{
  return m_nCompMode;
}

void SceneXMLWriter::setCompressMode(int n)
{
  m_nCompMode = n;
}
  
bool SceneXMLWriter::getBase64Flag() const
{
  return m_bBase64;
}

void SceneXMLWriter::setBase64Flag(bool b)
{
  m_bBase64 = b;
}

void SceneXMLWriter::setDefaultOpts(ScenePtr pScene)
{
  qlib::LDom2Node *pNode = pScene->getQscOpts();
  if (pNode==NULL) return;

  // read default setting
  readFrom2(pNode);
}

void SceneXMLWriter::write()
{
  LString src, srctype;

  // Update the SourcePath/BasePath
  {
    // If getPath() points to a relative path,
    // convert it to the absolute path name using the fs::current_path() (i.e. pwd)
    fs::path curpath = fs::current_path();
#if (BOOST_FILESYSTEM_VERSION==2)
    LString localfile = qlib::makeAbsolutePath(getPath(), curpath.file_string());
#else
    LString localfile = qlib::makeAbsolutePath(getPath(), curpath.string());
#endif

    m_pClient->setSource(localfile);
    m_pClient->setSourceType(getName());
  }
  
  //
  // Setup streams
  //
  qlib::FileOutStream fos;
  fos.open(getPath());

  qlib::LDom2OutStream oos(fos);

  // setup encoding flags
  LString encflag;
  if (m_bBase64) {
    encflag = "1";
  }
  else {
    encflag = "0";
  }

  if (m_nCompMode==COMP_NONE) {
    encflag += "0";
  }
  else if (m_nCompMode==COMP_GZIP) {
    encflag += "1";
  }
  else {
    MB_THROW(qlib::FileFormatException, "Unsupported compression mode");
    return;
  }

  MB_DPRINTLN("QDF Encoding type = %s", encflag.c_str());
  oos.setQdfEncType(encflag);

  //
  // Setup embedding objects
  //

  Scene::ObjIter oiter = m_pClient->beginObj();
  Scene::ObjIter oiter_end = m_pClient->endObj();
  for (; oiter!=oiter_end; ++oiter) {
    ObjectPtr obj = oiter->second;
    bool bEmbed = m_bForceEmbedAll;

    // Determine whether the object should be embedded or not.
    if (!bEmbed) {
      src = obj->getSource();
      srctype = obj->getSourceType();
      if (src.isEmpty() || srctype.isEmpty()) {
        // invalid source info --> embed it
        bEmbed = true;
      }
      else if (src.startsWith("datachunk:")) {
        // already embedded datasource will be anyway embedded.
        bEmbed = true;
      }
      else if (obj->getModifiedFlag()) {
        // modified since the obj loaded from file
        bEmbed = true;
      }
    }

    if (bEmbed) {
      obj->forceEmbed();
    }
  }
  
  // save qsc options
  qlib::LDom2Tree qsctree("qsc_opts");
  
  // Set qsc writer options from this writer
  //   Writer (options) --> LDOM2 tree
  qsctree.serialize(this, false);
  qsctree.dump();
  m_pClient->setQscOpts(qsctree.detach());

  //
  // build LDOM2 tree
  //
  qlib::LDom2Tree tree("scene");
  tree.serialize(m_pClient.get(), false);
  qlib::LDom2Node *pNode = tree.top();

  //
  // process data chunks
  procDataChunks(oos, pNode);

  //
  // write both tree and datachunks
  //
  oos.write(&tree);

  // we don't have to close stream here, because oos is FormatStream
  //oos.close();

  // End of writing
  fos.close();

}

void SceneXMLWriter::procDataChunks(qlib::LDom2OutStream &oos, qlib::LDom2Node *pNode)
{
  //////////
  // Recursively check the data embed requests

  LDom2Node::NodeList::const_iterator iter = pNode->childBegin();
  LDom2Node::NodeList::const_iterator eiter = pNode->childEnd();
  for (; iter!=eiter; ++iter) {
    LDom2Node *pChNode = *iter;
    if (pChNode!=NULL)
      procDataChunks(oos, pChNode);
  }

  //////////
  // setup data chunk for the embed-requesting node

  LDataSrcContainer *pCnt = pNode->getDataSrcContainer();
  if (pCnt==NULL || !pCnt->isDataSrcWritable())
    return;

  LString src = oos.prepareDataChunk(pCnt);
  LString srctype = pCnt->getDataChunkReaderName();
  pCnt->setDataChunkName(src, pNode);

  // obj->setSource(src);
  // obj->setSourceType(srctype);
  MB_DPRINTLN("SceneXMLWr> embeding src=%s, srctype=%s",
              src.c_str(), srctype.c_str());
}

qlib::LScrSp<qlib::LByteArray> SceneXMLWriter::toByteArray(const qlib::LScrSp<qlib::LScrObjBase> &pSObj,
                                                           const LString &type_ovwr)
{
  // Setup streams
  qlib::StrOutStream fos;
  qlib::LDom2OutStream oos(fos);

  // gzip-compressed base64 stream
  // (the output should be text format)
  oos.setQdfEncType("11");
  
  qlib::uid_t nSceneID = qlib::invalid_uid;
  LString top_type;
  ObjectPtr pObj(pSObj, qlib::no_throw_tag());
  RendererPtr pRend;
  CameraPtr pCam;
  bool bEmbed = false;

  if (!pObj.isnull()) {
    // pSObj is Object
    top_type = "object";
    nSceneID = pObj->getSceneID();
    LString src = pObj->getSource();
    LString srctype = pObj->getSourceType();
    if (src.isEmpty() || srctype.isEmpty()) {
      // invalid source info --> embed it
      bEmbed = true;
    }
    else if (src.startsWith("datachunk:")) {
      // already embedded datasource will be anyway embedded.
      bEmbed = true;
    }
    else if (pObj->getModifiedFlag()) {
      // modified since the pObj loaded from file
      bEmbed = true;
    }
    
    if (bEmbed) {
      MB_DPRINTLN("toByteArray> forceEmbedding...");
      pObj->forceEmbed();
    }
  }
  else {
    pRend = RendererPtr(pSObj, qlib::no_throw_tag());
    if (!pRend.isnull()) {
      // pSObj is Renderer
      top_type = "renderer";
      nSceneID = pRend->getSceneID();
    }
    else {

      pCam = CameraPtr(pSObj, qlib::no_throw_tag());
      if (!pCam.isnull()) {
	// pSObj is Camera
	top_type = "camera";
      }
      else {
	MB_THROW(qlib::InvalidCastException, "toXML: obj is not obj or renderer or camera");
	return qlib::LScrSp<qlib::LByteArray>();
      }
    }
  }

  //
  // Build LDOM2 tree
  //
  qlib::LDom2Tree tree(top_type);
  qlib::LDom2Node *pChNode = tree.top();

  if (!pRend.isnull()) {
    // Renderer type="nickname"
    if (type_ovwr.isEmpty())
      pChNode->setTypeName( pRend->getTypeName() );
    else
      pChNode->setTypeName( type_ovwr );
    // always in child element
    pChNode->setAttrFlag(false);
  }
  else if (!pObj.isnull()) {
    // Object type="QIF class name"
    if (type_ovwr.isEmpty())
      pChNode->setTypeNameByObj(pObj.get());
    else
      pChNode->setTypeName( type_ovwr );
    // always in child element
    pChNode->setAttrFlag(false);
  }
  else if (!pCam.isnull()) {
    // pChNode->setTagName("camera");
    // always in attrs
    // pChNode->setAttrFlag(true);
  }
  else {
    MB_ASSERT(false);
    return qlib::LScrSp<qlib::LByteArray>();
  }
  
  // Enter the context
  AutoStyleCtxt style_ctxt(nSceneID);

  pSObj->writeTo2(pChNode);

  // process data chunks
  procDataChunks(oos, pChNode);

  oos.write(&tree);
  oos.close();

  // End of writing
  fos.close();

  qlib::LScrSp<qlib::LByteArray> pRet = fos.getByteArray();

  //if (!pCam.isnull()) {
  MB_DPRINTLN("XML:\n%s<<<", pRet->data());
  MB_DPRINTLN("Length: %d", pRet->size());
  //}
  return pRet;
}

