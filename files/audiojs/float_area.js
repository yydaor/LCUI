function GetCurrentStyle (obj, prop) {      
	if (obj.currentStyle) {         
	return obj.currentStyle[prop];      
	}       
	else if (window.getComputedStyle) {         
		propprop = prop.replace (/([A-Z])/g, "-$1");
		propprop = prop.toLowerCase ();
		return document.defaultView.getComputedStyle (obj,null)[prop];      
	}
	return null;    
}
function inc_pos () 
{
	var float_area = document.getElementById("float_area"); 
	var pos = parseInt(GetCurrentStyle(float_area, "right"));
	pos += 1;
	float_area.style.right = pos+'px';
}
function dec_pos () 
{
	var float_area = document.getElementById("float_area"); 
	var pos = parseInt(GetCurrentStyle(float_area, "right"));
	pos -= 1;
	float_area.style.right = pos+'px';
}
function btn_click(){
   var btn = document.getElementById("btnfold");
   btn.onclick = function() {
		var i;
		var float_area = document.getElementById("float_area"); 
		var pos = parseInt(GetCurrentStyle(float_area, "right"));
		if(pos < 0) { 
		  float_area.className = "float_area_2";
		  btn.className = "btnfold_2"; 
		} else { 
			float_area.className = "float_area_1";
			btn.className = "btnfold_1"; 
		}
   }
}
window.onload=function(){
  btn_click();
}