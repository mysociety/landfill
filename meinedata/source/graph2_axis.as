#initclip
//var axisColour:Number = 0xffffff;
var temp:String = titleText;
// constructor for axis
trace("titleText 1 is " + titleText);
function Axis() {
	trace("titleText 2 is " + this.titleText + "," + this.axisColour);
	super();
	this.colour=0x999999;
	this.pointerVal = -1;
	this.wdth=2;
}

Axis.prototype = new MovieClip();

Axis.prototype.showSelected = function(showSel:Boolean) {
	trace("showSelected " + outlineRGB + "," + fillRGB);
  	if (showSel == true){
	} else {
	}
}
Axis.prototype.create = function(len:Number, wdth:Number, vertical:Boolean,colour:Number) {
	this.clear();
	trace ("create - Axis " + len + "," + wdth + "," + vertical + "," +  colour );
	if (wdth != undefined){
		this.wdth = wdth;
	}
	if (len != undefined){
		this.len = len;
	}
 	if (colour != undefined){
		this.colour = colour;
	}
 	if (vertical != undefined){
		this.vertical = vertical;
	}
	this.draw();
}
Axis.prototype.draw = function() {
	if (this.start == undefined){
		this.start = 0;
	}
	this.clear();
	trace("axis - draw " + this.len + "," + this.colour);
	
	// set some initial values
	// text lenegths shoul reallly be calcultated
	// maybe ?
	var axisIntervalFormat = new TextFormat();
	axisIntervalFormat.align = "center";
	axisIntervalFormat.size = _root._intervalFont;

	// Draw the line itself
	this.lineStyle(this.wdth, this.colour, 100);
  	this.moveTo(0,0);
  	if (this.vertical == true){
		this.lineTo(0,-this.len);
		var xTitle =-(_root._intervalWidth + _root._markLen + + _root._titleHeight);
		var yTitle = -(1 * this.len/3);
		trace("trace xTitle=" + xTitle + ",yTitle=" + yTitle);
		//xTitle = -40;
		//yTitle = -100;
		if (this.titleText != undefined && this.titleText != ""){
			this.createTextField("axisText", this.getNextHighestDepth(),xTitle,yTitle,_root._titleWidth,_root._titleHeight);
			this.setText(this.titleText);
			this["axisText"].autoSize = true;
			this["axisText"]._rotation = -90;
		}
	} else {
		this.lineTo(this.len,0);
		if (this.titleText != undefined && this.titleText != ""){
			this.createTextField("axisText", this.getNextHighestDepth(),this.len/3,_root._markLen + _root._intervalHeight,200,50);
			this["axisText"].autoSize = true("right");
			this.setText(this.titleText);
		}
	}
	// Now the interval marks
	// create textformat for marks
	this.lineStyle(0, colour, 100);
	trace("draw1 start="+this.start);
	//var yIntervalWidth =0;
	//if (vertical == true){
		//yIntervalText = this.mult * this.len;
	//}
	for (var i = 0;((i * this.interval) / this.mult <= this.len) && i < 20;i++){
		// temp
		var temp:Number = i * this.interval;
		//trace ("in loop " + temp + ",mult=" + this.logMult);
		var d:Number;
		if (this.linear == false){
			d = (Math.log(i * this.interval))/ this.logMult;
		} else {
			d = (i * this.interval)/ this.mult;
		}
		
		trace("d=" + d + ",linear="+linear);
		var textName = "interval_"+ i;
		if (this.vertical == true){
			this.moveTo(-_root._markLen,-d);
			this.lineTo(0,-d);
			var yText = -(d + _root._intervalHeight/2);
			var xText = -(_root._markLen + _root._intervalWidth);
			trace("textName " + textName + " lev " + lev + " width " + _root._intervalWidth + " height " + _root._intervalHeight + " x " + xText + " y " + yText);
			this.createTextField(textName,this.getNextHighestDepth(),xText,yText,_root._intervalWidth,_root._intervalHeight);
			this[textName].text= i * this.interval + this.start;
			this[textName].autoSize="right";
			//this[textName].embedFonts = true;
			this[textName].bold = false;
			//this[textName]._rotation = 90;
			axisIntervalFormat.align="right";
			this[textName].setTextFormat(axisIntervalFormat)
		} else {
			this.moveTo(d,_root._markLen);
			this.lineTo(d,0);
			var xText = d - _root._intervalWidth/2;
			var yText = _root._markLen;
			trace("textName " + textName + " lev " + lev + " width " + _root._intervalWidth + " height " + _root._intervalHeight + " x " + xText + " y " + yText);
			this.createTextField(textName,this.getNextHighestDepth(),xText,yText,_root._intervalWidth,_root._intervalHeight);
			this[textName].text= i * this.interval + this.start;
			this[textName].autoSize="center";
			//this[textName].embedFonts = true;
			axisIntervalFormat.align="center";
			this[textName].setTextFormat(axisIntervalFormat)
		}
	}
}


Axis.prototype.setText = function(titleText:String,units:String) {
	trace ("set text");
	if (titleText == undefined){
		return;
	}
	this.titleText = titleText;
	if (units != undefined){
		this.units = units;
	}
	//this["axisText"].text = this.titleText;
	trace ("axis Title is " + titleText);
	this["axisText"].text = this.titleText + _root.convertUnits(this.units,"",true);
	this["axisText"].embedFonts = true;
	var axisTextFormat = new TextFormat();
	axisTextFormat.font = "axisFont";
	axisTextFormat.align = "left";
	this["axisText"].setTextFormat(axisTextFormat);
	this["axisText"].autoSize = true;;
}


Axis.prototype.onRollOver = function() {
	this.outline.col.setRGB(0xFF);
}

Axis.prototype.onRollOut = function() {
	if(!this.selected) {
    	this.outline.col.setRGB(this.outline.rgb);
  	}
}
Axis.prototype.setPointerVal = function(seqVal:Number) {
	trace("trace setPointerVal " + seqVal + ",start=" + this.start +",mult=" + this.mult);
	this.pointerVal = seqVal;
	return (this.pointerVal - this.start) / this.mult;
}
Axis.prototype.positionPointer = function(d:Number) {
	trace("trace positionPointer d=" + d + ",interval=" + this.interval);
	var pos = d / this.interval
	var i:Number = ((this.mult * d) + this.interval/2)/this.interval;
	trace("i="+ i + ",start=" + this.start);
	return (this.start + Math.floor(i));
}


Object.registerClass("axisSymbol", Axis);

#endinitclip