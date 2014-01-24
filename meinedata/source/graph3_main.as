// code to pick up and parse the full url
var _urlStr:String = _url.substr(this._url.lastIndexOf("?") + 1);
_urls =_urlStr.split("&");
var _xmlFile:String = getParam("xml");
//var _pointSize:Number = 2;
var _pointSize:Number =  (getParam("pointSize") == ""?2:Number(getParam("pointSize")));

//var _pointFill:Boolean = true;
var _customList:Boolean = true;
var _opacity = 0;
var _playDelay = 0;
//var _noCache:Boolean = false;



//if (getParam("pointSize") != ""){
	//_pointSize = Number(getParam("pointSize"));
//}
var _pointFill:Boolean  = (getParam("pointFill") =="false"?false:true);
var _noCache:Boolean  = (getParam("noCache") == "true"?true:false);
var _doGet:Boolean  = (getParam("doGet") == "true"?true:false);

// global data

// Useful strings

_unselOpacityString = "Unselected Opacity - ";
_clearButtonString = "Show all";
_sequenceOptionString = "Sequence";
_defaultColourString = "Default Colours";
_sequenceModeString = "Sequence Mode";


// variable data read from XML
var _variableData = new Object();

// Point data read from XML
var _pointData = new Object();

// list of all points.  Could probably
// be got rid of - all info is in _pointData
var _itemList:Array  = new Array();

// The actual point objects
var _pointObjs = new Object();

// do the menus
// popUpListener - change the data types
var popUpListener:Object = new Object();
//var mpListener:Object = new Object();
var xSequenceCheckListener:Object = new Object();
var mouseListener:Object = new Object();
Mouse.addListener(mouseListener);

// popUpListener - change the data types
var seqListener:Object = new Object();
// drop down lists for vertual/log
var colourListener:Object = new Object();
var xTypeListener:Object = new Object();
var yTypeListener:Object = new Object();
var _sequenceFormat:TextFormat = new TextFormat();
_sequenceFormat.bold = true;
_sequenceFormat.size = 40;
_sequenceFormat.font = "axisFont";


// I want the list to always be above anything else

// set up some defaults
var _menuList = new Array(); 
if (_xmlFile == ""){
	_noCache = false;
	trace("doGet");
// for testing purposes 
	
	_xmlFile = "c:\\netprojs\\flash\\graph\\sjm8.xml";
	//_xmlFile = "http://localhost/FlashDice/FlashDice.aspx";
	//_xmlFile = "c:\\netprojs\\flash\\graph\\uncd1.xml";
	//_xmlFile = "c:\\netprojs\\flash\\graph\\vs.xml";
	//_xmlFile = "c:\\netprojs\\flash\\graph\\iquango.xml";
	//_xmlFile = "c:\\netprojs\\flash\\graph\\co2.xml";
}
if (_pointSize == undefined ){
	_pointSize = 2 ;
}

data_xml = new XML();
var _sendLv:LoadVars = new LoadVars();
data_xml.onLoad = startGraph;
trace("doGet " + _doGet);

if (_doGet == true){
	trace("doing get " + _xmlFile);
	_data_xml.sendAndLoad(_xmlFile + "?q=dummy",data_xml);
} else {
	if (_noCache == true){
		data_xml.load(_xmlFile + "?cachebuster=" + new Date().getTime());
		//data_xml.load(_xmlFile);
	} else {
		data_xml.load(_xmlFile);
	}
}
data_xml.ignoreWhite = true;

// the following are various items of global data
// could be picked up from xml or passed params

// length of axes
var _xLen:Number = getParam("xLen") == ""?420:Number(getParam("xLen"));
var _yLen:Number = getParam("yLen") == ""?420:Number(getParam("yLen"));

//var _yLen:Number = 420;
var _xOrig:Number = 50; // origin
var _yOrig:Number = _yLen + 30;
var _intervalFont = 10;
var _intervalHeight = 15;
var _intervalWidth = 20;
var _titleWidth = 200;
var _titleHeight = 25;
var _popUpWidth = 200;
var _mpWidth = 200;
var _seqWidth = 100;
var _markLen = 5;

var _seqOrig:Number = _yOrig + _markLen + _intervalHeight + _titleHeight + 10;
var _seqXOrig:Number = _xOrig + 50;
//these are global, just for convenience

var _xMult:Number = 1;
var _yMult:Number = 1;
var _seqMult:Number = 1;

var _xStart:Number = 0;
var _yStart:Number = 0;
var _seqStart:Number = 0;

var _xLogMult:Number = 1;
var _yLogMult:Number = 1;

//var _xName:String = "";
//var _yName:String = "";

var _xName:String = (getParam("xName") == ""?"":getParam("xName"));
var _yName:String = (getParam("yName") == ""?"":getParam("yName"));
var _colour1:String = (getParam("colour1") == ""?"0000ff":getParam("colour1"));
var _colour2:String = (getParam("colour2") == ""?"00ff00":getParam("colour2"));
var _colour3:String = (getParam("colour3") == ""?"ff0000":getParam("colour3"));
var _defaultColour:String = (getParam("defaultColour") == ""?"ffffff":getParam("defaultColour"));
var _numRanges:Number = (getParam("numRanges") == ""?7:Number(getParam("numRanges")));
_colour1 = "ff0000";
_colour2 = "00ff00";
_colour3 = "0000ff";
_numRanges=7;



//trace("Check params " + _urlStr);

var _seqName:String = "";

var _seqNumber = -1;
var _playMode = false;

var _playCount:Number = 0;;
var _delayCount = 0;
var _colourMin = 0;
var _colourMax = -1;
var _colourVar = "";


var lev:Number=200010;
this.createClassObject(mx.controls.List,"popUp_lb",lev,{_x:50,_y:50});
this.popUp_lb.hScrollPolicy = "on";
this.popUp_lb.maxHPosition = _popUpWidth * 2;
this.popUp_lb.addEventListener("change", popUpListener);
this.popUp_lb._visible = false;
this.popUp_lb.setSize(_popUpWidth, popUp_lb._height);
this.popUp_lb._alpha=100;

//Create seq list


playSpeed_mc =  _root.attachMovie("ScrollBarSymbol","playSpeed",_root.getNextHighestDepth(),{_x:5,_y:_seqOrig + 17,boxLen:0,len:25,width:20,scrollStyle:"meter",name:"playSpeed"});
createTextField("playMax_txt", _root.getNextHighestDepth(),playSpeed_mc._x + 20,this.playSpeed_mc._y - 10,_root._titleWidth,_root._titleHeight);
createTextField("playMin_txt", _root.getNextHighestDepth(),playSpeed_mc._x + 20,this.playSpeed_mc._y + 12,_root._titleWidth,_root._titleHeight);
createClassObject(mx.controls.Button,"play_btn",_root.getNextHighestDepth(),{_x:55,_y:_seqOrig + 5,label:"Play",_width:30});
createClassObject(mx.controls.CheckBox,"trace_ch",_root.getNextHighestDepth(),{_x:55,_y:_seqOrig + 27,label:"Trace"});
this["playMax_txt"].text = "fast";
this["playMin_txt"].text = "slow";
this["trace_txt"].text = "trace";
setSequence(false);



