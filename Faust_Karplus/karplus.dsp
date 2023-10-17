declare name "KarplusStrong";
declare description "Simple call of the Karplus-Strong model for the Faust physical modeling library";
declare license "MIT";
declare copyright "(c)Romain Michon, CCRMA (Stanford University), GRAME";

import("stdfaust.lib");

    freq = hslider("freq",1.0,0,2,0.01);
    gain = hslider("gain",0.5,0,1,0.01);
    damp = hslider("damp" ,0.01,0,1,0.01);
    gate = button("gate");

A = gate : pm.impulseExcitation * gain : 0.5 * pm.ks(freq , damp);

process = A <:_ , _;