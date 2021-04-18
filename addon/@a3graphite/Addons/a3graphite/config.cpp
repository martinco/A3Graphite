class CfgPatches {
	class a3graphite {
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {};
		author[] = {"MartinCo"};
		authorUrl = "http://yaina.eu";
	};
};

class CfgFunctions {
	class a3graphite {
		class Common {
			class preInit { preInit = 1; file = "\a3graphite\preInit.sqf"; };
		};
	};
};