// create linear log and log lists
// do it now just to get it out of the way,
// couls(should?) be done later
var _typeList = ["linear","log"]; 
createClassObject(mx.controls.ComboBox,"xType_cb",_root.getNextHighestDepth(),{_x:_xLen + _xOrig + 20,_y:_yOrig});
createClassObject(mx.controls.ComboBox,"yType_cb",_root.getNextHighestDepth(),{_x:0,_y:0});
this.createClassObject(mx.controls.ProgressBar,"progressBar_pb",_root.getNextHighestDepth(),{_x:100,_y:100});
this["progressBar_pb"].mode="manual";
xType_cb.addEventListener("change", xTypeListener);
xType_cb.setSize(60,xType_cb._height);
xType_cb.setDataProvider(_typeList);

yType_cb.addEventListener("change", yTypeListener);
yType_cb.setSize(60,yType_cb._height);
yType_cb.setDataProvider(_typeList);

// now various functions
var loaded = _root["data_xml"].getBytesLoaded();;
var total = _root["data_xml"].getBytesTotal();
trace("loaded=" + loaded +",total=" + total);


function getParam(str:String)
{

 	//trace(str + "," + _urlStr);
	for(var i=0; i<_urls.length;i++)
	{
	 	//trace(str + "," + _urls[i]);
		var temp = _urls[i].split("=");
		if (temp.length == 2 && temp[0] == str)
		{
			return temp[1];
		}
	}
	return "";
}

function calcStart(min:Number){
	// calculates a "round" start number from the min
	if (min == undefined || min <= 0){
		return 0;
	}
	var expNum = 0;
	var number = min
	expNum = 0;
	cnt = 0;
	while (number >= 10 && cnt < 20)
	{
		expNum++;
		number/=10;
		cnt++
	}
	trace ("calcStart " + number + "," + expNum);
	// we now have a "scientific number 
	// number 0<number<10
	// plus exp_num
	number = Math.floor(number);
	return (number* Math.pow(10,expNum));
}

function calcInterval(max:Number,min:Number){
	// works out a suitable interval on an axis
	trace("calcInterval max=" + max + " ,min=" + min);
	if (min == undefined){
		min = 0;
	}
	if (max == undefined || max <= 0 )
	{
		return 0;
	}
	var expNum = 0;
	var number = max - min
	if (number <= 9 ){
		return 1;
	}
	
	trace("calcInterval number=" + number + "min="+min);
	//number=max;
	if (number == 0)
	{
		expNum=0;
	}
	else if (Math.abs(number) > 1)
	{
		expNum = 0;
		while (number >= 10)
		{
			expNum++;
			number/=10;
		}
	}
	trace ("calcInterval 1" + number + "," + expNum);
	// we now have a "scientific number 
	// number 0<number<10
	// plus exp_num
	number = Math.ceil(number * 10);
	expNum--;
	trace ("calcInterval 2" + number + "," + expNum);
	var retBase:Number = 0;
	var retExp:Number = expNum;
	if (number <= 15){
		retBase = 2;
	}
	else if (number < 25)
	{
		retBase = 4;
	}
	else if (number < 40)
	{
		retBase = 5;
	}
	else if (number < 80)
	{
		retBase = 10;
	}
	else 
	{
		retBase = 20;
	}
	//trace("finishing retBase " + retBase + "," + retExp);
	return (retBase * Math.pow(10,retExp));

}


function calcMult(interval:Number,max:Number,len:Number,start:Number){
	//works out the multiplication factor
	if (start == undefined){
		start = 0;
	}
	var numIntervals:Number = Math.floor((max - start) / interval);
	trace("numIntervals=" + numIntervals);
	if ((max - start) % interval > 0){
		numIntervals++;
	}
	trace("calcMult numInterval=" + numIntervals + ",interval=" + interval +",len=" + len+ ",start=" + start);
	return((numIntervals * interval)/ len)
}

function calcLogMult(interval:Number,max:Number,len:Number,start:Number){
	// multiplication factor for log scale
	if (start == undefined){
		start = 0;
	}
	var numIntervals = (max  - start)/ interval;
	if ((max - start) % interval > 0){
		numIntervals++;
	}
	//trace("calcMult " + temp + "," + interval +"," + len);
	return((Math.log(numIntervals * interval))/ len)
}


function doXAxis(nodeName:String,linear:Boolean){
	// creates the x-axis
	// this and doYaxis could probably be partially combined to
	// save some code
	if (linear == undefined){
		linear = true;
	}
	var node:Object = getNode(nodeName,false);
	trace("doXAxis " + nodeName + "," + linear);
	var xTitle = node["title"];
	var xMax = node["max"];
	var xMin = node["min"];
	var xUnits = node["unit"];
	_xStart = calcStart(xMin);
	var xInterval = calcInterval(xMax,_xStart);
	_xMult = calcMult(xInterval,xMax,_xLen,_xStart);
	_xLogMult = calcLogMult(xInterval,xMax,_xLen,_xStart);
	trace("_xLogMult=" + _xLogMult +",_seqNumber="+_seqNumber+",index=")
	//trace("trace _xname is " + _xName);
	//trace ("doXAxis xmax is " +  xMax + ",xInterval " + xInterval + ",xMult " + _xMult);
	//trace("doXAxis xTitle=" + xTitle + ",start=" + _xStart);

	if (xAxis_mc != null ){
		xAxis_mc.removeMovieclip();
		trace("removed movieclip " + xAxis_mc + "," + xAxis_mc._x);
	}
	xAxis_mc = _root.attachMovie("axisSymbol","xAxis",/*_root.getNextHighestDepth()*/10001,{_x:_xOrig ,_y:_yOrig,titleText:xTitle,units:xUnits,start:_xStart,interval:xInterval,mult:_xMult,logMult:_xLogMult,linear:linear});
	xAxis_mc.create(_xLen,2,false);
	//createXPopUp();
	doXPopUp(xAxis_mc,_xLen/ 3 + _xOrig,_yLen - popUp_lb.height);
	
	//xAxis_mc._visible = true;
	//setColour();	
}
function setColour(noX:Boolean){

	//colour_cb.setDataProvider(_menuList);
//	trace("setColour " + _menuList.length);
	colour_cb.addItemAt(0,{label:_defaultColourString,data:_defaultColourString});
	for (var i=0; i <_menuList.length;i++){
		colour_cb.addItemAt(i + 1,{label:_menuList[i].label,data:_menuList[i].data});
	}
	colour_cb.sortItemsBy("label","ASC");
	//colour_cb.

}

