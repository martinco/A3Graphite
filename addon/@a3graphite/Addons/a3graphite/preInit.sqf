a3graphite = {
	params ["_metric", "_value"];
	"a3graphite" callExtension format["%1|%2", _metric, _value];
};