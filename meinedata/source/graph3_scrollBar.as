

this.colour=0x999999;

if (len == undefined){
	//var len:Number = 200;
	len = 200;
}
if (width == undefined){
	//var len:Number = 200;
	width = 10;
}
trace("boxLen=" + boxLen + "name=" + opacity);
if (boxLen == undefined){
	//var len:Number = 200;
	boxLen = 10;
}
if (buttonLen == undefined){
	//var len:Number = 200;
	buttonLen = boxLen;
}
if (smallClick == undefined){
	//var len:Number = 200;
	smallClick = 10;
}
if (bigClick == undefined){
	//var len:Number = 200;
	bigClick = 50;
}
if (scrollStyle == undefined){
	scrollStyle = "standard";
}
var itemHeight:Number = 20;
var darkColour:Number = 0x999999;
var lightColour:Number = 0xCCCCCC;
//var checkBoxListener:Object = new Object();


this.scrollStart = boxLen;
this.scrollEnd = len - (boxLen + buttonLen);
trace("style=" + scrollStyle);
if (scrollStyle == "standard"){
	this.beginFill(lightColour,100);
	this.lineStyle(0, darkColour, 100);
	this.moveTo(0,0);
	this.lineTo(0,this.boxLen);
	this.lineTo(this.width,this.boxLen);
	this.lineTo(this.width,0);
	this.lineTo(0,0);
	
	var temp:Number = this.len - this.boxLen;
	this.moveTo(0,temp);
	this.lineTo(0,this.len);
	this.lineTo(this.width,this.len);
	this.lineTo(this.width,temp);
	this.lineTo(0,temp);
	
	this.moveTo(0,boxLen);
	this.lineTo(0,temp);
	this.moveTo(this.width,boxLen);
	this.lineTo(this.width,temp);
	this.endFill();

	var temp1 = 4;
	var temp2 = 5;
	this.beginFill(0x000000,100);	
	this.moveTo(this.width/2,this.boxLen/temp2);
	this.lineTo(this.width/temp2, temp1 * this.boxLen/temp2);
	this.lineTo(temp1 * this.width/temp2, temp1 * this.boxLen/temp2);
	this.lineTo(this.width/2, this.boxLen/temp2);
	
	this.moveTo(this.width/2,this.len - this.boxLen/temp2);
	this.lineTo(this.width/temp2, this.len - (temp1 * this.boxLen/temp2));
	this.lineTo(temp1 * this.width/temp2, this.len - (temp1 * this.boxLen/temp2));
	this.lineTo(this.width/2, this.len - this.boxLen/temp2);
	this.endFill();	
	
	
} else if (scrollStyle == "meter"){
	darkColour = 0x000000;
	this.lineStyle(2, darkColour, 100);
	this.moveTo(width,0);
	this.lineTo(2 * width/3,0);
	this.lineTo(2 * width/3,this.len);
	this.lineTo(width,len);
	this.moveTo(2 *width/3,len/2);
	this.lineTo(width,len/2);
	//this.lineTo(this.width,this.boxLen);
	//this.lineTo(this.width,0);
	//this.lineTo(0,0);
  	
	var temp:Number = this.len - this.boxLen;
/*
	trace("startLabel="+ startLabel +",endLabel="+ endLabel + ",vertical=" + vertical);
	if (startLabel == true){
		if (vertical == true){
			createTextField("startLabel_txt",this.getNextHighestDepth(),width,0,0,0);
		} else {
			createTextField("startLabel_txt",this.getNextHighestDepth(),0,width,0,0);
		}
		this.startLabel_txt.autoSize = true;
	}
	if (endLabel == true){
		if (vertical == true){
			createTextField("startLabel_txt",this.getNextHighestDepth(),width,len,0,0);
		} else {
			createTextField("startLabel_txt",this.getNextHighestDepth(),len,width,0,0);
		}
		this.startLabel_txt.autoSize = true;
	}
*/
}

	

this.createEmptyMovieClip("scrollBarButton_mc",this.getNextHighestDepth());
if (scrollStyle == "standard"){
	this.scrollBarButton_mc.beginFill(this.darkColour,100);
	this.scrollBarButton_mc.lineStyle(0, this.darkColour, 100);
	this.scrollBarButton_mc.moveTo(0,0);
	this.scrollBarButton_mc.lineTo(0,this.boxLen);
	this.scrollBarButton_mc.lineTo(this.width,this.boxLen);
	this.scrollBarButton_mc.lineTo(this.width,0);
	this.scrollBarButton_mc.lineTo(0,0);
	this.scrollBarButton_mc.endFill();
} else if (scrollStyle == "meter"){
	this.scrollBarButton_mc.lineStyle(0, this.darkColour, 100);
	//this.scrollBarButton_mc.moveTo(0,0);
	//this.scrollBarButton_mc.lineTo(2 * width/3,0);
	this.scrollBarButton_mc.moveTo(2 * width/3,0);
	this.scrollBarButton_mc.beginFill(this.darkColour,100);
	this.scrollBarButton_mc.lineTo(width/4,-width/3);
	this.scrollBarButton_mc.lineTo(width/4,width/3);
	this.scrollBarButton_mc.lineTo(2* width/3,0);
	this.scrollBarButton_mc.endFill();
}
this.scrollBarButton_mc._x = 0;
trace("scrollStart=" + scrollStart + "," + boxLen);
this.scrollBarButton_mc._y = this.scrollStart;
this.scrollBarButton_mc.dragging = false;
//var scrollMmouseListener:Object = new Object();
//Mouse.addListener(scrollMouseListener);