function setSequence(seqMode:Boolean){
	//switches sequence stuff on or off
	play_btn._visible = seqMode;
	this["playMax_txt"]._visible = seqMode;
	this["playMin_txt"]._visible = seqMode;
	playSpeed_mc._visible = seqMode;
	seqPointer_mc._visible = seqMode;
	this.trace_ch._visible = seqMode;
	this.trace_ch.selected = false;
	this["sequence_txt"]._visible = seqMode;
	trace("setSequence sequence number="+_seqNumber + ",trace_ch=" + trace_ch.selected);
}

function doXPopUp(mc:MovieClip,x:Number,y:Number){
	trace("doPopUp x=" +x +",y=" + y + ",verticalAxis=" + verticalAxis +",vis=" + popUp_lb._visible);
	mc.onRelease = function(){
		mp_lb._visible = false;
		trace("popUp on release vertical=" + verticalAxis +",vis=" + _popUp_lb._visible)
		if (verticalAxis == false && popUp_lb._visible == true){
			trace("hiding popup");
			// ie were previously on x
			// so hide pop up
			popUp_lb._visible = false;
		} else {
			// display popup
			trace("displaying popup xAxis");
			var yNode:Object = getNode(_yName,true);
			
			if (yNode["sequences"] != null && popUp_lb.getItemAt(0).label != _sequenceOptionString){
				popUp_lb.addItemAt(0,{label:_sequenceOptionString,data:_sequenceOptionString});
			}
			
			//xSeq_ch._visible = (yNode["sequences"] != null);
			popUp_lb._x = _xLen/ 3 + _xOrig; // locate over axis label
			popUp_lb._y = _yLen - popUp_lb.height;
			//popUp_lb._x = x;
			//popUp_lb._y = y;
			popUp_lb.rowCount = 10;
			popUp_lb._visible = true;
		}
		verticalAxis = false;
	}
}

function doSeqAxis(force:Boolean,x:Number,y:Number){
	// creates the sequence (x) axis
	trace("doSeqAxis isSequenceOn=" + isSequenceOn() + ",force=" + force + ",yName=" + _yName + ",x=" + x + ",y=" + y)
	
	setSequence(false);
	
	if (seqAxis_mc != null ){
		seqAxis_mc.removeMovieClip();
	}
	var seqTitle:String;
	if (force == true){
		seqTitle = _sequenceOptionString;
		_seqList = getNode(_yName,true)["sequences"];
		trace("force="+ force + ",seq" + _seqList);
		if (_seqList == null){
			return;
		}
	} else {
		seqTitle = "";
		doSeqList();
		if (isSequenceOn () == false){
			return;
		}
	}
	_seqNumber = _seqList[_seqList.length - 1];
	
	//play_btn._visible = true;
	trace("doSeqAxis setSequence " + _seqNumber);
	setSequence(true);
	
	//if (_seqNumber < 0){
		//_seqNumber = _seqList[_seqList.length -1];
	//}
	var varName:String = _yName;
	if (varName == ""){
		return;
	}
	var yNode:Object = _variableData[_yName];
	if (yNode == null){
		return;	
	}
	trace("have got yNode");
	//var seqList = yNode["sequences"];
	trace("seqList " + _seqList);
	var min = _seqList[0];
	var max = _seqList[_seqList.length - 1];
	trace("doSeqAxis " + varName + ",min=" + min + ",max=" + max);
	var xUnits = "";
	//if ((max - min )> 9){
		//_seqStart = calcStart(min);
	//} else {
		_seqStart = Number(min);
	//}
	var interval = calcInterval(max,_seqStart);
	_seqMult = calcMult(interval,max,_xLen,_seqStart);
	trace("trace seq interval=" + interval + "seqMult=" + _seqMult + ",seqStart=" + _seqStart); 
	trace("will now create seq");
	//seqAxis_mc = _root.attachMovie("axisSymbol","seqAxis",_root.getNextHighestDepth(),{_x:_xOrig ,_y:_seqOrig,titleText:xTitle,units:xUnits,start:_seqStart,interval:interval,mult:_seqMult,logMult:_xLogMult,linear:true});
	if (x == undefined){
		x = _seqXOrig;
	}
	
	if (y == undefined){
		y = _seqOrig;
	}
	trace("trace " + "x=" + x + ",y=" + y);
	seqAxis_mc = _root.attachMovie("axisSymbol","seqAxis",/*_root.getNextHighestDepth()*/10003,{_x:x,_y:y,titleText:seqTitle,units:xUnits,start:_seqStart,interval:interval,mult:_seqMult,logMult:_xLogMult,linear:true});
	seqAxis_mc.create(_xLen,2,false);
	seqAxis_mc._visible = true;

	var temp = seqAxis_mc.setPointerVal(_seqNumber);
	if (force == true){
		trace_ch.selected = true;
		if(seqPointer_mc != null){
			seqPointer_mc._visible = false;
		}
	} else {
		if (seqPointer_mc == undefined){
			seqPointer_mc = _root.attachMovie("pointerSymbol","seqPointer",_root.getNextHighestDepth(),{_x:x,_y:y + _markLen + _intervalHeight});;
			seqPointer_mc.create(null,_markLen * 2);
		}
	}
	doSeqPointer(temp,x);
	trace("doSeqAxis seqNumber=" + _seqNumber + ",temp=" + temp);

		
	if (force == true){
		 doXPopUp(seqAxis_mc,_xLen/ 3 + _xOrig,_yLen - popUp_lb.height);
	} else {
		seqAxis_mc.onRelease = function(){
			colourSel = false;
			trace("trace seqAxis release " + this._xmouse + "," + _root._xmouse);
			_root._seqNumber = this.positionPointer(this._xmouse);
			
			var temp = this.setPointerVal(_root._seqNumber); 
			trace("_seqNumber="+_seqNumber+ ",temp=" + temp);
			_root.doSeqPointer(this.setPointerVal(_root._seqNumber)); 
		
			trace("floored i=" + Math.floor(i) + ",i="+i +",start=" + this.start);
			trace("seqaxis release xAxis=" + xAxis_mc);
			//doPoints();
			
			//doSeqAxis(false);
			//doXAxis();
			doPoints()

			
			
		}
	}
}
function getNode(nodeName:String,yAxis:Boolean){
	if (nodeName == null ){
		nodeName = (yAxis==true?_yName:_xName);
	}
	//trace("getNode " + nodeName );
	//var node:XMLNode;
	var node:Object;
	if (nodeName == ""){
		node = getVariableNum(Number(yAxis));
		nodeName = node["name"];
	} else {
		node = getVariable(nodeName);
	}
	// ensure that we "remember" the apprropriate nodeName
	if (yAxis == true ){
		_yName = nodeName;
	} else {
		_xName = nodeName;
	}
						 
	return node;
}

