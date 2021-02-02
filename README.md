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


### **About The Author**  
**Author**: YaweiZhang  
**mail**: yawei.zhang@foxmail.com  
**github**: https://github.com/zsummer/zperf  