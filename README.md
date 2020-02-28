
# Simple Behavior Tree Engine
-------------

## Behavior Tree Overview
[Behavior tree overview](docs/bt_overview.md)

[Behavior tree overview on wikipedia](http://en.wikipedia.org/wiki/Behavior_Trees_(Artificial_Intelligence,_Robotics_and_Control))

## Features
### Nodes
- select
- sequence
- decorator 'succeeder'

### Actions
- simple unix shell exec

### Streams
- simple text stream

### Tested on
CentOS Linux release 7.6.1810  
expect version 5.45  
libxml version 20901  
tcl 8.5 

## Installation
### Debian dependencies:
```
sudo apt install gcc
sudo apt install libxml2-dev
sudo apt install expect
```
### RedHat dependencies:
```
sudo yum istall gcc
sudo yum install libxml2-devel
sudo yum install tcl-devel
sudo yum install expect-devel
```
### Building
```
$ make build 
```
### Running tests
```
$ make test
```