function doYaxis(nodeName:String,linear:Boolean){
	// creates y Axis
	// see comment above
	if (nodeName == null){
		nodeName = _yName;
	}
	if (linear == undefined){
		linear = true;
	}
	trace("doyaxis " + nodeName + "," + linear);
	//var node:XMLNode;
	//var node:Object;
	
	var node:Object = getNode(nodeName,true);
	var yTitle = node["title"];
	var yMax = node["max"];
	var yMin = node["min"];
	var yUnits = node["unit"];
	_yStart = calcStart(yMin);
	var yInterval = calcInterval(yMax,_yStart);
	_yMult = calcMult(yInterval,yMax,_yLen,_yStart);
	_yLogMult = calcLogMult(yInterval,yMax,_yLen,yStart);
	//_yName = nodeName;

	if (yAxis_mc != null){
		yAxis_mc.removeMovieClip();
	}
	//trace ("doYaxis ymax is " +  yMax + ",yInterval " + yInterval + ",yMult " + _yMult);
	//trace("doYaxis title is " + yTitle);
	yAxis_mc = _root.attachMovie("axisSymbol","yAxis",/*_root.getNextHighestDepth()*/10002,{_x:_xOrig,_y:_yOrig,titleText:yTitle,units:yUnits,start:_yStart,interval:yInterval,mult:_yMult,logMult:_yLogMult,linear:linear});

	yAxis_mc.create(_yLen,2,true);
	yAxis_mc._visible = true;

	yAxis_mc.onRelease = function(){
		colourSel = false;
		if (verticalAxis == true && popUp_lb._visible == true){
			// ie were previously on y
			// so hide pop up
			popUp_lb._visible = false;
		} else {
			//trace("y axis showing");
			// display popup
			
			
			if (popUp_lb.getItemAt(0).label == _sequenceOptionString){
				popUp_lb.removeItemAt(0);
			}
			popUp_lb._x = 50;
			popUp_lb._y = 50;
			popUp_lb._visible = true;
		}
		verticalAxis = true;
	}
}




function isInArray(arr:Object,val:String){
	for (var i =0;i<arr.length;i++){
		if (val == arr[i]){
			return true;
		}
	}
	return false;
}
function doSeqList(){
	var xNode:Object= getNode(_xName,false);
	_seqList = xNode["sequences"];
	var yNode:Object=getNode(_yName,true);
	var temp:Array = yNode["sequences"];
	trace("trace doSeqList _seqList=" + _seqList + ",temp=" + temp + ",_xName=" + _xName);
	if (_seqList == undefined || temp == undefined){
		_seqList = undefined;
		return;
	}
	trace("trace doSeqList temp=" + temp);
	for (var i=0;i <_seqList.length;i++){
		if (isInArray(temp,_seqList[i]) == false){
			trace("deleting "+ i + "," + _seqList[i]);
			_seqlist = _seqList.splice(i,1);
		}
	}
	trace("trace doSeqlist _seqlist=" + _seqList);
}

function doGraph(nmeX:String,nmeY:String) {
	//doSeqList();
	trace ("seqList is  " + _seqList);
	doXAxis(nmeX);
	doYaxis(nmeY);
	doSeqAxis(false);
	doPoints();
}
function clearPoints(){
	//trace ("clearpoints");

	this.clear();
	for (var nme in _pointData){
		_pointObjs[nme]._visible = false;
		//_pointObjs[nme].removeMovieClip();
	}
	// the rest is obselesent
}
function getHighestPointDataSeq(nme:String,varName:String){
	//return null;
	var list;
	var node = getVariable(varName);
	var list = node["sequences"];
	//trace("getHighestPointDataSeq nme=" + nme + ",varName=" + varName + ",list=" + list + ",listlen=" + list.length);
	if (list == null){
		return null;
	}
	//trace("getHighestPointDataSeq searching");
	for (var i = list.length -1 ;i >= 0; i--){
		var val:Number = getPointDataSeq(nme,varName,list[i]);
		
		//trace("i=" + i + ",year=" + list[i] + ",val=" + val);
		if (val != undefined){
			return val;
		}
	}
	return null;
}
function getPointSequenceObj(nme:String,varName:String,seqNum:Number){
	//trace("getsequencedata nme=" + nme + ",varName=" + varName +",seqNum=" + seqNum);
	if (seqNum < 0){
		return null;
	}
	// when doing "play" we can have fractional sequnce
	// numberws - just for smoother drawing
	var pointData:Object = _pointData[nme];
	if (pointData == null){
		return null;
	}
	var sequences:Object = pointData["sequences"];
	if (sequences == null){
		return null;
	}
	var variableNode = sequences[varName];
	if (variableNode == null){
		return null;
	}
	var temp:String = "s" + seqNum;

	var seqObj:Object = variableNode["s" + seqNum];
	return seqObj;
}



function getPointDataSeq(nme:String,varName:String,seqNum:Number){
	//trace("getsequencedata nme=" + nme + ",varName=" + varName +",seqNum=" + seqNum);
	var seqObj:Object = getPointSequenceObj(nme,varName,seqNum);
	if (seqObj != null){
		return seqObj.value;
	}
	return null;
						 
}
function getValue(nme:String,varName:String){
	// gets the value of data in normal case
	// when we are plotting one type of data against
	// another
	//trace ("getValue nme=" + nme + ",varName=" + varName);
	var val:Number;
	if (isSequenceOn() == false){
		val = getPointDataVar(nme,varName);
		//trace("val=" + val);
		if (val == null){
			val = getHighestPointDataSeq(nme,varName);
			//trace("val=" + val);
		}
		return val;
	} else {
		//trace("in sequence seq=" + _seqNumber + ",val=" + getPointDataSeq(nme,varName,_seqNumber));
		return getPointDataSeq(nme,varName,_seqNumber);
	}
}
function getValueSeq(nme:String,varName:String){
	// This loks for the sequence value first,
	// takes a variable if sequence not found
	var val:Number = null;
	if (isSequenceOn() == true){
		val = getPointDataSeq(nme,varName,_seqNumber);
		trace("getValueSeq in sequence seq=" + _seqNumber + ",val=" + val);
	}
	if (val == null){
		//val = getPointDataVar(nme,varName);
		val = getValue(nme,varName);
		trace("getValueSeq val was null nme=" + nme + ",varName=" + varName + ",val=" + val);
	}
	return val;
		
		
}


