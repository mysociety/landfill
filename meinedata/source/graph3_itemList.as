this.colour=0x999999;
var selectedCount = 0;
var selectedList:Object = new Object();

//trace("selectedCnt=" + selectedCnt);
if (colour != undefined){
	this.colour = colour;
}



var scrollWidth:Number = 10;
var scrollPos:Number = 180;
var itemHeight:Number = 20;
var len:Number = 200;
var boxLen:Number = 10;

// should get more from thescrollbar ?
var smallClick = (len - 3 * boxLen) / (_root._itemList.length - (len / itemHeight)); 
var bigClick = smallClick * (len / itemHeight); 
trace ("itemList=" + _root._itemList.length + ",len=" + len + ",itemheight=" + itemHeight);
trace ("smallClick=" + smallClick +",bigClick=" + bigClick);
this.scrollBarV_mc = this.attachMovie("ScrollBarSymbol","scrollV",this.getNextHighestDepth(),{_x:scrollPos,_y:0,len:len,width:scrollWidth,smallClick:smallClick,bigClick:bigClick,name:"vert"});
this.scrollBarH_mc = this.attachMovie("ScrollBarSymbol","scrollH",this.getNextHighestDepth(),{_x:0,_y:len+scrollWidth,len:scrollPos,width:scrollWidth,name:"horiz",smallClick:10,bigClick:50});
this.scrollBarH_mc._rotation = -90;
var margin = 5;
var horizScrollWidth = 10;
var labHeight=25;
this.lineStyle(2, 0x000000, 100);
this.moveTo(0,0);
this.lineTo(scrollPos + scrollWidth + margin,0);
this.lineTo(scrollPos + scrollWidth + margin,len + margin + horizScrollWidth + labHeight);
this.lineTo(0,len + margin + horizScrollWidth + labHeight);
this.lineTo(0,0);
this.moveTo(0,len + margin + horizScrollWidth);
this.lineTo(scrollPos + scrollWidth + margin,len + margin + horizScrollWidth);
createClassObject(mx.controls.Button,"clear_btn",this.getNextHighestDepth(),{_x:0,_y:len+margin + 1 + horizScrollWidth,label:_root._clearButtonString,_width:80});


//trace("itemList mpList is (" + _root._mpList.length +")"  + _root._mpList);
createEmptyMovieClip("items_mc",this.getNextHighestDepth());
//checkBoxListener.click = function() {
function checkBoxListener(evt_obj:Object) {
	//trace("checkBoxListener");
	//var nme = evt_obj.target.name;
	var num = evt_obj.target._y / itemHeight;
	num-=(items_mc._y /itemHeight);
	var nme:String = _root._itemList[num].data;	
	if (evt_obj.target.selected == true){
		selectedList[nme] = true;
		selectedCount++;
		trace("added " + nme + "," + selectedList[nme] + ",selectedCount=" + selectedCount);
	}else{
		selectedList[nme] == false;
		if (selectedList[nme] != null){
			selectedCount--;
			delete selectedList[nme]; 
			//trace("deleted " + nme + "," + selectedList[nme]);
		}
	}
	//trace("selected count is " + selectedCount);
	//_root.doPoints();
	_root.updatePoints();
}


var numCh = len / itemHeight;
var xText:Number = 25;
var xCh:Number = 5;

var maxLen =0;
for (var i = 0; i< _root._itemList.length ; i++){
	//trace ("in loop i=" + i);
	var y:Number = 0 + (itemHeight * i);
	//trace("doing item xText=" + xText + ",xCh=" + xCh + ":" + y + "," + _root._itemList[i].label + ":" + _root._itemList[i].data);
	var vis:Boolean = true;
	//if (i < 50){
		//vis = true;
	//}
	var nme = _root._itemList[i].data;
	//var nme1 = _root._mpList[i].label;
	if ( i < numCh){
		this.createClassObject(mx.controls.CheckBox, "ch_" + i,this.getNextHighestDepth(),{_x:xCh,_y:y,label:_root._itemList[i].label,_visible:vis});
		this["ch_" + i].setSize(0,19);
		//trace ("trace created checkbox nme=" + nme + ",label=" + this.items_mc[nme].label + ",data=" + this.items_mc[nme].data);
		this["ch_" + i].addEventListener("click", this.checkBoxListener);
	}
	
	this.items_mc.createTextField("text_"+i,this.items_mc.getNextHighestDepth(),xText,y-1,scrollPos,itemHeight);
	this.items_mc["text_" + i].text = _root._itemList[i].data;
	this.items_mc["text_" + i].autoSize = true;
	if (items_mc["text_" + i]._width > maxLen){
		maxLen = items_mc["text_" + i]._width;
	}
	
	
	//this.createTextField("axisText", this.getNextHighestDepth(),xTitle,yTitle,_root._titleWidth,_root._titleHeight);

	//this.items_mc.createClassObject(mx.controls.CheckBox, nme,this.items_mc.getNextHighestDepth(),{_x:x,_y:y,label:_root._mpList[i].label,_visible:vis});
}
trace("maxLen=" + maxLen);	

createEmptyMovieClip("mask_mc",this.getNextHighestDepth());
this.mask_mc.beginFill(0xff0000,100);
this.mask_mc.lineStyle(0, 0xff0000, 100);
var tempLen = len;
trace ("mask len="+ tempLen + "scrollPos=" + scrollPos);
this.mask_mc.moveTo(xCh,0);
this.mask_mc.lineTo(xCh,tempLen);
this.mask_mc.lineTo(scrollPos,tempLen);
this.mask_mc.lineTo(scrollPos,0);
this.mask_mc.lineTo(xCh,0);
this.mask_mc.endFill(lightColour,100);
this.mask_mc._x =0;
this.mask_mc._y =0;
this.items_mc.setMask(this.mask_mc);


function handleScroll(d:Number,range:Number,round:Boolean,name:String,finish:Boolean){
	if (name == "horiz"){
		items_mc._x = -((maxLen - (scrollPos - xCh - 20)) * (d / range));
	} else {
		var listLen = _root._itemList.length * itemHeight;
		var y = (listLen - len) * d/range;
		//trace ("y=" + y + ",range=" + range + ",d=" + d + ",pos=" + pos + "listLen=" + listLen);
		if (round == true){
			var temp = Math.round(y /itemHeight);
			y = temp * itemHeight;
		}
		items_mc._y  = -y;
		for (i=0 ; i < len / itemHeight;i++){
			var pos1 = i - (items_mc._y / itemHeight);
			var nme:String = _root._itemList[pos1].data;
			//trace("doing checks i=" + i + ",y=" + items_mc._y + ",pos=" + pos1 +",nme=" + nme + ",item=" +selectedList[nme]);
			if (selectedList[nme] == true){
				this["ch_" + i].selected = true;
			} else {
				this["ch_" + i].selected = false;
			}
		}
	}
}

this.clear_btn.onRelease = function(){
	//trace("clear pressed");
	for (var nme in this._parent.selectedList){
		//trace("in loop " + nme);
		//this._parent.items_mc[nme].selected = false;
		this._parent.selectedList[nme] = false;
		delete this._parent.selectedList[nme];
	}
	for (var i = 0;i < len/itemHeight;i++){
		this._parent["ch_" + i].selected = false;
	}
	selectedCount = 0;
	//_root.doPoints();
	_root.updatePoints();
}


items_mc.onPress = function(){
	trace ("press " + this._xmouse +"," + this._ymouse);
}




