# Project Repositories #

Be sure to check out the [gui](https://github.com/slightlynybbled/curve_tracer_gui) and 
[hardware](https://github.com/slightlynybbled/curve_tracer_hardware) repositories!

# Purpose #

The purpose of this repository is to document the curve tracer project.  A
full description may be found at [for(embed)](http://www.forembed.com/project-curve-tracer-requirements).

# Development Environment #

## IDE and Compiler ##

Any flavor of MPLAB X IDE and XC16 compiler that supports the PIC24FV16KM202 should work just fine.
An optimization level of -O1 is recommended to reduce code size, which will allow future features
to fit within the limited flash memory of the device.

## Loading ##

I am currently using a Microchip ICD3.  You should be able to use a PICkit3 or REAL ICE as well.
The PICkit3 is the most economical solution at around $40 US.
