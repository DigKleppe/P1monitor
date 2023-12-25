var SIMULATE = false;
var SOLARPANELS = true;

var data2 = 0;
var simValue1 = 0;
var simValue2 = 0;

var ACTCOLLUMN = 1;
var OFFSETCOLLUN = 2;
var TEMPINFOROW = 1;
var RHINFOROW = 2;
var CO2INFOROW = 3;

//var simStr = "Onderdeel,Waarde\n\rTijdstempel=231028112841S\n\rEnergie tarief 1=535.356*kWh\n\rEnergie tarief 2=535.783*kWh\n\rActueel vermogen=65*W\n\rKorte onderbrekingen=12\n\rLange onderbrekingen=4\n\rOnderbrekingslog=2 210827121443 10761 210";

var simStr = "Onderdeel,Waarde\rActief vermogen=137*W\rVoltage=233.6*V\rEnergie tarief 1=539.224*kWh\rEnergie tarief 2=542.108*kWh\rKorte onderbrekingen=12\rLange onderbrekingen=4\rOnderbrekingslog=2 210827121443 10761 21\r"


	
function sendItem(item) {
	console.log("sendItem: " + item);
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("POST", "/upload/cgi-bin/", false);
	req.send(item);
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
		}
	}
}

function getItem(item) {
	console.log("getItem " + item);
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/" + item, false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
		}
	}
}
// returns array with accumulated momentary values
function getLogMeasValues() {
	console.log("getLogMeasValues");
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getLogMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}

// returns array minute  averaged values
function getAvgMeasValues() {
	console.log("getAvgMeasValues");
	if (SIMULATE)
		return "OK";

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getAvgMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}

// returns array with momentarty  values
function getRTMeasValues() {
	console.log("getRTMeasValues");
	if (SIMULATE)
		return simStr;

	var str;
	var req = new XMLHttpRequest();
	req.open("GET", "cgi-bin/getRTMeasValues", false);
	req.send();
	if (req.readyState == 4) {
		if (req.status == 200) {
			str = req.responseText.toString();
			return str;
			//	var arr = str.split(",");
			//	return arr;
		}
	}
}
