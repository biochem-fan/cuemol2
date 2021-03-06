// -*-Mode: javascript;-*-
//
// Povray rendering tool window
//
// $Id: render-pov-dlg.js,v 1.20 2011/04/29 14:52:08 rishitani Exp $
//

( function () {

  const povrender = require("povrender");
  const util = require("util");
  const timer = require("timer");
  const pref = require("preferences-service");
  
  const pov_width_key = "cuemol2.ui.render.pov-img-width";
  const pov_height_key = "cuemol2.ui.render.pov-img-height";
  const pov_dpi_key = "cuemol2.ui.render.pov-img-dpi";
  const pov_unit_key = "cuemol2.ui.render.pov-img-unit";

  var dlg = window.gDlgObj = new Object();
  dlg.mTgtSceID = window.arguments[0];
  dlg.mTgtVwID = window.arguments[1];
  
  // Save scene name here
  {
    let scene = cuemol.getScene(dlg.mTgtSceID);
    //var ini_name = scene.name;
    dlg.mSceName = scene.name;
    delete scene;
  }

  dlg.mPovRender = povrender.newPovRender();

  dlg.mPovRender.mTimerFn = function(arg){
    try {dlg.onTimer(arg);} catch (e) {debug.exception(e);}
  };
  
  dlg.mZoomPc = 100;
  dlg.mSerial=0;
  
  window.addEventListener("load", function(){
    try {dlg.onLoad();} catch (e) {debug.exception(e);}
  }, false);
  window.addEventListener("unload", function(){
    try {dlg.onUnload();} catch (e) {debug.exception(e);}
  }, false);
  
  //////////

  dlg.onLoad = function ()
  {
    var that = this;
    var elem;
    
    this.mPovExePathBox = document.getElementById("povray-exe-path");
    this.mPovIncPathBox = document.getElementById("povray-inc-path");
    
    this.mOutImgWidth = document.getElementById("output-image-width");
    this.mOutImgHeight = document.getElementById("output-image-height");
    this.mOutImgDPI = document.getElementById("output-image-dpi");
    this.mOutImgUnit = document.getElementById("output-image-unit");

    {
      // setup default values
      let view = cuemol.getView(dlg.mTgtVwID);
      this.mOutImgHeight.value = view.height;
      this.mOutImgWidth.value = view.width;
      // DPI==600
      // UNIT==px
      this.setupImgUnit("px");
    }

    if (pref.has(pov_dpi_key)) {
	let val = parseInt( pref.get(pov_dpi_key) );
	if (!isNaN(val))
	    this.mOutImgDPI.inputField.value = val;
    }
    if (pref.has(pov_unit_key)) {
      let value = pref.get(pov_unit_key);
      this.setupImgUnit(value);
      util.selectMenuListByValue(this.mOutImgUnit, value);
    }
    if (pref.has(pov_width_key)) {
	let val = parseFloat( pref.get(pov_width_key) );
	if (!isNaN(val))
	    this.mOutImgWidth.value = val;
    }
    if (pref.has(pov_height_key)) {
	let val = parseFloat( pref.get(pov_height_key) );
	if (!isNaN(val))
	    this.mOutImgHeight.value = val;
    }

    this.mImage = document.getElementById("image-box");

    this.mSaveImgBtn = document.documentElement.getButton("extra1");
    //getElementById("save-image");
    this.mSaveImgBtn.disabled = true;
    this.mCopyImgBtn = document.documentElement.getButton("extra2");
    this.mCopyImgBtn.disabled = true;

    //this.mStartStopBtn = document.getElementById("render-start-stop");
    this.mStartStopBtn = document.documentElement.getButton("accept");
    this.mCloseBtn = document.documentElement.getButton("cancel");
    
    this.mDisableTgt = document.getElementsByClassName("disable-target");
    
    this.mPovExePathBox.value = this.mPovRender.mPovExePath;
    this.mPovIncPathBox.value = this.mPovRender.mPovIncPath;

    elem = document.getElementById("ZoomBtn");
    elem.addEventListener(
      "command", function (a) { that.onZoomPreview(a); } , false);
    
    document.getElementById("UnzoomBtn").addEventListener(
      "command", function (a) { that.onZoomPreview(a); } , false);
    
    this.mZoomMenuList = document.getElementById("ZoomList");
    this.mZoomMenuList.addEventListener(
      "command", function (a) { that.onZoomPreview(a); } , false);
  };
  
  dlg.onUnload = function ()
  {
    util.persistChkBox("enable-clip-plane", document);
    util.persistChkBox("enable-post-blend", document);
    util.persistChkBox("enable-shadow", document);
    util.persistChkBox("enable-edgelines", document);

    // close OK ==> remove tmp&img files
    this.mPovRender.clearTmpFiles();
    this.mPovRender.clearImgFile();
    dd("PovRender: ***** tmp files removed");

    // save preferences
    pref.set(pov_width_key, this.mOutImgWidth.value);
    pref.set(pov_height_key, this.mOutImgHeight.value);
    var val = parseInt(this.mOutImgDPI.value);
    if (!isNaN(val)) {
      pref.set(pov_dpi_key, this.mOutImgDPI.value);
    }
    pref.set(pov_unit_key, this.mOutImgUnit.value);

    dd("PovRender: ***** prefs saved");
  };

  dlg.disableButtons = function (aFlag)
  {
    dd("Disable target = "+this.mDisableTgt);
    var tgt = Array.prototype.slice.call(this.mDisableTgt, 0);
    
    if (aFlag) {
      tgt.forEach( function (elem, ind, ary) {
        elem.setAttribute("disabled", true);
      });
      this.mCloseBtn.disabled = true;
      this.mSaveImgBtn.disabled = true;
      this.mCopyImgBtn.disabled = true;
      this.mStartStopBtn.setAttribute("label", "Stop");
    }
    else {
      tgt.forEach( function (elem) {
        elem.removeAttribute("disabled");
      });
      this.mCloseBtn.disabled = false;

      if (this.mPovRender._bTmpImageAvail) {
	this.mSaveImgBtn.disabled = false;
	this.mCopyImgBtn.disabled = false;
      }
      else {
        this.mSaveImgBtn.disabled = true;
	this.mCopyImgBtn.disabled = true;
      }
      this.mStartStopBtn.setAttribute("label", "Start");
    }
    
  };

  dlg.setupImgUnit = function (unit)
  {
    if (unit=="px") {
      this.mOutImgHeight.decimalPlaces = 0;
      this.mOutImgWidth.decimalPlaces = 0;
    }
    else {
      this.mOutImgHeight.decimalPlaces = 3;
      this.mOutImgWidth.decimalPlaces = 3;
    }
      
    this.mPrevImgUnit = unit;
  };

  // convert image size (in unit) to pixel size
  dlg.convImgSizeUnit = function (value, dpi, unit)
  {
    // in pixel --> no conversion
    if (unit=="px")
      return value;

    if (unit=="in")
      return value * dpi;

    if (unit=="mm") {
      // conv mm --> inch (1 in = 25.4 mm)
      value /= 25.4;
      return value * dpi;
    }

    if (unit=="cm") {
      // conv cm --> inch (1 in = 2.54 cm)
      value /= 2.54;
      return value * dpi;
    }

    return value;
  };

  dlg.onStartStopRender = function ()
  {
    this.mPovRender.setPovExePath(this.mPovExePathBox.value);
    this.mPovRender.setPovIncPath(this.mPovIncPathBox.value);

    if (this.mPovRender._bRender) {
      this.mPovRender.stopRender();
      this.disableButtons(false);
      return;
    }

    var that = this;
    var elem;

    this.disableButtons(true);

    // setup DPI
    var dpi = parseInt(this.mOutImgDPI.value);
    if (!isNaN(dpi)) {
      this.mPovRender.mDPI = dpi;
      // pref.set(pov_dpi_key, this.mOutImgDPI.value);
    }
    else {
      dd("Invalid DPI value: "+this.mOutImgDPI.value);
      dpi = 72;
    }

    // setup image size unit
    var unit = this.mOutImgUnit.value;

    this.mPovRender.img_height = Math.round( this.convImgSizeUnit(this.mOutImgHeight.value, dpi, unit) );
    this.mPovRender.img_width = Math.round( this.convImgSizeUnit(this.mOutImgWidth.value, dpi, unit) );
    // alert("value:"+this.mOutImgHeight.value+", DPI="+dpi+", UNIT="+unit+"--> "+this.mPovRender.img_height);

    var stereoElem = document.getElementById("stereo-mode-list");
    var steDep = document.getElementById("stereo-depth");
    let steDepVal = parseFloat(steDep.value);
    if (isNaN(steDepVal) || steDepVal<0 || steDepVal>1)
      steDepVal = 0.03; // default value
    if (stereoElem.selectedItem.value=="right") {
      this.mPovRender.nStereo = 1;
      this.mPovRender.dSteDep = steDepVal;
    }
    else if (stereoElem.selectedItem.value=="left") {
      this.mPovRender.nStereo = -1;
      this.mPovRender.dSteDep = steDepVal;
    }
    else
      this.mPovRender.nStereo = 0;

    elem = document.getElementById("proj-mode-list");
    this.mPovRender.bOrtho = (elem.selectedItem.value=="ortho");

    elem = document.getElementById("enable-clip-plane");
    this.mPovRender.mbClip = elem.checked;

    elem = document.getElementById("enable-post-blend");
    this.mPovRender.mbPostBlend = elem.checked;

    elem = document.getElementById("enable-shadow");
    this.mPovRender.mbShadow = elem.checked;

    elem = document.getElementById("enable-edgelines");
    this.mPovRender.mbShowEdgeLines = elem.checked;
    // alert("edge lines: "+this.mPovRender.mbShowEdgeLines);

    setTimeout( function () {
      try {
        that.mPovRender.makePovFiles(dlg.mTgtSceID, dlg.mTgtVwID);
        that.mPovRender.startRender();
      }
      catch (e) {
        debug.exception(e);
        that.mPovRender.stopRender();
        that.disableButtons(false);
        util.alert(window, "Rendering failed:\n"+e);
        return;
      }
    }, 100);
    return;
  };

  dlg.onPovExePath = function ()
  {
    const nsIFilePicker = Ci.nsIFilePicker;
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    
    fp.init(window, "Select POV-Ray executable file", nsIFilePicker.modeOpen);
    
    if (this.mPovRender.mPlfName=="Windows_NT") {
      fp.appendFilters(nsIFilePicker.filterApps);
    }
    else {
      fp.appendFilters(nsIFilePicker.filterAll);
    }
    
    var res = fp.show();
    if (res!=nsIFilePicker.returnOK) {
      return;
    }
    
    this.mPovRender.setPovExePath(fp.file.path);
    this.mPovExePathBox.value = this.mPovRender.mPovExePath;
  };
  
  dlg.onPovIncPath = function ()
  {
    const nsIFilePicker = Ci.nsIFilePicker;
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    
    fp.init(window, "Select POV-Ray inc folder", nsIFilePicker.modeGetFolder);

    var res = fp.show();
    if (res!=nsIFilePicker.returnOK) {
      return;
    }
    
    this.mPovRender.setPovIncPath(fp.file.path);
    this.mPovIncPathBox.value = this.mPovRender.mPovIncPath;
  };

  dlg.onCloseClicked = function ()
  {
    return dlg.onCloseEvent(null);
  };

  dlg.onCloseEvent = function (evt)
  {
    this.mPovRender.stopRender();
    //this.disableButtons(false);
    
    if (this.mPovRender._bTmpImageAvail &&
        !this.mPovRender._bTmpImageSaved) {
      // show query dialog
      var result = util.confirmYesNoCancel(window, "Rendered image is not saved. Save image?");

      if (result==0) {
        // Yes -> save changes and close
        if (!this.onSaveImage()) {
          // save scene (as) is canceled --> cancel closing
          return false;
        }
      }
      else if (result==1) {
        // Cancel -> cancel closing
        return false;
      }
      else {
        // No -> close immediately
      }
    }

    return true;
  };
  
  dlg.onSaveImage = function ()
  {
    if (!this.mPovRender._bTmpImageAvail)
      return;
    
    const nsIFilePicker = Ci.nsIFilePicker;
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    
    fp.appendFilter("PNG (*.png)", "*.png");
    
    // make initial dir and filename
    //var scene = cuemol.getScene(this.mTgtSceID);
    var ini_name = util.removeFileExt( this.mSceName ); //scene.name;
    
    var stereoElem = document.getElementById("stereo-mode-list");
    if (stereoElem.selectedItem.value=="right")
      ini_name = ini_name + "_r";
    else if (stereoElem.selectedItem.value=="left")
      ini_name = ini_name + "_l";
    
    ini_name = ini_name + ".png";
    fp.defaultString = ini_name;
    fp.defaultExtension = "png";
    
    fp.init(window, "Save image", nsIFilePicker.modeSave);
    var res = fp.show();
    if (res==nsIFilePicker.returnCancel)
      return false;
    
    this.mPovRender.saveImage(fp.file);
    return true;
  };

  dlg.onCopyImage = function ()
  {
    if (!this.mPovRender._bTmpImageAvail)
      return;
    
    try {
      var imgfile = this.mPovRender.getCurrentImgFile();
      
      let clipboard = require("qsc-copipe");
      clipboard.set(imgfile, "imagefilepng");
    }
    catch (e) {
      debug.exception(e);
      return;
    }
  };

  dlg.onZoomPreview = function (aEvent)
  {
    var tgt = aEvent.target.id;
    var sel = this.mZoomMenuList.selectedItem;

    dd("onZoomPreview tagid = "+tgt);

    switch (tgt) {
    case "ZoomBtn":
      var next = sel.nextElementSibling;
      if (next)
        this.mZoomMenuList.selectedItem = next;
      else
        return;
      break;

    case "UnzoomBtn":
      var next = sel.previousElementSibling;
      if (next)
        this.mZoomMenuList.selectedItem = next;
      else
        return;
      break;

    }

    sel = this.mZoomMenuList.selectedItem;
    var value = parseInt(sel.value);
    if (value>0 && value<=1000)
      this.mZoomPc = value;

    dd("onZoomPreview zoom="+this.mZoomPc);

    this.updateImagePreview();
  };

  dlg.onTimer = function (bEnd)
  {
    dd("PovDlg.onTimer> called bEnd="+bEnd);
    this.updateImagePreview();

    if (!bEnd)
      return;

    dd("PovDlg.onTimer> Timer END");
    this.disableButtons(false);

    // Now the new temporary image file is available.
    this.mSaveImgBtn.disabled = false;
    this.mCopyImgBtn.disabled = false;
  };

  dlg.updateImagePreview = function ()
  {
    var imgfile = this.mPovRender.getCurrentImgFile();
    if (imgfile==null) {
      dd("ERROR: img file==null");
      return;
    }
    try {
      if (!imgfile.isFile()) {
        throw "not a file";
      }
      //else if (this.mImgFile.fileSize<=0) {
      //throw "zero size file";
      //}
    }
    catch (e) {
      dd("Cannot open file: "+imgfile);
      debug.exception(e);
      this.mImage.removeAttribute("src");
      return;
    }

    /*
  var fileStream = Cc['@mozilla.org/network/file-input-stream;1']
    .createInstance(Ci.nsIFileInputStream);
  fileStream.init(imgfile, 1, 0, false);
  var binaryStream = Cc['@mozilla.org/binaryinputstream;1']
    .createInstance(Ci.nsIBinaryInputStream);
  binaryStream.setInputStream(fileStream);
  var bytes = binaryStream.readBytes(fileStream.available());
  binaryStream.close();
  fileStream.close();
  this.mImage.setAttribute("src", "data:image/png;base64,"+ btoa(bytes));
     */

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var URL = ios.newFileURI(imgfile);
    dd("URL.spec="+URL.spec);
    this.mImage.setAttribute("src", URL.spec+"?dummy="+this.mSerial);
    ++this.mSerial;

    //var height = this.mOutImgHeight.value * this.mZoomPc/100.0;
    //var width = height; //this.mOutImgWidth.value * this.mZoomPc/100.0;
    var height = this.mPovRender.img_height* this.mZoomPc/100.0;
    var width = this.mPovRender.img_width* this.mZoomPc/100.0;

    dd("image w="+width);
    dd("image h="+height);

    this.mImage.setAttribute("width", width);
    this.mImage.setAttribute("height", height);

    document.getElementById("imagebox-item").setAttribute("width", width);
    document.getElementById("imagebox-item").setAttribute("height", height);

    //binaryStream=null;
    //fileStream=null;
  };

  dlg.onPresetSel = function (aEvent)
  {
    try {
      var value = aEvent.target.value;
      dd("onPresetSel: "+value);
      if (value=="view") {
	if (this.mOutImgHeight && this.mOutImgWidth && this.mOutImgDPI) {
	  let view = cuemol.getView(dlg.mTgtVwID);
	  this.mOutImgHeight.value = view.height;
	  this.mOutImgWidth.value = view.width;
	  this.mOutImgDPI.inputField.value = 72;
	  this.setupImgUnit("px");
	  util.selectMenuListByValue(this.mOutImgUnit, "px");
	}
	return;
      }
      
      var ls = value.split(",");
      var w = ls[0];
      var h = ls[1];
      var dpi = ls[2];
      
      dd("w="+w);
      dd("h="+h);
      dd("dpi="+dpi);
      
      if (this.mOutImgHeight && this.mOutImgWidth && this.mOutImgDPI) {
	this.mOutImgWidth.value = w;
	this.mOutImgHeight.value = h;
	this.mOutImgDPI.inputField.value = dpi;
	this.setupImgUnit("px");
	util.selectMenuListByValue(this.mOutImgUnit, "px");
      }
    }
    catch (e) { debug.exception(e); }
  };

  dlg.convPixToUnit = function (pixval, dpi, unit)
  {
    if (unit=="in") {
      // pixel to inch
      return pixval/dpi;
    }
    else if (unit=="mm") {
      let inch = pixval/dpi;
      return inch*25.4;
    }
    else if (unit=="cm") {
      let inch = pixval/dpi;
      return inch*2.54;
    }
    else {
      // px
      return Math.round(pixval);
    }
  };

  dlg.onImgSzUnitSel = function (aEvent)
  {
    if (!this.mOutImgUnit)
      return; // not initialized

    let newval = this.mOutImgUnit.value;
    // let newval = aEvent.target.value;
    let oldval = this.mPrevImgUnit;
    if (newval==oldval)
      return;

    let dpi = parseInt(this.mOutImgDPI.value);
    if (isNaN(dpi))
      return;
    
    // alert(newval);
    let wpx = this.convImgSizeUnit(this.mOutImgWidth.value, dpi, oldval);
    let hpx = this.convImgSizeUnit(this.mOutImgHeight.value, dpi, oldval);

    dd("ChgImgUnit: pix size="+wpx+", "+hpx);

    let w = this.convPixToUnit(wpx, dpi, newval);
    let h = this.convPixToUnit(hpx, dpi, newval);

    dd("ChgImgUnit: newsize="+w+", "+h+" "+newval);

    this.setupImgUnit(newval);

    this.mOutImgWidth.value = w;
    this.mOutImgHeight.value = h;

  };

} )();

