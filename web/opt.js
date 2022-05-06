const pageParams = new URLSearchParams(window.location.search);

Module.arguments = [
	"-data:bin/data",
	"-w:900", "-h:600",
	"-smap:2048",
	"-nwrk:0"
];

if (pageParams.get("small") !== null) {
	Module.arguments.push("-w:480");
	Module.arguments.push("-h:320");
}

if (pageParams.get("vl") !== null) {
	Module.arguments.push("-vl:1");
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
	if (ua.match(/Android/i) || ua.match(/iPhone/i)) {
		Array.prototype.filter.call(btns, function(btn) {
			btn.style.fontSize = "20pt";
			btn.style.padding = "0 30px";
		});
	}
}
