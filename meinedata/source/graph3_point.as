﻿#initclip
//var axisColour:Number = 0xffffff;
// constructor for axis
function Point() {
	this.createTextField("toolTip",this.getNextHighestDepth(),0,-50,250,70);
	this.createTextField("details_txt",this.getNextHighestDepth(),-50,0,250,70);
	//this["toolTip"].text="I am here " + xVal;
	this["toolTip"].background = true;
	this["toolTip"].backgroundColor = 0xffffcc;
	this["toolTip"]._alpha = 100;
	this["toolTip"]._visible = false;
	this["toolTip"].multiline = true;
	this["toolTip"].multiline = true;
	this["toolTip"].autoSize=true;
	this["details_txt"]._visible = false;
	this["details_txt"].autoSize=true;
	//this["toolTip"].background = true;
	//this["toolTip"].backgroundColor  = 0xffffcc;
	this.colour=0x999999;
	super();
}

Point.prototype = new MovieClip();

Point.prototype.showSelected = function(showSel:Boolean) {
	//trace("showSelected " + outlineRGB + "," + fillRGB);
  	if (showSel == true){
	} else {
	}
}

Point.prototype.create = function(colour:Number,len:Number) {
	this.clear();
	//trace ("create - point len " + this.len + ", xVal " + this.xVal + ":" + this.yVal + "," + colour + "(" + this.name + ")" + ",sel=" + this.sel);
	if (len != undefined){
		this.len = len;
	}
 	if (colour != undefined){
		this.colour = colour;
	}
	this.draw(_root._pointFill);
	if (this.sel == true){
		//this.details_txt.text=this.name;
		this.toolTip.text=this.name;
		this.updatePos("toolTip");
	}
	//this.details_txt._visible = this.sel;
	this.toolTip._visible = this.sel;
}

Point.prototype.draw = function(doFill:Boolean) {
	this.clear();
	
	//trace("point - draw " + this.len + ", " + this.colour);
	if (doFill == true){
		this.beginFill(this.colour,100);
	}
	this.lineStyle(0, this.colour, 100);
  	//this.drawCircle(this.len/2, -this.len/2,-this.len/2);
  	this.drawCircle(this.len/2,0,0);
	if (doFill == true){
		this.endFill();
	}
}
Point.prototype.updatePos = function(txtName:String) {
	trace("trace updatePos " + txtName +" - " + this._x + ":" + this._y);
	var temp = this._x - (_root._xOrig + _root._xLen - this[txtName]._width);
	//if (this._x > _root._xOrig + _root._xLen - this[txtName]._width){
		//this[txtName]._x =( - this[txtName]._width);
	if (temp > 0){
		this[txtName]._x = - temp;
	} else {
		this[txtName]._x = 0;
	}
	var temp = this._y - (_root._yOrig - this[txtName]._width);
	//if (this._y > _root._yOrig - this[txtName]._height){
		//this[txtName]._y =( - this[txtName]._height);
	//if (this._y > temp){
		//this[txtName]._y = - temp;
	//} else {
		this[txtName]._y = 0;
	//}
	trace("trace new x " + txtName +" - " + this._x + ":" + this._y);
}

Point.prototype.onRollOver = function() {
	if (this._alpha == 0){
		return;
	}
	//trace("name is " + this.name +  " x is " + this.xVal + " y is " + this.yVal);

	//trace("rollover " + this.link + ",vals=" + this.xVal + ":" + this.yVal);
	//this["toolTip"].text = this.name + "(" + this.group + ")" + "\n" + this.xName + ":" + /*addUnits(this.xVal,this.xUnits)*/convertUnits(this.xUnits,this.xVal,false) + "\n" + this.yName + " : " + /*addUnits(this.yVal,this.yUnits)*/convertUnits(this.yUnits,this.yVal,false);;
	//trace("trace point " + this.xName);
	if (this.xName == "undefined"){
		this.xName = "";
		//trace("trace point set xName " +  this.xName);
	}

	
	this["toolTip"].text = this.name + (this.group == undefined?"":"(" + this.group + ")");
	//this["toolTip"].text+=(" trace " + this.xName);

	this["toolTip"].text+=(this.xName==""?"":"\n" + this.xName + " : " + convertUnits(this.xUnits,this.xVal,false));
	this["toolTip"].text+=(this.yName==""?"":"\n" + this.yName + " : " + convertUnits(this.yUnits,this.yVal,false));
	if (this.link != null && this.link != ""){
		this["toolTip"].text+=("\n" + "(click to view)");
	}
	trace("trace x " + this._x + "," + this._width);
	trace("trace pos " + _root._xOrig + "," + _root._xLen );
	this.updatePos("toolTip");
	/*
	if (this._x > _root._xOrig + _root._xLen - this["toolTip"]._width){
		this["toolTip"]._x =( - this["toolTip"]._width);
	} else {
		this["toolTip"]._x = 0;
	}
	if (this._y > _root._yOrig - this["toolTip"]._height){
		this["toolTip"]._y =( - this["toolTip"]._height);
	} else {
		this["toolTip"]._y = 0;
	}
	trace("trace new x " + this._x + ":" + this._y);
	*/
	
	this["toolTip"]._visible = true;
	this.details_txt._visible = false;	
	//this.outline.col.setRGB(0xFF);
}

Point.prototype.onRollOut = function() {
	this["toolTip"]._visible = false;
	if(!this.selected) {
    	//this.outline.col.setRGB(this.outline.rgb);
  	}
	//this.details_txt._visible = this.sel;	
	this.toolTip.text = this.name;
	this.updatePos("toolTip");
	this.toolTip._visible = this.sel;
}
Point.prototype.onRelease = function() {
	if (this.link != null && this.link != ""){
		trace ("link is " + this.link);
		var lv:LoadVars = new LoadVars();
		lv.send(this.link,"_blank");
	}
}

Point.prototype.drawCircle = function (radius, x, y) {
  // The angle of each of the eight segments is 45 degrees (360 divided by eight),
  // which equals p/4 radians.
  var angleDelta = Math.PI / 4;

  // Find the distance from the circle's center to the control points
  // for the curves.
  var ctrlDist = radius/Math.cos(angleDelta/2);

  // Initialize the angle to 0 and define local variables that are used for the
  // control and ending points. 
  var angle = 0;
  var rx, ry, ax, ay;

  // Move to the starting point, one radius to the right of the circle's center.
  this.moveTo(x + radius, y);

  // Repeat eight times to create eight segments.
  for (var i = 0; i < 8; i++) {

    // Increment the angle by angleDelta (p/4) to create the whole circle (2p).
    angle += angleDelta;

    // The control points are derived using sine and cosine.
    rx = x + Math.cos(angle-(angleDelta/2))*(ctrlDist);
    ry = y + Math.sin(angle-(angleDelta/2))*(ctrlDist);

    // The anchor points (end points of the curve) can be found similarly to the
    // control points.
    ax = x + Math.cos(angle)*radius;
    ay = y + Math.sin(angle)*radius;

    // Draw the segment.
    this.curveTo(rx, ry, ax, ay);
  }
}


Object.registerClass("pointSymbol", Point);

#endinitclip