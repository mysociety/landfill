#initclip
//var axisColour:Number = 0xffffff;
// constructor for axis
function Pointer() {
	//this.createTextField("toolTip",this.getNextHighestDepth(),0,-50,250,70);
	//this["toolTip"].text="I am here " + xVal;
	this.colour=0x999999;
	this.dragged=false;
	super();
}

Pointer.prototype = new MovieClip();

Pointer.prototype.create = function(colour:Color,len:Number) {
	this.clear();
	trace ("create - pointer len " + this.len + ", xVal " + this.xVal + ":" + this.yVal + "," + colour + "(" + this.name + ")");
	if (len != undefined){
		this.len = len;
	}
 	if (colour != undefined){
		this.colour = colour;
	}
	this.draw(true,colour);
}

Pointer.prototype.draw = function(doFill:Boolean, colour) {
	this.clear();
	if (colour != undefined){
		this.colour = colour;
	}
	trace("pointer draw len=" + this.len + ",colour=" + this.colour);
	//trace("point - draw " + this.len + ", " + this.colour);
	if (doFill == true){
		this.beginFill(this.colour,100);
	}
	this.lineStyle(0, this.colour, 100);
	//this.drawCircle(this.len/2, -this.len/2,-this.len/2);
  	//this.moveTo(0,0);
  	//this.lineTo(0,this.len);
  	this.moveTo(0,0);
  	this.lineTo(-(this.len)/2,this.len/2);
  	//this.moveTo(0,0);
  	this.lineTo(this.len/2,this.len/2);
  	this.lineTo(0,0);
	if (doFill == true){
		this.endFill();
	}
}
Pointer.prototype.onRollOver = function() {
}

Pointer.prototype.onRollOut = function() {
	this["toolTip"]._visible = false;
	if(!this.selected) {
    	//this.outline.col.setRGB(this.outline.rgb);
  	}
}
Pointer.prototype.onRelease = function() {
}
//Pointer.prototype.isDragged = function() {
	//return (this.dragged == true)
//}

Object.registerClass("pointerSymbol", Pointer);

#endinitclip