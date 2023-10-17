declare name "DjembeStrong";
declare description "Simple call of the Karplus-Strong model for the Faust physical modeling library";
declare license "MIT";
declare copyright "(c)Romain Michon, CCRMA (Stanford University), GRAME";

import("stdfaust.lib");

    freq = hslider("freq",0.5,0,1,0.01);
    gain = hslider("gain",0.5,0,1,0.01);
    damp = hslider("damp" ,0.01,0,1,0.01);
    posit = hslider("posit", 0.5, 0.1, 1, 0.01);
    sharp = hslider("sharp", 0.5, 0.1, 1, 0.01);
    gate = button("gate");

A = gate : (ba.impulsify * gain : 0.5 * pm.ks(freq, damp)) + (0.5 * pm.djembe(4000*freq, posit, sharp, gain, gate));

process = A <:_ , _;