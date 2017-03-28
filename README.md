#Fortexta
###A lightweight text editor for the La Routa Della Fortuna

Fortexta is a lightweight text editor for getting user string inputs in built to
work on the University of Southampton's "La Routa Della Fortuna" boards.

##Install
This program is designed to run specifically on "La Fortuna" boards. These boards
make use of an Atmel procressor, and so to installing this software will require
using the avr-toolchain as well as dfu-programmer >= 0.7.0.

Once these are installed, clone this repository. Then, once inside the repo
folder run "$ sudo make". Sudo is required in order to communicate with the
"La Fortuna" via USB.

##Usage
To use Fortexta, include fortexta.c in your project.

Then use the openFortexta(char*, int) method, the first argument being the
string to be edited and the second being it's initial length.