function getXValue(nme:String){
	// gets the value of data in normal case
	// when we are plotting one type of data against
	// another
	return getValue(nme,_xName);
}
function getYValue(nme:String){
	// gets the value of data in normal case
	// when we are plotting one type of data against
	// another
	return getValue(nme,_yName);
}
function getSeqValue(nme:String){
	// gets the value of data in the case case
	// when we are plotting a type of data 
	//against a sequence number
	xVal = seqList[_seqNumber];
	yVal = getPointDataSeq(nme,_yName,_seqNumber);
}

function doPoints(xLinear:Boolean,yLinear:Boolean,traceMode:Boolean,seqMode:Boolean) {
	trace("starting doPoints traceMode=" + traceMode +",seqmode=" + seqMode);
	//this["sequence_txt"].text = _sequenceOptionString + "  :- " + _seqNumber;
	this["sequence_txt"].text =_seqNumber;
	this["sequence_txt"].setTextFormat(_sequenceFormat);
	this["sequence_txt"]._x = _xLen + _xOrig - this["sequence_txt"]._width;


	if (xLinear == undefined){
		xLinear = true;
		if (xType_cb.enabled == true){
			xLinear = (xType_cb.selectedIndex == 0)
		}
	}
	if (yLinear == undefined){
		yLinear = (yType_cb.selectedIndex == 0)
	}
	if (traceMode != true){
		clearPoints();
	}
	var cnt = 0;
	var selItems:Boolean = isAnySelected();
	for (var nme in _pointData){
		//trace ("doPoints in loop -  nme is " + nme);
		var node:Object = _pointData[nme];
		//trace ("doPoints node is " + node + "\n_xName " + _xName + " _yName " + _yName);
		var colour:String  = "0x" + node["colour"];
		var pointSize:Number  = node["pointsize"];
		if (pointSize == null){
			pointSize = _pointSize;
		}
		var group:String  = node["group"];
		var link:String  = node["link"];
		var xVal:Number;
		var yVal:Number;
		
		var xUnits:String;
		var xDesc:String;
		var x:Number;
		if (seqMode == true){
			xVal = _seqNumber;
			yVal = getPointDataSeq(nme,_yName,_seqNumber);
			xDesc  = String(_seqList[_seqNumber]);
			xUnits = "";
			//trace("_seqList=" + _seqList + ",seqNumber," +_seqNumber + ",seqMode=" + seqMode + ",xVal=" + xVal + ",yVal=" + yVal);
			if (xVal != null && yVal != null){
				x = _xOrig + ((xVal - _seqStart)/_seqMult);
				//trace ("x=" + x);
			}
		} else {
			xVal = getXValue(nme);
			yVal = getYValue(nme);
			xDesc  = getVariableData(_xName,"title");
			xUnits  = getVariableData(_xName,"unit");
			if (xVal != null && yVal != null){
				if (xLinear == true){
					x = _xOrig + ((xVal - _xStart)/_xMult);
				}else {
					x = _xOrig + (Math.log(xVal - _xStart)/_xLogMult);
					if (x < 0 || x > _xLen){
						continue;
					}
				}
			}
		}
		var yDesc:String  = getVariableData(_yName,"title");
		var yUnits:String  = getVariableData(_yName,"unit");
		
		//trace("doPoints ("+_xName + ":" + _yName +")" + xName + "," + yName);
		//trace ("doPoints xVal " + xVal + " yVal " + yVal);
		
		if (xVal != null && yVal != null){
			var y:Number;
			if (yLinear == true){
				var y = _yOrig - ((yVal - _yStart)/_yMult);
			}else {
				var y = _yOrig - (Math.log(yVal - _yStart)/_yLogMult);
				if (y < 0 || y > _yLen){
					continue;
				}
			}
			//trace ("y=" + y);
			var opacity:Number = 100;
			var sel:Boolean = false;
			if (selItems == true){
				opacity = isSelected(nme) == true?100:_opacity;
				sel = isSelected(nme) == true?true:false;
			}
			//trace("opacity=" + opacity + ",selItems" + selItems);
			//var levNumber = _root.getNextHighestDepth();
			var lev:Number= 100000 + cnt;
			//trace("sel is " + sel + ",opacity=" + opacity);
			
			if (_pointObjs[nme] == undefined){
				newPoint = _root.attachMovie("pointSymbol","point" + cnt++,lev/*_root.getNextHighestDepth()*/,
				{
					 _x:x,_y:y,len:pointSize,
				 	xVal:xVal,xName:xDesc,
				 	yVal:yVal,yName:yDesc,name:nme,
				 	xUnits:xUnits,yUnits:yUnits,
				 	group:group,link:link,
				 	_alpha:opacity,sel:sel
			 	}
			 	);
			} else {
				newPoint = _pointObjs[nme];
				//trace("tracemode="+ traceMode + ",visible="+ newPoint._visible);
				if ((selItems == false  || sel == true) && traceMode == true && newPoint._visible == true){
					lineStyle(0,colour,100);
					moveTo(newPoint._x,newPoint._y);
					lineTo(x,y);
				}
				newPoint. _x = x;
				newPoint._y = y;
				newPoint.len = pointSize;
				newPoint.xVal = xVal;
				newPoint.xName = xDesc;
				newPoint.yVal = yVal;
				newPoint.yName=yDesc;
				newPoint.name = nme;
				newPoint.xUnits=xUnits;
				newPoint.yUnits=yUnits;
				newPoint.group=group;
				newPoint.link=link;
				newPoint._alpha=opacity;
				newPoint.sel=sel;
				newPoint._visible = true;
			}
			//trace("_colourVar="  + _colourVar +",_colourMax=" +_colourMax + ",_colourMin=" + _colourMin);
			if (_colourVar != "" && _colourMin >=0  && _colourMax > _colourMin){
				var colourVal:Number = 0;
				if (_colourVar == _xName){
					colourVal = xVal;
				} else  if (_colourVar == _yName){
					colourVal = yVal;
				} else  {
					//colourVal = getPointDataVar(nme,_colourVar);
					//colourVal = getValueSeq(nme,_colourVar);
					colourVal = getValue(nme,_colourVar);
					//trace("called getvalueSeq nme=" + nme + ",_colourVar=" + _colourVar + "," + _colourMax + "," +  _colourMin + ":" + colourVal);
				}
				trace ("calling getColour " +  colourVal + "," + _colourMin + "," +  _colourMax);
				colour = colourGrad_mc.getColour(colourVal,true);
				//trace ("get colour " +  yVal + "," + _colourMax + "," +  _colourMin);
			}
				
			newPoint.create(Number(colour));
			//_points.push(newPoint);
			_pointObjs[nme] = newPoint;
		}
	}
	trace("finished doPoints");
}

