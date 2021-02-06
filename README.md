# zperf record 

### **特点**  
* 纳秒级精度, 纳秒级性能损耗: 
  * rdtsc intel平台下 计数有效精度1纳秒 计数消耗偏差10纳秒  统计消耗总偏差15ns以下(默认使用)  
  * rdtsc amd  平台下 计数有效精度1纳秒 计数消耗偏差15纳秒  统计消耗总偏差20ns以下(默认使用)  
  * clock 统计消耗总偏差30ns以下   
  * WIN32下精度100ns   
  * 系统时钟精度1us  
* 被检测代码段外无额外性能损耗  
* 可用于判定定时器的周期稳定性和平均偏离度  
* 可用于判定函数性能消耗  
* 可用于系统内存监控统计  
* 可用于内存分配器消耗统计  


### ref  


* invariant TSC support| `cat /proc/cpuinfo |grep constant_tsc`
  * INTEL Nehalem from 2008 has invariant TSC support and no TSC SYNC problem
  * INTEL 17.15.1 
  ```
  The time stamp counter in newer processors may support an enhancement, referred to as invariant TSC. 
  Processor’s support for invariant TSC is indicated by CPUID.80000007H:EDX[8]. 
  The invariant TSC will run at a constant rate in all ACPI P-, C-. and T-states. This is the architectural behavior 
  moving forward. On processors with invariant TSC support, the OS may use the TSC for wall clock timer services 
  (instead of ACPI or HPET timers). TSC reads are much more efficient and do not incur the overhead associated with 
  a ring transition or access to a platform resource.
  ```

  * AMD K8 (10H Bacelona)  FROM 2003 has invariant TSC support  from K10 no TSC drift  

  * ARM V7 V8 has invariant PMCCNTR_EL0 support (need enable PMU)

  * PowerPC TBR
  

* intel fence;  amd has lfence from 0Fh/11h(K8&K10 hybrid 2008) but same mfence 
```
__asm__ __volatile__("mfence" : :: "memory")  //memory == l&s
__asm__ __volatile__("lfence" : :: "memory")  //load   
__asm__ __volatile__("sfence" : :: "memory")  //store
```



* 解决的问题  
  * 指令支持问题  是否支持rdtsc 不同的CPU体系有不同的指令 并且需要注意一下几个问题  
  * 编译器乱序问题  
  * 指令乱序和预读问题
    * 通过内存屏障解决 需要查看CPU手册确认内存屏障的粒度  
  * 变频问题  
    * CPU睿频以及节能带来的主频动态变化  需要查看CPU手册确认tsc寄存器是否以恒定频率执行    
  * 多核同步问题  
    * 多核心的调度带来的cpu切换问题 需要查看CPU手册确认tsc寄存器是否跨内核同步 以及提供的解决方式   
  * 最终性能   
    * 不同的CPU上, 以及不同的虚拟化下, 可用的clock类型和rdtsc有截然不同的性能表现  需要实际测试选择  

### 常用计时工具精度和耗时   

* C库函数 time(NULL)
  * WIN32 计数精度为1s  获取消耗32ns  
  * linux 计数精度为1s  获取消耗2ns  

* C库函数 clock  
  * WIN32 计数精度为1ms  获取消耗38ns  
  * linux 下计数精度为1us 获取消耗122ns(Intel X5650 下766ns)  实际测试精度准确度在100ms级别(误差有几十ms 唯一一个有误差)    

* C++ chrono
  * WIN32 system_clock 计数精度为100ns    获取消耗25ns DEBUG 39ns
  * WIN32 steady_clock 计数精度为100ns    获取消耗18ns DEBUG 65ns
  * linux system_clock 计数精度为1ns      获取消耗20ns DEBUG 25ns
  * linux steady_clock 计数精度为1ns      获取消耗19ns DEBUG 26ns

* QueryPerformanceCounter   
  * 计数精度为1ns    获取消耗28ns 
* GetSystemTimeAsFileTime  
  * 计数精度为1us    获取消耗4ns/23ns 
* clock_gettime
  * 相同CPU不同选项下甚至DEBUG/RELEASE下的区别差异都较大  多台不同硬件和linux发行版下相对稳定可用的为CLOCK_REALTIME  
  * 计数精度为1ns    获取消耗18ns  
  * 如果需要使用需要先本地测试, 性能消耗可能不符合预期(参考rdtscp数据)  
* rdtsc  
    * 计数精度为0.4ns左右 取决于主频   获取消耗7ns （18 circle） 
* rdtscp  
    * 计数精度为0.4ns左右 取决于主频   获取消耗2.58us (X5650双路CPU下600us/2.28s)  
* load fence rdtsc   
  * 计数精度为0.4ns左右 取决于主频   获取消耗9ns  (24 CPU CIRCLE)  
* load&store fence rdtsc   
  * 计数精度为0.4ns左右 取决于主频   获取消耗15ns 
  * 
* __rdtsc (WIN32)  
  * 计数精度为0.4ns左右 取决于主频   获取消耗9ns (24 CPU CIRCLE) 

* 对比测试  
  * 三元赋值一般约8个circle (存在指令并行,预读等和其他周围代码一起统计会有推算上的偏差)
  * s64类型两次乘法一次除法计算一次三元赋值和若干普通赋值的cpu统计代码消耗约为3.71ns  (大样本均摊) 
  * s64类型两次乘法           一次三元赋值和若干普通赋值的cpu统计代码消耗约为2.93ns  (大样本均摊) 
  * 4次加法赋值 1.59ns  (大样本均摊)   

* 小结   
  *  一般CPU主频是2.5~4Ghz之间(对本文来说为标频, 睿频无意义),  标频通常代表着高精度计数的极限
     *  计数极限为标频倒数 按照主流CPU的标频而言  通常最高精度在0.4~0.2ns左右;  
     *  读取和使用计数也需要执行指令 执行指令需要CPU计算    
     *  读取和使用计数可能涉及到指令以及计数的缓存内存操作等     
     *  不加保护的rdtsc约7ns (本文I7 3.7g主频CPU)  
  
  * std::chrono的稳定性和精度均为良好 如果要求不是特别高 可以采用该方案 
    * rdtsc比chrono方案快2~3倍 从比例上来说未有大量级的变化, 但是在指令运行消耗上有不可替代的差异  
    * 本文测试只是读取count而几乎未用到任何duration包装和cast转换, 在debug下还会有额外的开销   


### other perf tools   
* pstack  
* perf 
* strace
* trace-cmd record -P -p 
* valgrind 
* gperf & gperftools (need compile)

### **About The Author**  
**Author**: YaweiZhang  
**mail**: yawei.zhang@foxmail.com  
**github**: https://github.com/zsummer/zperf  