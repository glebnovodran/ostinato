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


