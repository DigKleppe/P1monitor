/**
 * 
 */

function createDataTable(tableName, descriptorData) {
	var colls;
	var tblBody;
	var tbl = document.getElementById(tableName);
	var x = tbl.rows.length;
	if ( x == 0 ) // new table 
	{
		tblBody = document.createElement("tbody");
	}
	else {
		for (var r = 0; r < x; r++) 
			tbl.deleteRow(-1);
	}
	var rows = descriptorData.split("\r");

	for (var i = 0; i < rows.length ; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			colls = rows[i].split("=");
			for (var j = 0; j < colls.length; j++) {
				var cell = document.createElement("td");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		tblBody.appendChild(row);
	}
	tbl.appendChild(tblBody);
}

function updateDataTable (tableName, descriptorData) {
	var tbl = document.getElementById(tableName);
	var x = tbl.rows.length;
	if ( x == 0 ) // new table 
		createDataTable (tableName, descriptorData)
	else {
		var rows = descriptorData.split("\r");
		if ( rows.length == 1) {  // only header 
			colls = rows[0].split(",");
			tbl.rows[0].cells[1].innerHTML = colls[1];
		}
		else
			for (var i = 1; i < rows.length-1 ; i++) {   // update data fields
				colls = rows[i].split("=");
				tbl.rows[i].cells[1].innerHTML = colls[1];
			}
	}
}
	
	
	


