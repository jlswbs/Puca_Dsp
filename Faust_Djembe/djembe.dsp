import("stdfaust.lib");

freq = hslider("freq", 130, 50, 800, 0.1);
strikePosition = hslider("posit", 0.5, 0.1, 1, 0.01);
strikeSharpness = hslider("sharp", 0.5, 0.1, 1, 0.01);
strikeGain = hslider("gain", 0.5, 0.1, 1, 0.01);
gate = button("gate");
A = pm.djembe(freq, strikePosition, strikeSharpness, strikeGain, gate);

process = A <:_ , _;
			