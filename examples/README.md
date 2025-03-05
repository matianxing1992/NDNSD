## NDNSD Examples

The examples are not build by default. Use option --with-examples with configure to build them i.e. `ndnsd-consumer` and `ndnsd-consumer`. 

`./waf configure --with-examples`  
`./waf build`

The example binaries (ndnsd-producer and ndnsd-consumer) can be found in build/examples. 
* Service Publisher: `ndnsd-producer`

run `sudo ./waf install` to install the libraries + examples into the system (optional, but will be easy to use)

Once everything is installed, go through the following steps to run the consumer and the producer

#### Start NFD 

* start nfd -> `nfd-start &` '&' to run in backgroud.


#### start ndnsd-producer  
* `export NDN_LOG=ndnsd.*=TRACE`
* `nfdc strategy set /muas /localhost/nfd/strategy/multicast`
* `ndnsd-producer /muas /muas/node1`
* `ndnsd-producer /muas /muas/node2`