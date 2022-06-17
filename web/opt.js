const pageParams = new URLSearchParams(window.location.search);

Module.arguments = [
	"-data:bin/data",
	"-w:900", "-h:600",
	"-smap:2048",
	"-nwrk:0",
	"-showperf:0"
];

if (pageParams.get("small") !== null) {
	Module.arguments.push("-w:480");
	Module.arguments.push("-h:320");
}

if (pageParams.get("low") !== null) {
	Module.arguments.push("-smap:1024");
	Module.arguments.push("-nostgshadow:1");
} else if (pageParams.get("pseudoshd") !== null) {
	Module.arguments.push("-smap:-1");
	Module.arguments.push("-nostgshadow:1");
}

if (pageParams.get("vl") !== null) {
	Module.arguments.push("-vl:1");
}

if (pageParams.get("perf") !== null) {
	Module.arguments.push("-showperf:1");
}

if (pageParams.get("wperf") !== null) {
	Module.arguments.push("-showperf:1");
	Module.arguments.push("-exerep:5");
	Module.arguments.push("-speed:0.2");
}

var btns = document.getElementsByClassName("btn");
if (btns !== null) {
	const ua = navigator.userAgent;
	if (ua.match(/Android/i) || ua.match(/iPhone/i) || ua.match(/iPad/i)) {
		Array.prototype.filter.call(btns, function(btn) {
			btn.onmousedown = "";
			btn.onmouseup = "";
		});
	}
}
