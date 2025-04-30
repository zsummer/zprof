
# Introduction:  
[![Build](https://github.com/zsummer/zbase/actions/workflows/cmake.yml/badge.svg)](https://github.com/zsummer/zbase/actions/workflows/cmake.yml)

from zbase

# Feature:  
代码风格: stl likely 
框架层级: base/utils   
应用场景: high-performance server   
核心特性: shared memory resume      
 

#  Example  



# dist tree 
the dist dir auto release and commit from src by github action.   

```
dist
├── include
│   └── zbase
│       ├── LICENSE
│       ├── README.md
│       ├── VERSION
│       ├── zallocator.h
│       ├── zarray.h
│       ├── ...
│       └── zvector.h
│        
├── lib
│   └── zbase
└── doc
    └── zbase
```

# dist branch   
the dist branch auto merge  from ```[master]/dist``` to ```[dist]/```   



# How To Use  
1. copy [master]/dist to your project   
2. use [git subtree] into your project 
   * ```git subtree add --prefix=vender/zbase --squash -d  https://github.com/zsummer/zbase.git dist``` 
3. use [git submodule] into your project 
   * ```git submodule add -b dist https://github.com/zsummer/zbase.git vender/zbase```



# mark action token use
1. generay key 
   * [github] Settings -> Developer settings   
   * copy key
2. secrys env [var]   
   * Repo -> Settings -> Secrets -> Action   
   * write var and content(key)   
3. use secrys var in actions  
   * ${{secrets.VAR_NAME}}   

 

# About The Author  
Author: YaweiZhang  
Mail: yawei.zhang@foxmail.com  
QQGROUP: 524700770  
GitHub: https://github.com/zsummer/zbase  

