var _cols:Array = new Array(_root._colour1,_root._colour2,_root._colour3)
var _rangeColours:Array = new Array();

this.createEmptyMovieClip("shape_mc",1);
shape_mc.lineStyle(0,0x000000,100);
var colNum1 = Number("0x" + _root._colour1); 
var colNum2 = Number("0x" + _root._colour2); 
var colNum3 = Number("0x" + _root._colour3);
trace ("grad " + colNum1 + "," + colNum2 + "," + colNum3);
colours = [ Number("0x" + _root._colour3),Number("0x" + _root._colour2),Number("0x" + _root._colour1)];
alphas=[100,100,100];
ratios=[0,127.5,255];
matrix = {matrixType:"box",x:0,y:0,w:width,h:height,r:Math.PI/2};
shape_mc.beginGradientFill("linear",colours,alphas,ratios,matrix);
//trace("filling " + col1);

shape_mc.moveTo(0,0);
shape_mc.lineTo(width,0);
shape_mc.lineTo(width,height);
shape_mc.lineTo(0,height);
shape_mc.lineTo(0,0);
shape_mc.endFill();
createTextField("minText", this.getNextHighestDepth(),width + 1,height - 15,0,0);
createTextField("maxText", this.getNextHighestDepth(),width + 1,0,0,0);
maxText.autoSize = true;
minText.autoSize = true;
setMaxMin(min,max);


function setMaxMin(minVal:Number,maxVal:Number){
	min = minVal;
	max = maxVal
	trace("setMaxMin " + min + ":" + max);
	this["minText"].text = min;
	this["maxText"].text = max
	delete _rangeColours;
	_rangeColours = new Array()
	trace("initialised " + _rangeColours.length);
}
function getColour(num:Number){

	//trace("getColour1 number=" + num);
//	num = (num < 50?num * 2:(num -50) * 2);
	if (num == null){
		//return 0x000000;
		trace("return default colour");
		return _root._defaultColour;
	}
	num = (num - min)/(max - min);
	trace("getColour num=" + num +",min=" + min + ",max=" + max);
	
	//trace("getColour1 num=" + num + ",numColours=" + numColours + ",numRanges=" + _root._numRanges);
	var rangeUnit = 1 / _root._numRanges;
	var temp = num == 1? _root._numRanges - 1 : num/rangeUnit;
	var rangeNum = Math.floor(temp);
	trace("checking colour rangeNum=" + rangeNum + ",colour=" + _rangeColours[rangeNum] + ",length=" + _rangeColours.length);
	if (rangeNum >= _root._numRanges - 1){
		trace("Alert " + rangeNum)
	}
	if (_rangeColours[rangeNum] != undefined){
		trace("returning pre-selected");
		return _rangeColours[rangeNum];
	}
	trace("calculating colour");

	var numColours = _cols.length;
	var colourUnit = (numColours -1) / (_root._numRanges -1);
	var colourNum = rangeNum * colourUnit;
	
	var col = Math.floor(colourNum);
	var weight = colourNum - col;
	//trace ("getColour1 colourUnit=" + colourUnit + ",rangeNum=" + rangeNum + ",colourNum=" + colourNum + ",col=" + col + ",weight=" + weight);
	var col:Number = doBlend(col,colourNum - col);
	_rangeColours[rangeNum] = col;
	trace("setting colour rangeNum=" + rangeNum + ",col=" + col + ",value=" + _rangeColours[rangeNum] +",length=" + _rangeColours.length);
	return col;
		
}


function doBlend(colour:Number,weight:Number){
	//trace("doblend " + ",colour=" + colour + ",weight=" + weight);
	
	var alpha = 256 - weight * 256;
	if (colour >= _cols.length - 1){
		return Number("0x"  + _cols[_cols.length -1]);
	}
							
	var col1 = _cols[colour];
	var col2 = _cols[colour + 1];
	//trace("doblend " + ",col1=" + col1 + ",col2=" + col2 + ",alpha=" + alpha);
	var nr = (Number("0x" + col1.substr(0,2)) * alpha +
		 	Number("0x" + col2.substr(0,2)) * (256 - alpha))/256;
	var ng = (Number("0x" + col1.substr(2,2)) * alpha +
			Number("0x" + col2.substr(2,2)) * (256 - alpha))/256;
	var nb = (Number("0x" + col1.substr(4,2)) * alpha +
		Number("0x" + col2.substr(4,2)) * (256 - alpha))/256;
	//trace("colours " + nr + "," + ng + "," + nb);
	nr = Math.round(nr,0) * 256 * 256;
	ng = Math.round(ng,0) * 256;
	nb = Math.round(nb,0);
	var res1:Number = (nr + ng + nb) ;
	return res1;
}