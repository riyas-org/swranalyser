# swranalyser
A simple swr analyser based on ad9850 dds

This project uses a simple ad9850 module as its heart to generate the rf signals for the sweep. The controller is an arduino promini (with an atmega328 running on 3.3volt) and for display, it uses an ili9341 tft.

The core circuitry for the analyser is very simple and is based on the design from Beriks (K6BEZ) simple antenna analyser (pdf). I added a buffer with the dds  (as shown on W5DOR).

Pictures at : http://blog.riyas.org/2015/04/a-simple-standalone-antenna-analyzer-with-ili9341tft.html

==================
libraries
==================

For driving lcd see : http://blog.riyas.org/2015/03/quickly-test-ili9341-with-arduino.html

For testing AD9850 see : http://blog.riyas.org/2014/02/quickly-test-ad9850-ebay-module-with-arduino-and-software-defined-radio.html

=================
VSWR -logic
=================

VSWR = (FWD+REV)/(FWD-REV);

where FWD and REV are the voltages read from the bridge (A0 and A1)




