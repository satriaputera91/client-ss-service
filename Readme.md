Smart Speaker Client 
====================

To build the library: see `./INSTALL`.  These instructions are valid for UNIX
systems including various flavors of Linux; Darwin; and Cygwin (has not been
tested on more "exotic" varieties of UNIX). 
    
    
If you encounter problems (and you probably will), please do not hesitate to
contact the developers (see below). In addition to specific questions, please
let us know if there are specific aspects of the project that you feel could be
improved, that you find confusing, etc., and which missing features you most
wish it had.



contact us  : info@bahasakita.co.id

website     : https://www.bahasakita.co.id/


Platform specific notes
-----------------------
### PowerPC 64bits little-endian (ppc64le)

Smart Speaker Client Program is expected to work out of the box in Ubuntu >= 16.04 with
json-c

Installing depedencies
----------------------
	sudo apt-get install libasound2 libasound2-dev libjson0 libjson0-dev libzmq5 libzmq3-dev mpd mpc vim

How to use
----------
## Setting mpd.conf
sudo vim /etc/mpd.conf
change set audio output :

	audio_output {
        	type            "alsa"
        	name            "My ALSA Device"
        	device          "hw:0,0"        # optional
        	mixer_type      "software"      # optional	
	}

reboot now your computer !!!
	
## Running Program
	make 
	run.sh
