
var MINUTESPERTICK = 5;// log interval dayslog
var LOGDAYS = 1;
var MAXDAYPOINTS = LOGDAYS * 24 * 60 / MINUTESPERTICK;
var MAXHOURPOINTS = 60*60; // seconds interval 

var TABLEROWPWR			= 1;
var TABLEROWVOLTAGE		= 2;

var dayChartData;
var dayChart;
var hourChartData;
var hourChart;

var chartRdy = false;
var tick = 0;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = MINUTESPERTICK * 60;
var simMssgCnts = 0;
var lastTimeStamp = 0;
var energyToday =0;

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];
var chartOptions = {
	title: '',
//	curveType: 'function',
//	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
//		1: { logScale: false },
	},

	series: {
		0: { targetAxisIndex: 0 },  // power
//		1: { targetAxisIndex: 1 },	// voltage
	},
};
var chartOptionsSolarPanels = {
	title: '',
//	curveType: 'function',
//	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
//		1: { logScale: false },
	},

	series: {
		0: { targetAxisIndex: 0 },  // power
		1: { targetAxisIndex: 1 },	// deliverd power
	},
};



function sim() {
	var res;
	simValue1 += 0.1;
	simValue2 = Math.sin(simValue1) * 100;
	res = simValue1.toFixed(2) + ","  + simValue2.toFixed(2) +";";
	return res;
}


function simplot() {
	var str2 = "";
	for (var n = 0; n < 100; n++) {
		str2 +=	sim();
	}
	str2 = str2+ "\r";
	plotHourArray(str2);
	plotDayArray(str2);

//	for (var m = 1; m < NRItems; m++) { // time not used for now 
//		var value = simValue2; // from string to float
//		document.getElementById(displayNames[m]).innerHTML = value.toFixed(2);
//	}
}


function clear() {
	hourChartData.removeRows(0, hourChartData.getNumberOfRows());
	chart.draw(hourChartData, options);
	tick = 0;
}

//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5

function plotPoint(chartData,maxPoints, channel, value) {
	if (chartRdy) {
		if (channel == 0) {
			chartData.addRow();
			if (chartData.getNumberOfRows() > maxPoints == true)
				chartData.removeRows(0, chartData.getNumberOfRows() - maxPoints);
		}
		value = parseFloat(value); // from string to float
		chartData.setValue(chartData.getNumberOfRows() - 1, channel+1, value);
	}
}

// 101, 2345\r\n" 
function plotRTvalue (chartData,maxpoints, str) {
	var cols = str.split(",");

	if ( SOLARPANELS) {
		for (var i = 0; i < cols .length ; i++) 	
			plotPoint( chartData,maxpoints, i, cols[i]);
	}
	else {
		for (var i = 0; i < 1 ; i++) { // only power
			plotPoint( chartData,maxpoints, i, cols[i]);
		}
	}
}

function initCharts() {
	dayChart = new google.visualization.LineChart(document.getElementById('dayChart'));
	dayChartData = new google.visualization.DataTable();
	dayChartData.addColumn('string', 'Tijd');
	dayChartData.addColumn('number', 'Verbruik (W)');
	if (SOLARPANELS)
		dayChartData.addColumn('number', 'Teruggeleverd (W)');

	hourChart = new google.visualization.LineChart(document.getElementById('hourChart'));
	hourChartData = new google.visualization.DataTable();
	hourChartData.addColumn('string', 'Tijd');
	hourChartData.addColumn('number', 'Verbruik (W)');
	if (SOLARPANELS)
		hourChartData.addColumn('number', 'Teruggeleverd (W)');
	chartRdy = true;

}
function startReqTimer() {
	var str;
	initCharts();
	
if ( !SIMULATE)	{
	str = getItem ("getHourLogValues");
	plotHourArray(str);
	str = getItem ("getDayLogValues");
	plotDayArray(str);
}
	else
		simplot();
		
	setInterval(function() { timer() }, 1000);	
}

var firstRequest = true;
var rows = 0;

function updateLastDayTimeLabel(data) {
	var ms = Date.now();
	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);
}


function updateAllDayTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var minutesAgo = rows * MINUTESPERTICK;
	var ms = Date.now();
	ms -= (minutesAgo * 60 * 1000);
	for (var n = 0; n < rows; n++) {
		var date = new Date(ms);
		var labelText = dayNames[date.getDay()] + ';' + date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 60 * 1000 * MINUTESPERTICK;

	}
}

// "100,230.0;100,230.0\r"  // averaged values over 5 mimutes for 24hr


