Dependencies:

#### Hiredis (Redis C++ library)
```sh
$ git clone https://github.com/redis/hiredis
$ cd hiredis
$ make
```

#### Boost (C++ Library)
```sh
$ wget ://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz/download
$ tar -xvf boost_1_56_0.tar.gz
$ cd boost_1_56_0/
$ ./booststrap.sh
$ ./b2
```

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
#### Run `net_recv`  which gives the `stone-id`
```sh
$ ./bin/net_recv -m /net/hu21/agangil3/khanEVPath/test -d -p 6379 -s stores.txt
```

#### Run `net_send` with stone id obtained in the last step as an argument.
```sh
$ ./bin/net_send 0:Abdsf23424324233423423423 -s stores.txt
```

Porting Khan to EVPath http://www.cc.gatech.edu/systems/projects/EVPath/