//trace("dragging is " + this.scrollBarButton_mc.dragging);
//this.createClassObject(mx.controls.CheckBox, "test1" + i,this.getNextHighestDepth(),{_x:0,_y:0,label:"SPECIAL ONE"});

//trace("itemList mpList is (" + _root._mpList.length +")"  + _root._mpList);
//checkBoxListener.click = function() {
function checkBoxListener(evt_obj:Object) {
	//trace("checkBoxListener");
	//var nme = evt_obj.target.name;
	var num = evt_obj.target._y / itemHeight;
	num-=(items_mc._y /itemHeight);
	var nme:String = _root._mpList[num].data;	
	//trace("nme is " + nme + ",y=" + y  +",num=" + num );
	if (evt_obj.target.selected == true){
		selectedList[nme] = true;
		selectedCount++;
		//trace("added " + nme + "," + selectedList[nme]);
	}else{
		selectedList[nme] == false;
		if (selectedList[nme] != null){
			selectedCount--;
			delete selectedList[nme]; 
			//trace("deleted " + nme + "," + selectedList[nme]);
		}
	}
	//trace("selected count is " + selectedCount);
	_root.doPoints();
}

this.scrollBarButton_mc.onPress = function(){
	//trace("listItem button scroll " + this.dragging + "," + this._parent.scrollStart + "," + this._parent.scrollEnd);
	this.dragging = true;
	this.startDrag(
					true,0,this._parent.scrollStart,0,this._parent.scrollEnd);
}
this.scrollBarButton_mc.onRelease = function(){
	//trace("listItem button release ");
	this.stopDrag();
	this.dragging = false;
	setItems(this._parent._ymouse,true,true);

}
this.scrollBarButton_mc.onReleaseOutside = function(){
	trace("listItem button outside release ");
	this.onRelease();
	//this.dragging = false;
}
this.onMouseMove = function(){
	//trace("moving " + this.scrollBarButton_mc.dragging);
	if (this.scrollBarButton_mc.dragging == true && this._ymouse >= this.scrollStart & this._ymouse <= this.scrollEnd){
		//trace("scrolling y=" + this._ymouse + this.scrollStart + "," + this.scrollEnd);
		setItems(this._ymouse,true);
	}
}
this.onMouseUp = function(){
	//trace("mouse up");
	this.clicking = false;
}
this.onMouseDown = function(){
	//trace("new scrollbar mouse down " + this.scrollBarButton_mc.dragging );
	if (this.hitTest(_root._xmouse,_root._ymouse) != true){
		//trace("new not on scrollbar ");
		return;
	}
	//trace("on new scrollbar " + this.scrollBarButton_mc.dragging);
	if (this.scrollBarButton_mc.hitTest(_root._xmouse,_root._ymouse) == true){
		//trace("on button");
	} else {
		//trace("not on button");
		this.doClick(this._ymouse);
		this.clicking = true;
		this.clickDelay=12;
	}
}
/*
this.onMouseUp = function(){
	trace("new scrollbar mouse down " + this.scrollBarButton_mc.dragging );
	if (this.hitTest(_root._xmouse,_root._ymouse) != true){
		trace("new not on scrollbar ");
		return;
	}
	trace("on new scrollbar " + this.scrollBarButton_mc.dragging);
	if (this.scrollBarButton_mc.hitTest(_root._xmouse,_root._ymouse) == true){
		trace("on button");
	} else {
		trace("not on button");
		this.doClick(this._ymouse);
	}
}
*/


function doClick(d:Number){
	trace("new doClick " + d);
	if (d < boxLen)
	{
		//trace(this.scrollBarButton_mc._y);
		if (this.scrollBarButton_mc._y > boxLen + smallClick){
			this.scrollBarButton_mc._y-= smallClick;
		} else { 
			this.scrollBarButton_mc._y = boxLen;
			this.clicking = false;
		}
	} else if (d > len - 2 * boxLen){
		if (this.scrollBarButton_mc._y < (len - 2 * boxLen - smallClick)){
			this.scrollBarButton_mc._y+=smallClick;
		} else { 
			this.scrollBarButton_mc._y = len - 2 * boxLen;
			this.clicking = false;
		}
		//trace(this.scrollBarButton_mc._y);
	} else if (d <  this.scrollBarButton_mc._y){
		if (this.scrollBarButton_mc._y >(boxLen + bigClick )){
			this.scrollBarButton_mc._y-=bigClick;
		} else { 
			this.scrollBarButton_mc._y = boxLen;
			this.clicking = false;
		}
		//trace(this.scrollBarButton_mc._y);
	} else if (d > this.scrollBarButton_mc._y ){
		if (this.scrollBarButton_mc._y < (len - 2 * boxLen - bigClick)){
			this.scrollBarButton_mc._y+=bigClick;
		} else { 
			this.scrollBarButton_mc._y = len - 2 * boxLen;
			this.clicking = false;
		}
	}
		//trace(this.scrollBarButton_mc._y);
	setItems(this.scrollBarButton_mc._y,true);
}

function setItems (pos:Number,round:Boolean,finish:Boolean){
	//trace("setItems " + items_mc._y);
	var range:Number = len - 3 * boxLen;
	var d:Number = pos - boxLen;
	if (d < 0 ){
		d = 0;
	}
	if (d > range ){
		d = range;
	}
		
	_parent.handleScroll(d,range,round,name,finish);
}

onEnterFrame = function() {
	if (this.clicking == true && this.clickDelay-- == 0){
		this.doClick(this._ymouse);
		this.clickDelay = 2;
	}
}