function updatePoints(checkColour:Boolean) {
	var cnt = 0;
	var selItems:Boolean = isAnySelected();
	trace("starting updatePoints " + selItems);
	
	//for(var i =0;i <_points.length;i++){
	for(var nme in _pointObjs){
		//trace ("doPoints starting loop. nme is " + nme);
		var pt:MovieClip = _pointObjs[nme]; 
		//trace("nme=" + nme);
		pt._alpha = 100;
		pt.sel = false;
		if (selItems == true){
			var sel = isSelected(nme);
			
			pt.sel = sel;
			pt._alpha = (pt.sel==true?100:_opacity);
			if (checkColour == true && _colourMax > 0 && _colourMax > _colourMin){
				//colour = colour_mc.getColour(yvalue * 100 / (_colourMax - _colourMin));
				colour = colourGrad_mc.getColour(yVal * 100 / (_colourMax - _colourMin),true);
				trace ("get colour " +  yVal + "," + _colourMax + "," +  _colourMin);
				trace ("get colour " +  yVal * 100 / (_colourMax - _colourMin) + "," + colour);
			}

		}
		pt.create();
	}
}


function startGraph(success:Boolean) {
	if (success == true) {
		_root["progressBar_pb"]._visible = false;
		if (this.hasChildNodes()){
			trace("startGraph "+ this.hasChildNodes());
			rootElement = this.firstChild;
			trace("root is "+ rootElement.hasChildNodes()+":" + this.nodeName);
			parseQuango(rootElement);
			doGraph();
			//_menuList.addItem({label:"MP Sequence",data:"MP"});
			var maxSize = 0;
			for (var nme in _variableData){
				var node:Object = _variableData[nme];
				var temp:String = node["title"];
				//trace("trace temp is " + temp);
				_menuList.addItem({label:temp,data:nme});
				//trace ("trace textstyle " +popUp_lb.textstyle + ":" + popUp_lb.textStyle.getTextExtent("STUFF"));
			}
			setColour();
			//trace ("data provider=" + _menuList);
			//popUp_mc.rowCount = 2;
			popUp_lb.rowCount = 10;//_menuList.length;
			popUp_lb.setDataProvider(_menuList);
			popUp_lb.sortItemsBy("label","ASC");
		
		}
	}
}
// the following 3 functions do the parsing

function parseQuango(quango:XMLNode) {
	// used by cropForm
	//trace("parseQuango " + quango);
	var children = quango.childNodes;
	for (var i = 0; i<children.length; i++) {
		//trace ("looking through quango :- " + children[i].nodeName);
		if (children[i].nodeName == "variables") {
			parseVariables(children[i]);
			trace("variables found");
		} else if (children[i].nodeName == "points"){
			parsePoints(children[i]);
			trace("points found");
		}
	}
}
function parseVariableData(variableData:XMLNode) {
	var list = new Object();
	var children = variableData.childNodes;
	//trace("parseVariableData " + children.length);
	for (var i = 0; i<children.length; i++) {
		if (children[i].nodeName == "sequences"){
			var seqList = new Array();// should it be a hash
			var sequences = children[i].childNodes;
			//trace("sequenceNodes " + sequences.length);
			for (var j = 0; j<sequences.length; j++) {
				//trace("adding sequence to variableData - name=" + sequences[j].nodeName + ",val=" + sequences[j].firstChild.nodeValue);
				//seqList[sequences[j].nodeName]=sequences[j].firstChild.nodeValue;
				seqList.push(sequences[j].firstChild.nodeValue);
			}
			list["sequences"]=seqList;
		} else {
			//trace("adding variableData name=" + children[i].nodeName + ",val=" + children[i].firstChild.nodeValue);
			list[children[i].nodeName]=children[i].firstChild.nodeValue;
		}
	}
	return list;
}
function parsePointData(pointData:XMLNode) {
	var list = new Object();
	var children = pointData.childNodes;
	//trace("parsePointData " + children.length);
	for (var i = 0; i<children.length; i++) {
		//trace("pointItem is " + children[i].nodeName);
		if (children[i].nodeName == "sequences"){
			var sequences = children[i].childNodes;
			var seqList = new Object();
			//trace("sequencesNode " + sequences.length);
			for (var j = 0; j<sequences.length; j++) {
				//trace("adding sequence to pointsData - name=" + sequences[j].nodeName + ":" + sequences[j]);
				var valList = new Object();
				var vals = sequences[j].childNodes;
				//trace("inside sequences Node " + vals.length);
				for (var k = 0; k< vals.length; k++) {
					var temp:String=vals[k].nodeName;
					var seqObj:Object = new Object();
					seqObj["value"] = vals[k].firstChild.nodeValue;
					var temp1:XMLNode = vals[k];
					if (vals[k].attributes != null){
						trace("special again");
						for(var attr in temp1.attributes){
							seqObj[attr] = temp1.attributes[attr];
						}
					}
					//}
					//valList[temp]=vals[k].firstChild.nodeValue;
					valList[temp]= seqObj;
				}
				seqList[sequences[j].nodeName]=valList;
			}
			list["sequences"] = seqList;
		} else if (children[i].nodeName == "variables"){
			var variables = children[i].childNodes;
			var varList = new Object();
			//trace("sequencesNode " + sequences.length);
			for (var j = 0; j<variables.length; j++) {
				//trace("adding variable to pointData - name=" + variables[j].nodeName + ":" + variables[j].firstChild.nodeValue);
				varList[variables[j].nodeName]= variables[j].firstChild.nodeValue; 
			}
			list["variables"] = varList;
		} else {
			//trace("adding variableData name=" + children[i].nodeName + ",val=" + children[i].firstChild.nodeValue);
			list[children[i].nodeName]=children[i].firstChild.nodeValue;
		}
	}
	return list;
}

