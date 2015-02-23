Dependencies:

#### Install Curl
```sh
$ sudo apt-get install libcurl4-gnutls-dev
```

#### Install Fuse
```sh
$ sudo apt-get install fuse libfuse-dev
```

#### Install Python Dev 
```sh
$ sudo apt-get install python-dev
```


#### Hiredis (Redis C++ library)
```sh
$ git clone https://github.com/redis/hiredis
$ cd hiredis
$ make
```
*Had to create a symbolic link in the hiredis directory to the .so library named *.so.0.12.

#### Redis
```sh
$ wget http://download.redis.io/releases/redis-2.8.15.tar.gz
$ tar -xvf redis-2.8.15.tar.gz
$ cd redis-2.8.15/
$ make
$ make (test to check if the installation went correctly)
```

#### To build:
```sh
$ make
```

* Generates two executables `net_send` and `net_recv` in the bin folder 
and all object files in build folder

#### Start the redis server 
```sh
cd ~/redis/src 
./redis-server & 
```

#### Point khan to the data
Modify stores.txt to point to the data by replacing the first string on the second line.

#### Create a "test" directory
Under the project base folder, this is the place where the fuse filesystem gets mounted.


### To Run

#### Initialize the dfg topology
```sh
./rundfgmaster.sh
```

#### Run the source test client
```sh
./bin/net_send "a" -s stores.txt
```

#### Run the sink client
```sh
./bin/net_recv "b" -m /net/hu21/agangil3/khanEVPath/test -d -p 6279 -s stores.txt
```

* You might need to do python setup.py build && python setup.py install in PyScripts/libim7 folder


### Deprecated

~~#### Run `net_recv`  which gives the `stone-id`--
```sh
~~$ ./bin/net_recv -m /net/hu21/agangil3/khanEVPath/test -d -p 6379 -s stores.txt~~
```

~~#### Run `net_send` with stone id obtained in the last step as an argument.~~
```sh
~~$ ./bin/net_send 0:Abdsf23424324233423423423 -s stores.txt~~
```


Porting Khan to EVPath http://www.cc.gatech.edu/systems/projects/EVPath/
