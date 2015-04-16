# swranalyser
A simple swr analyser based on ad9850 dds

This project uses a simple ad9850 module as its heart to generate the rf signals for the sweep. The controller is an arduino promini (with an atmega328 running on 3.3volt) and for display, it uses an ili9341 tft.

The core circuitry for the analyser is very simple and is based on the design from Beriks (K6BEZ) simple antenna analyser (pdf). I added a buffer with the dds  (as shown on W5DOR).

Pictures at : http://blog.riyas.org/2015/04/a-simple-standalone-antenna-analyzer-with-ili9341tft.html