function parseVariables(variables:XMLNode) {
	var children = variables.childNodes;
	//trace("parseVariables " + children.length);
	for (var i = 0; i<children.length; i++) {
		//trace("variable is " + children[i]);
		if (children[i].nodeName = "variable"){
			var nodeName:String = getItem(children[i],"name");
			//_variablesData[nodeName] = children[i];
			// This will replace above line
			var variableData:Object = parseVariableData(children[i]);
			if (variableData != null){
				_variableData[nodeName] = variableData;
				//trace("added variable " + nodeName + variableData);
			}
		}
	}

}
function parsePoints(points:XMLNode) {
	//var mpList = new Array();
	var children = points.childNodes;
	for (var i = 0; i<children.length; i++) {
		if (children[i].nodeName = "point"){
			var nodeName:String = getItem(children[i],"name");
			//_pointsData[nodeName] = children[i];
			// above line replace by following
			//trace ("add " + nodeName + " to _pointData");
			var pointData = parsePointData(children[i]);
			if (pointData != null){
				_pointData[nodeName] = pointData;
				var temp = pointData["name"];
				var lab = temp.substr(0,temp.indexOf(","));
				//_mpList.push(pointData["name"]);
				//trace("temp is " + temp + ",lab is " + lab);
				_itemList.addItem({label:lab,data:temp,sel:0});
				//trace("added point " + nodeName + variableData);
			}

			//trace("added point " + nodeName);
		}

	}
	this.createTextField("sequence_txt", this.getNextHighestDepth(),_xLen + _xOrig - 120,0,_root._titleWidth,_root._titleHeight);
	this["sequence_txt"].autoSize = true;
	this["sequence_txt"].embedFonts = 10;	
	this["sequence_txt"]._alpha = 50;	
	//this["sequence_txt"].text = "Ok there";
	this["sequence_txt"]._visible = false;
	//colourGrad_mc = _root.attachMovie("colourGradSymbol","colourGrad",_root.getNextHighestDepth(),{_x:_xOrig + _xLen + 25,_y:60,width:10,height:80});
	colourGrad_mc = _root.attachMovie("colourGradSymbol","colourGrad",lev + 1,{_x:_xOrig + _xLen + 25,_y:60,width:10,height:80});
	colourGrad_mc._visible = false;
	this.createClassObject(mx.controls.ComboBox,"colour_cb",lev + 2 /*5000000000*//*_root.getNextHighestDepth()*/,{_x:_xLen + _xOrig + 35 ,_y:30});
	//this.createClassObject(mx.controls.ComboBox,"colour_cb",5000000000/*_root.getNextHighestDepth()*/,{_x:_xLen + _xOrig + 15 ,_y:55});
	//this.createClassObject(mx.controls.ComboBox,"colour_cb",_root.getNextHighestDepth(),{_x:100,_y:100});
	colour_cb.addEventListener("change", colourListener);
	colour_cb.hScrollPolicy = "on";
	colour_cb.rowCount = 4;
	colour_cb.maxHPosition = _popUpWidth * 2;
	colour_cb.setSize(_popUpWidth -10, colour_cb._height);
	colour_cb._alpha=100;
	if (_customList == true){
		itemList_mc = _root.attachMovie("ItemListSymbol","itemList",_root.getNextHighestDepth(),{_x:_xLen + _xOrig + 25,_y:150});
		opacity_mc =  _root.attachMovie("ScrollBarSymbol","opacity",_root.getNextHighestDepth(),{_x:_xLen + _xOrig + 55,_y:420,boxLen:0,len:150,width:20,vertical:false,scrollStyle:"meter",name:"opacity",startLabel:"0",endLabel:"100"});
		opacity_mc._rotation = -90;
		this.createTextField("opacity_txt", _root.getNextHighestDepth(),_xLen + _xOrig + 45,415,_root._titleWidth,_root._titleHeight);
		//trace("trace x=" + temp1 + ",y=" + temp2);
		this["opacity_txt"].text= _unselOpacityString + _opacity + " %";
		this["opacity_txt"].autoSize=true;
		//this["opacityVal_txt"].text= _opacity.toString();
	}else {
	}
	//test_mc.create();
	trace("mpList length=" + _itemList.length);
	//mp_lb.setDataProvider(mpList);
}
// these are 6 utility function to fetch data from
// the stored Xml nodes
// Would be better to store the data in proper 
// structures rather than xmlNodes.  Would probably 
// retain these function but rework them to access
// new data structures - rest of the code would
// then be unchanged.
// logic of these would go into main parse 

function getItem(node:XMLNode,item:String){
	//trace("getItem " + node + ":" + item);
	var children = node.childNodes;
	for (var i = 0; i<children.length; i++) {
		if (children[i].nodeName == item) {
			return children[i].firstChild.nodeValue;
		}
	}
	return null;
}


function getPointDataVar(name:String,item:String) {
	var pointData:Object = _pointData[name];
	//trace("getPointDataVar name=" + name + ",item" + item);
	if (pointData == null){
		return;
	}
	var vars:Object = pointData["variables"];
	if (vars == null){
		return;
	}
		
	return vars[item];
}

function getVariableNum(num:Number) {
	//trace("getvariableNum " + num);
	var cnt:Number = 0;
	for (var nme in _variableData){
	//trace("getvariableNum " + nme + ":" + _variableData[nme]);
		if (cnt++ != num){
			continue;
		}
		return _variableData[nme];
	}
}


function getVariable(nme:String) {
	return _variableData[nme];
}
function getVariableData(name:String,item:String) {
	//trace("getvariableData " + name +"," + item);
	var variable:Object = _variableData[name];
	return variable[item];
}
function doSeqPointer(d:Number,x:Number) {
	trace("doSeqPointer d="+d + ",x=" + x);
	seqPointer_mc._visible = true;
	if (x == undefined){
		x = _seqXOrig;
	}
	seqPointer_mc._x = x + d;
}

// Now the listeners

colourListener.change = function(evnt:Object) {
    trace("colourListener " + colour_cb.selectedIndex + "," + colour_cb.selectedIndex.data );
	if (colour_cb.selectedIndex == 0){
		_colourVar  = "";
		trace("colour range " + _colourMin + ":" + _colourMax);
	}	else if (colour_cb.selectedIndex == 1){
		_colourMin  = getVariableData("population","min");
		_colourMax  = getVariableData("population","max");
		_colourVar  = colour_cb.selectedItem;
		trace("colour range " + _colourMin + ":" + _colourMax);
	} else {
		
		_colourMin  = getVariableData(colour_cb.selectedItem.data,"min");
		_colourMax  = getVariableData(colour_cb.selectedItem.data,"max");
		_colourVar  = colour_cb.selectedItem.data;
		trace("colour range " + colour_cb.selectedItem.data + "," +  colour_cb.selectedItem.label);
		trace("colour range " + _colourMin + ":" + _colourMax);
		
	}

		if (_colourMax == undefined || _colourMin == undefined || _colourVar == undefined || _colourVar == ""){
			_colourVar  = "";
			colourGrad_mc._visible = false;
		} else {
			trace("setting colour gradient");
			colourGrad_mc.setMaxMin(_colourMin,_colourMax);
			colourGrad_mc._visible = true;
			doPoints();
	}
}

