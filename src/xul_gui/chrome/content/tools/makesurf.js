//
// Molecular surface calculation UI
// $Id$
//

( function () { try {

  ///////////////////////////
  // Initialization
  
  const pref = require("preferences-service");
  const util = require("util");
  
  var dlg = window.gDlgObj = new Object();
  dlg.mTgtSceID = window.arguments[0];
  dd("MolSurfDlg> TargetScene="+dlg.mTgtSceID);
  
  dlg.mObjBox = new cuemolui.ObjMenuList(
    "mol-select-box", window,
    function (elem) {
      return cuemol.implIface(elem.type, "MolCoord");
    },
    cuemol.evtMgr.SEM_OBJECT);
  dlg.mObjBox._tgtSceID = dlg.mTgtSceID;
  
  window.addEventListener("load", function(){
    try {dlg.onLoad();} catch (e) {debug.exception(e);}
  }, false);
  
  dlg.mMolSel = null;

  var default_path = "";

  ///////////////////////////
  // Event Methods

  dlg.onLoad = function ()
  {
    var that = this;
    
    this.mSelBox = document.getElementById('mol-selection');
    this.mSelBox.targetSceID = this.mTgtSceID;
    this.mSurfName = document.getElementById('surf-obj-name');
    this.mSurfName.disabled=false;

    this.mObjBox.addSelChanged(function(aEvent) {
      try { that.onObjBoxChanged(aEvent);}
      catch (e) { debug.exception(e); }
    });

    var sel_chk = document.getElementById("selection-check");
    sel_chk.checked = false;
    this.mSelBox.disabled = true;

    var nobjs = this.mObjBox.getItemCount();
    
    //alert("item count="+nobjs);
    if (nobjs==0) {
      // no mol obj to calc --> error?
      sel_chk.disabled = true;
      this.mSelBox.disabled = true;
      this.mSurfName.disabled = true;
    }
    else {
      var mol = this.mObjBox.getSelectedObj();
      if (mol) {
	this.mSelBox.molID = mol.uid;
	this.mSurfName.value = this.makeSugName(mol.name);
      }

      try {
	if (mol.sel.toString().length>0) {
	  // target is mol and has valid selection --> enable selection option
	  sel_chk.checked = true;
	  this.mSelBox.disabled = false;
	}
      } catch (e) {}
    }
    this.mSelBox.buildBox();
  }
  
  dlg.makeSugName = function (name)
  {
    var newname = "sf_"+name;
    var scene = cuemol.getScene(this.mTgtSceID);
    if (scene==null||scene==undefined)
      return newname;

    if (scene.getObjectByName(newname)!=null) {
      newname = util.makeUniqName2(
	function (a) {return newname+"("+a+")"},
	function (a) {return scene.getObjectByName(a);} );
    }

    return newname;
  }

  dlg.onObjBoxChanged = function (aEvent)
  {
    dd("MolSurfDlg> ObjSelChg: "+aEvent.target.id);
    var mol = this.mObjBox.getSelectedObj();
    if (mol) {
      this.mSelBox.molID = mol.uid;
      this.mSurfName.value = this.makeSugName(mol.name);
    }
  }

  dlg.onSelChk = function (aEvent)
  {
    if (aEvent.target.checked)
      this.mSelBox.disabled = false;
    else
      this.mSelBox.disabled = true;
  }
  
  dlg.onDialogAccept = function (event)
  {
    var tgtmol = this.mObjBox.getSelectedObj();
    if (tgtmol==null)
      return;

    this.buildMolSurf();
  }

  ////////////////

  dlg.buildMolSurf = function ()
  {
    var scene = cuemol.getScene(this.mTgtSceID);

    var strMgr = cuemol.getService("StreamManager");

    var tgtmol = this.mObjBox.getSelectedObj();
    var newname = this.mSurfName.value;

    var rend_name = util.makeUniqName2(
      function (a) {return "molsurf"+a; },
      function (a) {return scene.getRendByName(a);} );

    // setup seleciton
    var molsel = null;
    if (!this.mSelBox.disabled)
      molsel = this.mSelBox.selectedSel;

    if (molsel==null)
      molsel = cuemol.createObj("SelCommand");
    else if (molsel.toString()!=="")
      this.mSelBox.addHistorySel();
    
    ////
    // density value

    var nden = parseInt(document.getElementById("point-density-value").value);
    if (nden==NaN || nden<1)
      nden = 1;

    ////
    // probe radius

    var prad = parseFloat(document.getElementById("probe-radius").value);
    if (prad==NaN || prad<0.1)
      prad = 1.4;

    ////
    // do the real task

    // EDIT TXN START //
    scene.startUndoTxn("Create mol surface");

    try {
      var newobj = cuemol.createObj("MolSurfObj");
      newobj.createSESFromMol(tgtmol, molsel, nden, prad);

      newobj.name = newname;
      scene.addObject(newobj);
      newobj.forceEmbed();

      // create default renderer
      rend = newobj.createRenderer("molsurf");
      rend.name = rend_name;
      
      rend.target = tgtmol.name;
      if (molsel.toString()!=="")
	rend.sel = molsel;
      rend.colormode = "molecule";
      rend.coloring = cuemol.createObj("CPKColoring");
    }
    catch (e) {
      dd("Error: "+e);
      debug.exception(e);
      
      util.alert(window, "Failed to generate molecular surface");
      scene.rollbackUndoTxn();
      return;
    }

    // EDIT TXN END //
    scene.commitUndoTxn();
  }

} catch (e) {debug.exception(e);} } )();

