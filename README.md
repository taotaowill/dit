### virtial file system
#### orgnize directorys on multi machines into one root
```
/
├── 192.168.1.1:8888
│   ├── dev
│   ├── home
│   └── root
└── 192.168.1.2.8888
     ├── dev
     ├── home
     └── root
```

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
  -r: travel directories recursively
  -f: do not prompt before overwriting
  -a: do not ignore entries starting with .
```

#### ls
```
./dit ls /
./dit ls /192.168.1.1:8888/home/work -ra
```

#### cp
```
./dit cp /192.168.1.1:8888/home/work/xxx //home/test/yyy
./dit cp /192.168.1.1:8888/home/work/xxx /192.168.1.2:8888/home/test
```