function plotDayArray( str) {
	var arr2 = str.split(";");
	var nrPoints = arr2.length - 1;

	let now = new Date();
	var hours = now.getHours();
	var minutes = now.getMinutes();
	var samplesToday = hours * (60/MINUTESPERTICK) + minutes / MINUTESPERTICK;  
	startSample = nrPoints - samplesToday;
	energyToday = 0; 

	for (var p = 0; p < nrPoints; p++) {
		plotRTvalue( dayChartData,MAXDAYPOINTS, arr2[p]);
		if (p >= startSample ) {
			var ar = arr2[p].split(",");
			energyToday += parseFloat(ar[0]); 
		}
	}
	
	energyToday = energyToday/ (60/MINUTESPERTICK); // make WH

	if (nrPoints == 1) { // then single point added 
		updateLastDayTimeLabel(dayChartData);
	}
	else {
		updateAllDayTimeLabels(dayChartData);
	}
//	dayChart.draw(dayChartData, google.charts.Line.convertOptions(chartOptions));
	if (SOLARPANELS)
		dayChart.draw(dayChartData, chartOptionsSolarPanels);
	else	
		dayChart.draw(dayChartData, chartOptions);
}

function updateLastHourTimeLabel(data) {
	var ms = Date.now();
	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);
}


function updateAllHourTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var ms = Date.now();
//	ms -= (minutesAgo * 60 * 1000);
	ms -= (rows * 1000);  // go back in time rows seconds
	for (var n = 0; n < rows ; n++) {
		var date = new Date(ms);
		var labelText = date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 1000;
	}
}

// "100,230.0;100,230.0\r"  // reat time values in 1 second interval over 1 hour
function plotHourArray( str) {
	var arr2 = str.split(";");
	var nrPoints = arr2.length - 1;

	for (var p = 0; p < nrPoints; p++) {
		plotRTvalue( hourChartData, MAXHOURPOINTS, arr2[p]);	
	}
	if (nrPoints == 1) { // then single point added 
		updateLastHourTimeLabel(hourChartData);
	}
	else {
		updateAllHourTimeLabels(hourChartData);
	}
	//hourChart.draw(hourChartData, google.charts.Line.convertOptions(chartOptions));
	
	if (SOLARPANELS)
		hourChart.draw(hourChartData, chartOptionsSolarPanels);
	else	
		dayChart.draw(dayChartData, chartOptions);
}

var accumulatedPwr = 0;
var accumulatedDeliveredPwr = 0;
var accumulatedVoltage = 0;

function timer() {
	var str;
	var deliveredPwr = 0;
//	str = getRTMeasValues();
	if ( SIMULATE)
		str = simStr;
	else	
		str = getItem ("getInfoValues");  // all items 

	updateDataTable( "infoTable", str);
	// data for actual power and voltage in table

	var tbl = document.getElementById("infoTable");
	var s1 = tbl.rows[TABLEROWPWR ].cells[1].innerHTML; // 137 W
	var arr = s1.split(" ");
	var pwr = parseFloat(arr[0]);
		
	if (SOLARPANELS) { //  and 3 phases	
		s1 = tbl.rows[(TABLEROWPWR +1)].cells[1].innerHTML;
		arr = s1.split(" ");
		pwr += parseFloat(arr[0]);
		s1 = tbl.rows[(TABLEROWPWR +2)].cells[1].innerHTML;
		arr = s1.split(" ");
		pwr += parseFloat(arr[0]);
		
		s1 = tbl.rows[(TABLEROWPWR +3)].cells[1].innerHTML;  // delivered  L1
		arr = s1.split(" ");
	    deliveredPwr = parseFloat(arr[0]);
	    
	    s1 = tbl.rows[(TABLEROWPWR +4)].cells[1].innerHTML;  // delivered L2
		arr = s1.split(" ");
	    deliveredPwr += parseFloat(arr[0]);
	    
	    s1 = tbl.rows[(TABLEROWPWR +5)].cells[1].innerHTML;  // delivered L3
		arr = s1.split(" ");
	    deliveredPwr += parseFloat(arr[0]);
	    accumulatedDeliveredPwr += deliveredPwr;
	    
	    energyToday += (pwr-deliveredPwr) /3600; // add energy for this second
	}
	else
		energyToday += pwr/3600; // add energy for this second
	
	accumulatedPwr += pwr;
		
	str = "Verbruik vandaag:," + (energyToday/1000).toFixed(3) + " kWh";
	updateDataTable( "headTable", str);
	
	str = arr[0];
	s1 = tbl.rows[TABLEROWVOLTAGE ].cells[1].innerHTML; // 230.5*W
	arr = s1.split("*");
	accumulatedVoltage += parseFloat(arr[0]);
	
	str = pwr.toFixed(1) + ',' + deliveredPwr.toFixed(1) +';';
	plotHourArray(str);
					
	presc--;
//	presc = 0;
	if (presc == 0){
		presc = MINUTESPERTICK * 60;
		accumulatedPwr /= (MINUTESPERTICK * 60);
		accumulatedDeliveredPwr /= (MINUTESPERTICK * 60);
		str = accumulatedPwr.toFixed(1) +',' +  accumulatedDeliveredPwr.toFixed(1) +';';
		accumulatedPwr = 0;
		accumulatedDeliveredPwr = 0;
		plotDayArray(str);
	}
}	




