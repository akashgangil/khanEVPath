Dependencies:

Hiredis (Redis C++ library)
  * git clone https://github.com/redis/hiredis
  * cd hiredis
  * make

Boost (C++ Library)
* wget ://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz/download
* tar -xvf boost_1_56_0.tar.gz
* cd boost_1_56_0/
* ./booststrap.sh
* ./b2

To build:
make

* Generates two executables `net_send` and `net_recv` in the bin folder 
and all object files in build folder
* Run `net_recv`  which gives the `stone-id` ./net_recv "arg:mount_dir" -d
Ex: bin/net_recv <mount_dir> -d ==> bin/net_recv /net/hu21/agangil3/temp -d

* Run `net_send` with stone id obtained in the last step as an argument.
Ex: bin/net_send 0:Abdsf23424324233423423423

Porting Khan to EVPath http://www.cc.gatech.edu/systems/projects/EVPath/
