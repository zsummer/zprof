
/*
* zperf License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/



#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <array>
#include <mutex>
#include <thread>
#include <functional>
#include <regex>
#include <atomic>
#include <cmath>
#include <cfloat>
#include <list>
#include <deque>
#include <queue>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <memory>
#include <atomic>

#ifdef _WIN32
#ifndef KEEP_INPUT_QUICK_EDIT
#define KEEP_INPUT_QUICK_EDIT false
#endif

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif


#ifndef ZPERF_H
#define ZPERF_H

#define MAX_PERF_NODE_NAME_SIZE 100
#define MAX_PERF_NODE_LINE_SIZE (MAX_PERF_NODE_NAME_SIZE + 200)
#define MAX_PERF_NODE_CHILD_COUNT 10
#define MAX_PERF_NODE_CHILD_DEPTH 5
struct PerfDesc
{
    char node_name[MAX_PERF_NODE_NAME_SIZE];
    int node_name_len;
};

struct PerfCPU
{
    long long call_count; //调用次数   
    long long call_use_time;  //调用耗时    
    long long user_val_sum; //用户累加值      
};

struct PerfMEM
{
    long long call_count; //调用次数   
    long long change_mem;  //内存累加变化    
    long long fixed_size; //内存固定值     
};

struct PerfNode
{
    bool active; 
    bool is_child;  
    std::array<unsigned int, MAX_PERF_NODE_CHILD_COUNT> child_ids; 
    int child_count; 
    PerfDesc desc; 
    PerfCPU cpu; 
    PerfMEM mem; 
};  



#define PERF_USE_THREAD
#define PERF_USE_REALTIME

inline long long perf_now_ns()
{
#ifdef WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long now = ft.dwHighDateTime;
    now <<= 32;
    now |= ft.dwLowDateTime;
    now /= 10;
    now -= 11644473600000000ULL;
    return (long long)now * 1000; //ns

#elif (defined PERF_USE_REALTIME)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 * 1000 + ts.tv_nsec;
#elif (defined PERF_USE_THREAD)
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000 * 1000 + ts.tv_nsec;
#else
    struct timeval tm;
    gettimeofday(&tm, nullptr);
    return tm.tv_sec * 1000 * 1000 * 1000 + tm.tv_usec * 1000;
#endif
}


inline int perf_self_memory_use()
{
    const char* file = "/proc/self/status";
    FILE* fp = fopen(file, "r");
    if (NULL == fp)
    {
        return 0;
    }

    char line_buff[256];
    int vm_size = 0;
    while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
    {
        if (strstr(line_buff, "VmSize") != NULL)
        {
            const char* p = line_buff;
            while (p < &line_buff[255] && (*p < '0' || *p > '9'))
            {
                p++;
            }
            if (p == &line_buff[255])
            {
                break;
            }

            int ret = sscanf(p, "%d", &vm_size);
            if (ret <= 0)
            {
                vm_size = 0;
                break;
            }
            break;
        }
    }

    fclose(fp);
    return (long long) vm_size * 1000;
}


class PerfTime
{
public:
    PerfTime()
    {
        begin_tick();
    }
    PerfTime(long long begin)
    {
        last_time_ = begin;
        duration_ = 0;
    }
    void begin_tick()
    {
        last_time_ = perf_now_ns();
        duration_ = 0;
    }

    PerfTime& end_tick()
    {
        long long elapse = perf_now_ns() - last_time_;
        duration_ = elapse > 0 ? elapse : 0;
        return *this;
    }

    long long duration() {return duration_; }
    double duration_second() { return (double)duration_ / (1000.0 * 1000.0 * 1000.0); }
    long long end(){ return last_time_ + duration_;}
    long long begin(){return last_time_;}
    
private:
    long long last_time_;
    long long duration_;
};


template<int T, int S>
class PerfRecord 
{
public:
    static const int NODE_COUNT = S;
    static const int PERF_TYPE = T;
    PerfRecord() 
    {
        memset(nodes_, 0, sizeof(nodes_));
    };
    static PerfRecord& instance()
    {
        static PerfRecord inst;
        return inst;
    }
    
    inline int regist_node(int idx, const char* desc, bool overwrite);
    inline int add_node_child(int idx, int child);
    
    void reset_cpu(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.cpu, 0, sizeof(node.cpu));
    }
    void reset_mem(int idx)
    {
        PerfNode& node = nodes_[idx];
        long long fixed_size = node.mem.fixed_size;
        memset(&node.mem, 0, sizeof(node.mem));
        node.mem.fixed_size = fixed_size;
    }
    inline void reset_childs(int idx, int depth = 0);
    void call_cpu(int idx, long long call_count, long long use_time, long long add_val)
    {
        PerfNode& node = nodes_[idx];
        node.cpu.call_count += call_count;
        node.cpu.call_use_time += use_time;
        node.cpu.user_val_sum += add_val;
    }
    void call_mem(int idx, long long call_count, long long change_mem)
    {
        PerfNode& node = nodes_[idx];
        node.mem.call_count += call_count;
        node.mem.change_mem += change_mem;
    }
    
    void set_fixed_mem(int idx, long long fixed_size)
    {
        PerfNode& node = nodes_[idx];
        node.mem.fixed_size = fixed_size;
    }

    int serialize(int entry_idx, int depth, char* org_buff, int buff_len);
    const char* serialize(int entry_idx);


    PerfNode& node(int idx) { return nodes_[idx]; }
    int node_count()const { return NODE_COUNT; }
private:
    PerfNode nodes_[S];
};

template<int T, int S>
int PerfRecord<T, S>::add_node_child(int idx, int cidx)
{
    if (idx < 0 || idx >= NODE_COUNT || cidx < 0 || cidx >= NODE_COUNT)
    {
        return -1;
    }

    if (idx == cidx)
    {
        return -2;  
    }

    PerfNode& node = nodes_[idx];
    PerfNode& child = nodes_[cidx];
    if (!node.active || !child.active)
    {
        return -3; //regist method has memset all info ; 
    }
    if (node.child_count >= MAX_PERF_NODE_CHILD_COUNT)
    {
        return -4;
    }
    if (std::find_if(node.child_ids.begin(), node.child_ids.end(), [cidx](int id) {return id == cidx; }) != node.child_ids.end())
    {
        return -5; //duplicate 
    }

    node.child_ids[node.child_count++] = cidx;
    child.is_child = true;
    return 0;
}

template<int T, int S>
int PerfRecord<T, S>::regist_node(int idx, const char* desc, bool overwrite)
{
    if (idx < 0)
    {
        return -1;
    }
    if (idx >= NODE_COUNT)
    {
        return -2;
    }
    if (desc == NULL)
    {
        return -3;
    }
    PerfNode& node = nodes_[idx];


    if (overwrite && node.active)
    {
        return 0;
    }

    int len = (int)strlen(desc);
    if (len + 1 > MAX_PERF_NODE_NAME_SIZE)
    {
        return -4;
    }
    
    memset(&node, 0, sizeof(node));
    memcpy(node.desc.node_name, desc, len+1);
    node.desc.node_name_len = len;
    node.active = true;
    return 0;
}

template<int T, int S>
void PerfRecord<T, S>::reset_childs(int idx, int depth)
{
    if (idx < 0)
    {
        return;
    }
    if (idx >= NODE_COUNT)
    {
        return;
    }
    PerfNode& node = nodes_[idx];
    memset(&node.cpu, 0, sizeof(node.cpu));
    memset(&node.mem, 0, sizeof(node.mem));
    if (depth > 5)
    {
        return;
    }
    for (int i = 0; i < node.child_count; i++)
    {
        reset_childs(node.child_ids[i], depth + 1);
    }
}



static inline const char* human_count_format(char *buff, long long count)
{
    if (count > 1000 * 1000)
    {
        sprintf(buff, "%lld,%03lld,%03lld", count / 1000 / 1000, (count / 1000) % 1000, count % 1000);
        return buff;
    }
    else if (count > 1000)
    {
        sprintf(buff, "%lld,%03lld", count / 1000, count % 1000);
        return buff;
    }
    sprintf(buff, "%lld", count);
    return buff;
}

static inline const char* human_count_format(long long count)
{
    static char buff[100] = { 0 };
    return human_count_format(buff, count);
}

static inline const char* human_time_format(char* buff, long long ns)
{
    if (ns > 1000*1000*1000)
    {
        sprintf(buff, "%.4lfs", ns / 1000.0 / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000*1000)
    {
        sprintf(buff, "%.4lfms", ns / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000)
    {
        sprintf(buff, "%.4lfus", ns / 1000.0 );
        return buff;
    }
    sprintf(buff, "%lldns", ns);
    return buff;
}

static inline const char* human_time_format(long long ns)
{
    static char buff[100] = { 0 };
    return human_time_format(buff, ns);
}

static inline const char* human_mem_format(char* buff, long long bytes)
{
    if (bytes > 1024 * 1024 * 1024)
    {
        sprintf(buff, "%.4lfg", bytes / 1024.0 / 1024.0 / 1024.0);
        return buff;
    }
    else if (bytes > 1024 * 1024)
    {
        sprintf(buff, "%.4lfm", bytes / 1024.0 / 1024.0);
        return buff;
    }
    else if (bytes > 1024)
    {
        sprintf(buff, "%.4lfk", bytes / 1024.0);
        return buff;
    }
    sprintf(buff, "%lldb", bytes);
    return buff;
}

static inline const char* human_mem_format(long long bytes)
{
    static char buff[100] = { 0 };
    return human_mem_format(buff, bytes);
}


template<int T, int S>
int PerfRecord<T, S>::serialize(int entry_idx, int depth, char* org_buff, int buff_len)
{
    if (entry_idx < 0 || entry_idx >= NODE_COUNT)
    {
        return -1;
    }
    if (buff_len < MAX_PERF_NODE_LINE_SIZE)
    {
        return -2;
    }
    char* buff = org_buff;


    PerfNode& node = nodes_[entry_idx];
    if (node.cpu.call_count > 0)
    {
        for (int i = 0; i < depth; i++)
        {
            *buff++ = ' ';
            *buff++ = ' ';
            buff_len -= 2;
        }
        *(buff + 1) = '\0';
        if (buff_len < MAX_PERF_NODE_LINE_SIZE)
        {
            return -3;
        }
        char buff_call[50];
        human_count_format(buff_call, node.cpu.call_count);
        char buff_avg[50];
        human_time_format(buff_avg, node.cpu.call_use_time / node.cpu.call_count);
        char buff_sum[50];
        human_time_format(buff_sum, node.cpu.call_use_time);
        char buff_val[50];
        human_count_format(buff_val, node.cpu.user_val_sum);

        int ret = sprintf(buff, "[%s] cpu: call:%s  avg use: %s  sum use: %s  user val sum:%s \n",
            node.desc.node_name, buff_call, buff_avg, buff_sum, buff_val);

        if (ret < 0)
        {
            return -4;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -5;
        }
    }
    if (node.mem.call_count > 0)
    {
        if (buff_len < MAX_PERF_NODE_LINE_SIZE)
        {
            return -6;
        }
        for (int i = 0; i < depth; i++)
        {
            *buff++ = ' ';
            *buff++ = ' ';
            buff_len -= 2;
        }
        *(buff + 1) = '\0';
        if (buff_len < MAX_PERF_NODE_LINE_SIZE)
        {
            return -6;
        }
        char buff_call[50];
        human_count_format(buff_call, node.mem.call_count);
        char buff_avg[50];
        human_time_format(buff_avg, node.mem.change_mem / node.mem.call_count);
        char buff_sum[50];
        human_time_format(buff_sum, node.mem.change_mem);
        char buff_val[50];
        human_count_format(buff_val, node.mem.fixed_size);

        int ret = sprintf(buff, "[%s] cpu: call:%s  avg change: %s  sum: %s  fixed size:%s \n",
            node.desc.node_name, buff_call, buff_avg, buff_sum, buff_val);

        if (ret < 0)
        {
            return -7;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -8;
        }
    }
    if (node.cpu.call_count <= 0 && node.mem.call_count <= 0)
    {
        int ret = sprintf(buff, "[%s] no any call data. mem fixed size:<%lld>. \n", node.desc.node_name, node.mem.fixed_size);
        if (ret < 0)
        {
            return -7;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -8;
        }
    }

    if (depth > 5)
    {
        return -9;
    }

    for (int i = 0; i < node.child_count; i++)
    {
        int ret = serialize(node.child_ids[i], depth + 1, buff, buff_len);
        if (ret < 0)
        {
            ret -= 10 * (1+depth);
            return ret;
        }
        buff += buff_len - ret;
        buff_len = ret;
    }
    return buff_len;
}





template<int T, int S>
const char* PerfRecord<T, S>::serialize(int entry_idx)
{
    static const int buff_size = MAX_PERF_NODE_CHILD_DEPTH * MAX_PERF_NODE_CHILD_COUNT * MAX_PERF_NODE_LINE_SIZE * 2;
    static char buff[buff_size];
    int remaind = buff_size;
    remaind -= sprintf(buff, "serialize:\n");
    int ret = serialize(entry_idx, 0, buff + (buff_size - remaind), remaind);
    if (ret < 0)
    {
        sprintf(buff, "serialize idx:<%d> has error:<%d>\n", entry_idx, ret);
    }
    return buff;
}


#define PerfInst PerfRecord<0, 100>::instance()





#endif
