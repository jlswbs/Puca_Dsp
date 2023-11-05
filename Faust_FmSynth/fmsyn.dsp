declare name "fmsyn";
declare description "simple 2op fm synth";
declare license "MIT";
declare copyright "(c)Romain Michon, CCRMA (Stanford University), GRAME";

import("stdfaust.lib");

gate = button("gate");
freq = hslider("freq", 440, 20, 4000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);
volA = hslider("enva", 0.1, 0, 1, 0.01);
volD = hslider("envd", 0.5, 0, 1, 0.01);
volS = hslider("envs", 0.5, 0, 1, 0.01);
volR = hslider("envr", 1, 0, 2, 0.01);

// modwheel:
feedb = (freq-1) * (hslider("feed", 0, 0, 1, 0.001) : si.smoo);
modFreqRatio = hslider("shape",1,0,4,0.01) : si.smoo;

envelop = en.adsre(volA,volD,volS,volR,gate);

// modulator frequency
modFreq = freq*modFreqRatio;

// modulation index
lfo = (hslider("lfo",0,0,1,0.01));
FMdepth = envelop * 1000 * (1+lfo);

FMfeedback(frq) = (+(_,frq):os.osci) ~ (*(feedb));
FMall(f) = envelop * os.osci(f + (FMdepth*FMfeedback(f*modFreqRatio)));

A = gain * (0.5 * FMall(freq));
process = A <:_ , _;