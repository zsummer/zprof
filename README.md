# zperf record 

### **特点**  
* 纳秒级精度, 纳秒级性能损耗: 
  * rdtsc intel平台下 计数有效精度1纳秒 计数消耗偏差10纳秒  统计消耗总偏差15ns以下(默认使用)  
  * rdtsc amd  平台下 计数有效精度1纳秒 计数消耗偏差15纳秒  统计消耗总偏差20ns以下(默认使用)  
  * clock 统计消耗总偏差30ns以下   
  * WIN32下精度100ns   
  * 系统时钟精度1us  

* 可用于判定定时器的周期稳定性和平均偏离度  
* 可用于判定函数性能消耗  
* 可用于系统内存监控统计  
* 可用于内存分配器消耗统计  
* 使用方便   

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

### **About The Author**  
**Author**: YaweiZhang  
**mail**: yawei.zhang@foxmail.com  
**github**: https://github.com/zsummer/zperf  