xTypeListener.change = function(evnt:Object) {
    trace("xType listener " + xType_cb.selectedIndex + "," + yType_cb.selectedIndex );
	//xAxis_mc._visible = false;
	xAxis_mc.clear();
	//xAxis_mc.removeMovieClip();
	doXAxis(null,xType_cb.selectedIndex == 0);
	doPoints(xType_cb.selectedIndex == 0,yType_cb.selectedIndex == 0);
}
yTypeListener.change = function(evnt:Object) {
    trace("yType listener " + yType_cb.selectedIndex + "," + yType_cb.selectedIndex );
	//yAxis_mc._visible = false;
	//yAxis_mc.removeMovieClip();
	doYaxis(null,yType_cb.selectedIndex == 0);
	doPoints(xType_cb.selectedIndex == 0,yType_cb.selectedIndex == 0);
}
popUpListener.change = function(evnt:Object) {
    trace("popUp listener " + popUp_lb.selectedIndex + "," + popUp_lb.selectedItem);
	popUp_lb._visible = false;
	if (verticalAxis == true){
		//yAxis_mc._visible = false;
		yAxis_mc.removeMovieClip();
		doYaxis(popUp_lb.selectedItem.data);
		if (xAxis_mc == null){
			trace("doing xAxis");
			doXAxis();
		}
		doSeqAxis();
		doPoints();
	}else {
		xType_cb.enabled = true;
		_xSeqName = "";
		
		if (popUp_lb.selectedItem.data == _sequenceOptionString){
			xAxis_mc = xAxis_mc.removeMovieClip();
			doSeqAxis(true,_xOrig,_yOrig);
			xType_cb.enabled = false;
			doPoints(null,true,false,true)
			
		} else {
			doXAxis(popUp_lb.selectedItem.data);
			doSeqAxis(false);
			doPoints();
		}
	}
	//popUp_lb._visible = false;
	var yNode:Object = getNode(_yName,true);
	xSeq_ch._visible = (yNode["sequences"] != null);
	xType_cb.enabled = true;
	trace("xSeq_mc y " + yNode + "," + yNode["sequences"] + ":" + yNode["max"]+"," + xSeq_ch._visible);
}

xSequenceCheckListener.click  = function(){
	trace("xSeq check changed " + xSeq_ch.selected);
	if (xSeq_ch.selected == true){
		xAxis_mc = xAxis_mc.removeMovieClip();
		trace("removed xAxis \"" + xAxis_mc + "\"" + temp);
		
		doSeqAxis(true,_xOrig,_yOrig);
		xType_cb.enabled = false;
		doPoints(null,true,false,true)
	} else {
		doXAxis();
		doSeqAxis(false);
		doPoints()
	}
}
mouseListener.onMouseDown = function() {
	trace("mouse down");
	if (_root.seqPointer_mc.hitTest(_root._xmouse, _root._ymouse)) {
		trace("are over pointer");
		_root.seqPointer_mc.dragged = true;
		_root.seqPointer_mc.startDrag(true,_xOrig,_seqOrig + _markLen + _intervalHeight,_xOrig + _xLen,_seqOrig + _markLen + _intervalHeight);
	}

}

mouseListener.onMouseUp = function() {
	//trace("mouse up");
	if (_root.seqPointer_mc.dragged == true) {
		_root._seqNumber = seqAxis_mc.positionPointer(_root._xmouse - _xOrig);
		trace("are dragging pointer seqNum=" + _root._seqNumber);
		_root.doSeqPointer(seqAxis_mc.setPointerVal(_root._seqNumber)); 
		_root.seqPointer_mc.stopDrag();
		_root.seqPointer_mc.dragged = false;
		doPoints();
	}
}

//A utility function - used to be in 
// axis, but now used also by point
function convertUnits(units:String,val:String,brackets:Boolean) {
	if (units == undefined|| units == ""){
		return val;
	}
	if (val == undefined){
		val = "";
	}
	//trace("convertUnits val is " + val);
	if (brackets == undefined){
		brackets = false;
	}
	trace("units=" + units + ",val=" + val);
	var temp:String = units;
	switch(units){
		case "&pound;" :
		{
			temp="£" + val;
			break;
		}
		default :
		{
			temp = val + " " + units;
			break;
		}
	}
	var retVal:String = ((brackets==true?"(":"") + temp + (brackets==true?")":""));
	trace("units="+units+",temp=" + temp +",val=" + val+",retval=" + retVal);
	return retVal;
}
play_btn.onRelease = function(){
	if (play_btn.label == "Play"){
		play_btn.label = "Stop";
		_playMode = true;
		_seqNumber = _seqList[0];
		trace("play_btn seqNumber=" + _seqNumber);
		setSeqPointer(seqAxis_mc.setPointerVal(_seqNumber));
		_playCount= 0;
	} else {
		play_btn.label = "Play";
		_playMode = false;
	}
}
function isSequenceOn(){
	return (_seqList != undefined && _seqList.length > 0);
}
function isPlayOn(){
	return(play_btn._visible == true && play_btn.label != "Play");
}

onEnterFrame = function() {
	if (progressBar_pb._visible == true){
		var loaded:Number = _root["data_xml"].getBytesLoaded();;
		var total:Number = _root["data_xml"].getBytesTotal();
		trace("loaded=" + loaded +",total=" + total);
		//this.my_pb.setProgress(loaded,total);
		this["progressBar_pb"].setProgress(loaded,total);
		if (loaded >= total){
			trace("making invisible");
			//this["progressBar_pb"]._visible = false;
		}
	}
//pr1_pb.setProgress(1,67);
if (_playMode == false){
		return;
	}
	if (_delayCount > 0){
		_delayCount--;
		return;
	}
	var year:Number = Number(_seqList[_playCount]);
	doSeqPointer(seqAxis_mc.setPointerVal(year));
	_seqNumber = year;
	trace("trace seqNumber=" + _seqNumber); 
	if (_playCount == 0 && this.trace_ch.selected == true){
		// when in trace mode, do points will not 
		// clear points, so do it at the start
		trace("cleared points");
		clearPoints();
	}
	doPoints(null,null,this.trace_ch.selected,(xAxis_mc == null));
	_delayCount = _playDelay;
	if (++_playCount >= _seqList.length){
		play_btn.onRelease();
		cnt = 0;
	}

}
function isAnySelected(){
	//trace ("isAnySelected " + itemList_mc.selectedCount);
	return(itemList_mc.selectedCount != undefined && itemList_mc.selectedCount > 0);
}
function isSelected(nme:String){
	//trace ("isSelectedItem " + nme + "," + itemList_mc.selectedList[nme]);
	return (itemList_mc.selectedList[nme] != undefined && itemList_mc.selectedList[nme] == true);
}
function handleScroll(d:Number,range:Number,round:Boolean,name:String,finish:Boolean){
	//trace("handleScroll d=" + d + ",range=" + range);
	var ratio = d / range;
	if (name == "opacity"){
		_opacity = 0 + Math.floor(ratio * 100);
		trace("opacity=" + _opacity);
		opacity_txt.text = _unselOpacityString + _opacity + " %";
		if (finish == true){
			updatePoints();
		}
	} else if (name == "playSpeed"){
		_playDelay = 24 * ratio;
	}
}