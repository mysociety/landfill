// JavaScript Document

function addEvent(obj, evType, fn){
    if (obj.addEventListener){ 
        obj.addEventListener(evType, fn, false); 
        return true; 
    } else if (obj.attachEvent){ 
        var r = obj.attachEvent("on"+evType, fn); 
        return r; 
    } else { 
        return false; 
    }
}

addEvent(window, 'load', initialise);

function initialise(){
	if(document&&document.getElementById){
		var entryText = document.getElementById("reasons");
		if (entryText!=null)addEvent(entryText, 'focus', clearForm);
		var commentText = document.getElementById("commenttext");
		if (commentText!=null) addEvent(commentText, 'focus', clearComments);
	}
}

function checkEmail(form){
	var atplace = form.email.value.indexOf("@");
	var dotplace = form.email.value.lastIndexOf(".");
	if(form.email.value==""){
		alert ("please enter an e-mail address");
		return false;
	}else if(atplace<1 || dotplace <1 || dotplace < atplace){
		alert ("please enter a valid e-mail address");
		return false;
	}
	return true;
}

function clearForm() {
	clearText(document.getElementById("reasons"));
}

function clearComments(){
	clearText(document.getElementById("commenttext"));
}

function clearText(input){
	if (input.value=="Write your response..."||input.value=="Write your reasons here..."||input.value=="Write your message here..."){
		input.value = "";
	}
	input.style.background="#FFFFFF";
}

function growTextarea(id) {
    if (document && document.getElementById) {
        d = document.getElementById(id);
        if (id) d.rows += 4;
    }
}

function growTextarea(id) {
    if (document && document.getElementById) {
        d = document.getElementById(id);
        if (d) d.rows += 5;
        d = document.getElementById('growtext');
        if (d && d.innerHTML) d.innerHTML = 'Still want more space to write in?';
    }
}
