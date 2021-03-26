
/*
* zperf License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
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
#include <array>
#include <limits.h>
#include <chrono>
#include <stdio.h>
#include <cstdarg>
#include <iostream>


#ifndef ZPERF_SERIALIZE_H
#define ZPERF_SERIALIZE_H


class PerfSerializeBuffer
{
public:
    PerfSerializeBuffer(char* buff, size_t buff_size)
    {
        buff_ = buff;
        buff_len_ = buff_size;
        offset_ = 0;
    }
    inline int serialize(const char* fmt, ...);
    inline int push_human_count(long long count);
    inline int push_human_time(long long ns);
    inline int push_human_mem(long long bytes);
    char* buff() { return buff_; }
    size_t offset() { return offset_; }
private:
    char* buff_;
    size_t offset_;
    size_t buff_len_;
};



int PerfSerializeBuffer::serialize(const char* fmt, ...)
{
    if (buff_ == NULL)
    {
        return 0;
    }
    if (buff_len_ == 0)
    {
        return 0;
    }
    if (buff_len_ <= offset_)
    {
        return 0;
    }

    int ret = 0;
    va_list argp;
    int argno = 0;
    char para;
    va_start(argp, fmt);
#if WIN32
    ret = _vsnprintf_s(buff_ + offset_, buff_len_ - offset_, _TRUNCATE, fmt, argp);
#else
    ret = vsnprintf(buff_ + offset_, buff_len_ - offset_, fmt, argp);
#endif // WIN32
    va_end(argp);
    if (ret < 0)
    {
        return 0;
    }
    offset_ += ret;
    return ret;
}

int PerfSerializeBuffer::push_human_count(long long count)
{
    if (count > 1000 * 1000)
    {
        return serialize("%lld,%03lld,%03lld", count / 1000 / 1000, (count / 1000) % 1000, count % 1000);

    }
    else if (count > 1000)
    {
        return serialize("%lld,%03lld", count / 1000, count % 1000);
    }
    return serialize("%lld", count);
}

int PerfSerializeBuffer::push_human_time(long long ns)
{
    if (ns > 1000 * 1000 * 1000)
    {
        return serialize("%.4lfs", ns / 1000.0 / 1000.0 / 1000.0);
    }
    else if (ns > 1000 * 1000)
    {
        return serialize("%.4lfms", ns / 1000.0 / 1000.0);
    }
    else if (ns > 1000)
    {
        return serialize("%.4lfus", ns / 1000.0);
    }
    return serialize("%lldns", ns);
}


int PerfSerializeBuffer::push_human_mem(long long bytes)
{
    if (bytes > 1024 * 1024 * 1024)
    {
        return serialize("%.4lfg", bytes / 1024.0 / 1024.0 / 1024.0);
    }
    else if (bytes > 1024 * 1024)
    {
        return serialize("%.4lfm", bytes / 1024.0 / 1024.0);
    }
    else if (bytes > 1024)
    {
        return serialize("%.4lfk", bytes / 1024.0);
    }
    return serialize("%lldb", bytes);
}



#endif
