# zperf record 

### **特点**  
* 皮秒级计时精度, 10纳秒以下统计分析的总性能损耗, 跨windows/mac/linux: 
  * rdtsc intel平台下 计数有效精度小于1纳秒 计数消耗偏差7~10纳秒   统计消耗总偏差10~13ns以下(默认使用, 可根据实际环境选择更快的方案(统计总消耗10ns以下))  
  * rdtsc amd  平台下 计数有效精度小于1纳秒 计数消耗偏差10~15纳秒  统计消耗总偏差17~20ns以下(默认使用)  
  * 最快的方案中(需要保证CPU乱序边界或者从测试数据角度不影响的情况下) 离散统计15ns的消耗代码段总统计只有5ns的增量偏差(实际被测试场景一般在100ns或者1us以上 测试代码消耗和精度都在5%以下)
* 有丰富的其他备选计时方案以及数据对比 (AMD/INTEL cpu中支持constant_tsc即可 不区分操作系统 其他CPU暂时更改默认宏为chrono或者clock 后续有需求作者可以补更多CPU的支持)    
* 被检测代码段外无额外性能损耗  
* 可用于判定定时器的周期稳定性和平均偏离度  
* 可用于判定函数性能消耗  
* 可用于系统内存监控统计  
* 可用于内存分配器消耗统计  
* 可嵌入现有大规模项目埋点测试和统计    
* 可用于小项目和测试方案的的快速检测  

### ref  


* invariant TSC support| `cat /proc/cpuinfo |grep constant_tsc`
  * INTEL Nehalem from 2008 has invariant TSC support and no TSC SYNC problem
  * INTEL 17.15
    ```
      The RDTSC instruction is not serializing or ordered with other instructions. It does not necessarily wait until all
    previous instructions have been executed before reading the counter. Similarly, subsequent instructions may begin
    execution before the RDTSC instruction operation is performed.
    ```
  * INTEL 17.15.1 Invariant TSC
      ```
      The time stamp counter in newer processors may support an enhancement, referred to as invariant TSC. 
      Processor’s support for invariant TSC is indicated by CPUID.80000007H:EDX[8]. 
      The invariant TSC will run at a constant rate in all ACPI P-, C-. and T-states. This is the architectural behavior 
      moving forward. On processors with invariant TSC support, the OS may use the TSC for wall clock timer services 
      (instead of ACPI or HPET timers). TSC reads are much more efficient and do not incur the overhead associated with 
      a ring transition or access to a platform resource.
      ```
  * 17.15.4 Invariant Time-Keeping
    ```
    The invariant TSC is based on the invariant timekeeping hardware (called Always Running Timer or ART), that runs
    at the core crystal clock frequency. The ratio defined by CPUID leaf 15H expresses the frequency relationship
    between the ART hardware and TSC.
    If CPUID.15H:EBX[31:0] != 0 and CPUID.80000007H:EDX[InvariantTSC] = 1, the following linearity relationship
    holds between TSC and the ART hardware:
    TSC_Value = (ART_Value * CPUID.15H:EBX[31:0] )/ CPUID.15H:EAX[31:0] + K
    Where 'K' is an offset that can be adjusted by a privileged agent2.
    When ART hardware is reset, both invariant TSC and K are also reset.
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
    * 多核心的调度带来的cpu切换问题 需要查看CPU手册确认tsc寄存器的实现方式或者是否有跨内核同步的解决方式   
  * 最终性能   
    * 不同的CPU上, 以及不同的虚拟化下, 可用的clock类型和tsc相关获取指令有截然不同的性能表现  需要查找手册确认或者实际测试 

### 常用计时工具精度和耗时   


* C库函数 time(NULL)
  * WIN32 计数精度为1s  获取消耗32ns  
  * linux 计数精度为1s  获取消耗2ns  
  * MAC   计数精度为1s  获取消耗155ns  

* C库函数 clock  
  * WIN32 计数精度为1ms  获取消耗38ns  
  * linux 下计数精度为1us 获取消耗122ns(Intel X5650 下766ns, mac:476)  实际测试精度准确度在100ms级别(误差有几十ms 唯一一个有误差)    

* C++ chrono : high_resolution_clock是通常为steady clock(实现定义 最好指定为steady)  
  * WIN32 system_clock 计数精度为100ns    获取消耗25ns DEBUG 39ns
  * WIN32 steady_clock 计数精度为100ns    获取消耗18ns DEBUG 65ns
  * linux system_clock 计数精度为1ns      获取消耗20ns DEBUG 25ns  
  * linux steady_clock 计数精度为1ns      获取消耗19ns DEBUG 26ns
  * linux system_clock 计数精度为1ns      获取消耗33ns DEBUG  
  * linux steady_clock 计数精度为1ns      获取消耗45ns DEBUG  
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
  * 计数精度为0.4ns左右 取决于主频   获取消耗9ns  (24 CPU CIRCLE)  (MAC 13ns)
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
  *  一般CPU主频是2.5\~4Ghz之间(对本文来说为标频, 睿频无意义),  标频通常代表着高精度计数的极限(INTEL平台下和同时和ART硬件有关)
     *  计数极限为标频倒数 按照主流CPU的标频而言  通常最高精度在0.4\~0.2ns左右;  
     *  读取和使用计数也需要执行指令 执行指令需要CPU计算    
     *  读取和使用计数可能涉及到指令以及计数的缓存内存操作等     
     *  不加保护的rdtsc约7ns (本文I7 3.7g主频CPU)  
  
  * std::chrono的稳定性和精度均为良好 并且跨平台性最好(C++11标准)   
    * 通常在20\~65ns左右(大概测试了3台windows 5台linux 1台mac (均是INTEL CPU))  
    * 100ns以下的获取损耗 以及1ns精度   
  
  * rdtsc精度最高 速度最快 稳定性最好 但是需要确认CPU体系和版本保证可用(当前只针对INTEL/AMD上市年份在07/08年以后的新CPU)   
    * 通常稳定在10ns以下或者说在30个CPU周期之内, 几乎不受编译选项和平台影响, 并且不同CPU损耗接近;
    * 横向对比则相当于4次三元取值指令的性能开销   
    * 对于指令级粒度的性能测试,  以及进行高频函数的性能统计采样中,  更小的性能开销和更高的精度具有不可取代的作用和价值.     
      * 对于9ns的代码段进行独立性能测试 rdtsc测试为11~15ns, chrono测试为44~50ns  (对应每秒1亿次规模的代码段进行性能对比 rdtsc 44.4% chrono 422%  提高了10倍的精度 chrono统计的数据在该级别基本无意义) 
      * 对于80ns的代码段进行独立性能测试 rdtsc测试为82~85ns, chrono测试为100~122ns  (对应每秒1千万次规模的代码段进行性能对比 rdtsc 4.4%,  chrono 48.4% 提高了10倍的精度 rdtsc在该级别已经非常精准, chrono偏差较大) 
  
  
  * 该小结中未列举到的其他方案 存在以下问题不推荐使用 
    * 性能开销太大或者不稳定  
    * 精度不够或者不稳定  
    * 不同编译选项或者平台差异过大  


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