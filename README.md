# Dit
data transmission tool

## Features
### 1.transmit data with multi-thread
### 2.orgnize directorys on multi machines into one root
```
/
├── 192.168.1.1:8888
│   ├── dev
│   ├── home
│   ├── root
│   └── ...
└── 192.168.1.2:8888
    ├── dev
    ├── home
    ├── root
    └── ...
```

## Build
```
./build_deps.sh
scons
```

## Usage
### server
```
./ditd
```

### client
```
./dit

Usage: dit COMMAND [OPTION]... [PATH]...
commands:
  ls PATH [-a] [-c]: list path
  cp SRC_PATH DST_PATH [-r] [-f]: copy path
  rm PATH [-r] [-f]: remove path
options:
  it is recommend to place options at the end of line
  -a: do not ignore entries starting with .
  -c: control whether color is used to distinguish file
  -f: do not prompt before overwriting
  -r: travel directories recursively
```

```
./dit ls /192.168.1.1:8888/home/work -r -a

/192.168.1.1:8888/home/work/
/192.168.1.1:8888/home/work/a/
/192.168.1.1:8888/home/work/a/a.txt
/192.168.1.1:8888/home/work/b/
/192.168.1.1:8888/home/work/b/c
```

```
# local start with: '//'

# copy file from remote to local
./dit cp /192.168.1.1:8888/home/work/xxx //home/test/yyy

# copy file from local to remote
./dit cp //home/test/yyy /192.168.1.1:8888/home/work/xxx 
```
