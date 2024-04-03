
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
## CentOS Linux release 7.6.1810  
- expect version 5.45  
- libxml version 20901  
- tcl 8.5 
## CentOS Linux release 7.9.2009
- gcc-4.8.5
- libxml2-devel-2.9.1
- tcl-devel-8.5.13
- expect-devel-5.45

## Installation
### Debian dependencies:
```
sudo apt install gcc
sudo apt install libxml2-dev
sudo apt install expect

```
### CentOS/RedHat dependencies:
```
sudo yum install gcc
sudo yum install libxml2-devel
sudo yum install tcl-devel
sudo yum install expect-devel

```
### Building
```
$ make
```
### Running tests
```
$ make test
```
