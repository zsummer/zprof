
/*
* zprof License
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

#ifndef ZPROF_SERIALIZE_H
#define ZPROF_SERIALIZE_H

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <array>
#include <limits.h>
#include <chrono>
#include <stdio.h>
#include <cstdarg>
#include <iostream>


namespace zprof
{



    #ifdef WIN32
    #define PROF_LINE_FEED "\r\n"
    #elif (defined __APPLE__)
    #define PROF_LINE_FEED "\r\n"
    #else
    #define PROF_LINE_FEED "\n"
    #endif // WIN32


    #define PROF_NAME_MAX_SIZE 50  
    #define PROF_DESC_MAX_SIZE 100
    #define PROF_LINE_MIN_SIZE 200
    #define PROF_MAX_DEPTH 5

    class ProfSerializer
    {
    public:
        ProfSerializer() = delete;
        explicit ProfSerializer(char* buff, size_t buff_size)
        {
            buff_ = buff;
            buff_len_ = buff_size;
            offset_ = 0;
        }


        inline ProfSerializer& push_human_count(long long count);
        inline ProfSerializer& push_human_time(long long ns);
        inline ProfSerializer& push_human_mem(long long bytes);
        inline ProfSerializer& push_char(char ch, int repeat = 1);
        inline ProfSerializer& push_string(const char* str);
        inline ProfSerializer& push_string(const char* str, size_t size);
        inline ProfSerializer& push_now_date();
        inline ProfSerializer& push_number(unsigned long long number, int wide = 0);
        inline ProfSerializer& push_number(long long number, int wide = 0);

        inline ProfSerializer& push_indent(int count);
        inline ProfSerializer& push_blank(int count);


        inline void closing_string();
        bool is_full() { return offset_ + 1 >= buff_len_; } //saved one char  
        char* buff() { return buff_; }
        const char* buff() const { return buff_; }
        size_t offset() { return offset_; }
        size_t offset() const { return offset_; }
        size_t buff_len() { return buff_len_; }
        size_t buff_len()const { return buff_len_; }
        void reset_offset(size_t offset = 0) { offset_ = offset; }
    private:
        char* buff_;
        size_t offset_;
        size_t buff_len_;
    };




    inline ProfSerializer& ProfSerializer::push_human_count(long long count)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (count > 1000 * 1000)
        {
            push_number((unsigned long long)(count / 1000 / 1000));
            push_char(',');
            push_number((unsigned long long)((count / 1000) % 1000), 3);
            push_char(',');
            push_number((unsigned long long)(count % 1000), 3);
            return *this;
        }
        else if (count > 1000)
        {
            push_number((unsigned long long)(count / 1000));
            push_char(',');
            push_number((unsigned long long)(count % 1000), 3);
            return *this;
        }
        return push_number((unsigned long long)(count));
    }

    inline ProfSerializer& ProfSerializer::push_human_time(long long ns)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        long long fr = 1;
        long long mr = 1000;
        char fc = 'n';
        char lc = 's';
        if (ns >= 1000*1000*1000)
        {
            fr = 1000 * 1000 * 1000;
            mr = 1000 * 1000;
            fc = 's';
            lc = ' ';
        }
        else if (ns >= 1000*1000)
        {
            fr = 1000  * 1000;
            mr = 1000;
            fc = 'm';
            lc = 's';
        }
        else if (ns >= 1000)
        {
            fr = 1000;
            mr = 1;
            fc = 'u';
            lc = 's';
        }
        else if (ns < 0) //cost may be over  when long long time  elapse . 
        {
            //ns = 0;
            push_string("invalid");
            return *this;
        }
        push_number((long long)(ns/ fr));
        buff_[offset_++] = '.';
        push_number((unsigned long long)((ns/ mr) % 1000), 3);
        buff_[offset_++] = fc;
        buff_[offset_++] = lc;
        return *this;
    }


    inline ProfSerializer& ProfSerializer::push_human_mem(long long bytes)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (bytes > 1024 * 1024 * 1024)
        {
            push_number((unsigned long long)(bytes / 1024 / 1024 / 1024));
            push_char('.');
            push_number((unsigned long long)((bytes / 1024 / 1024) % 1024), 3);
            push_char('G');
            return *this;
        }
        else if (bytes > 1024 * 1024)
        {
            push_number((unsigned long long)(bytes / 1024 / 1024));
            push_char('.');
            push_number((unsigned long long)((bytes / 1024) % 1024), 3);
            push_char('M');
            return *this;
        }
        else if (bytes > 1024)
        {
            push_number((unsigned long long)(bytes / 1024));
            push_char('.');
            push_number((unsigned long long)(bytes % 1024), 3);
            push_char('K');
            return *this;
        }
        else
        {
            push_number((unsigned long long)bytes);
            push_char('B');
        }
        return *this;
    }

    inline ProfSerializer& ProfSerializer::push_char(char ch, int repeat)
    {
        while (repeat > 0 && offset_ < buff_len_)
        {
            buff_[offset_++] = ch;
            repeat--;
        }
        return *this;
    }

    inline ProfSerializer& ProfSerializer::push_number(unsigned long long number, int wide)
    {
        if (buff_len_ <= offset_ + 30)
        {
            return *this;
        }
        static const char* dec_lut =
            "00010203040506070809"
            "10111213141516171819"
            "20212223242526272829"
            "30313233343536373839"
            "40414243444546474849"
            "50515253545556575859"
            "60616263646566676869"
            "70717273747576777879"
            "80818283848586878889"
            "90919293949596979899";

        static const int buf_len = 30;
        char buf[buf_len];
        int write_index = buf_len;
        unsigned long long m1 = 0;
        unsigned long long m2 = 0;
        do
        {
            m1 = number / 100;
            m2 = number % 100;
            m2 += m2;
            number = m1;
            *(buf + write_index - 1) = dec_lut[m2 + 1];
            *(buf + write_index - 2) = dec_lut[m2];
            write_index -= 2;
        } while (number);
        if (buf[write_index] == '0')
        {
            write_index++;
        }
        while (buf_len - write_index < wide)
        {
            write_index--;
            buf[write_index] = '0';
        }
        memcpy(buff_ + offset_, buf + write_index, buf_len - write_index);
        offset_ += buf_len - write_index;
        return *this;
    }

    inline ProfSerializer& ProfSerializer::push_number(long long number, int wide)
    {
        if (buff_len_ <= offset_ + 30)
        {
            return *this;
        }
        if (number < 0)
        {
            buff_[offset_++] = '-';
            number *= -1;
            wide = wide > 1 ? wide - 1 : 0;
        }
        return push_number((unsigned long long)number, wide);
    }

    inline ProfSerializer& ProfSerializer::push_string(const char* str)
    {
        return push_string(str, strlen(str));
    }
    inline ProfSerializer& ProfSerializer::push_string(const char* str, size_t size)
    {
        if (str == NULL)
        {
            return *this;
        }
        size_t max_size = buff_len_ - offset_ > size ? size : buff_len_ - offset_;
        memcpy(buff_ + offset_, str, max_size);
        offset_ += max_size;
        return *this;
    }
    inline ProfSerializer& ProfSerializer::push_now_date()
    {
        time_t timestamp = 0;
        unsigned int precise = 0;
        do
        {
    #ifdef _WIN32
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            unsigned long long now = ft.dwHighDateTime;
            now <<= 32;
            now |= ft.dwLowDateTime;
            now /= 10;
            now -= 11644473600000000ULL;
            now /= 1000;
            timestamp = now / 1000;
            precise = (unsigned int)(now % 1000);
    #else
            struct timeval tm;
            gettimeofday(&tm, nullptr);
            timestamp = tm.tv_sec;
            precise = tm.tv_usec / 1000;
    #endif
        } while (0);

        struct tm tt = { 0 };
    #ifdef WIN32
        localtime_s(&tt, &timestamp);
    #else 
        localtime_r(&timestamp, &tt);
    #endif

        push_char('[');
        push_number((unsigned long long)tt.tm_year + 1900, 4);
        push_number((unsigned long long)tt.tm_mon + 1, 2);
        push_number((unsigned long long)tt.tm_mday, 2);
        push_char(' ');
        push_number((unsigned long long)tt.tm_hour, 2);
        push_char(':');
        push_number((unsigned long long)tt.tm_min, 2);
        push_char(':');
        push_number((unsigned long long)tt.tm_sec, 2);
        push_char('.');
        push_number((unsigned long long)precise, 3);
        push_char(']');
        return *this;
    }


    inline void ProfSerializer::closing_string()
    {
        size_t closed_id = offset_ >= buff_len_ ? buff_len_ - 1 : offset_;
        buff_[closed_id] = '\0';
    }

    inline ProfSerializer& ProfSerializer::push_indent(int count)
    {
        static const char* const pi = "                                                            ";
        constexpr int pi_size = 50;
        static_assert(pi_size >= PROF_NAME_MAX_SIZE, "");
        static_assert(pi_size >= PROF_MAX_DEPTH*2, "indent is two blank");
        if (count > pi_size)
        {
            count = pi_size;
        }
        if (count <= 0)
        {
            return *this;
        }
        if (offset_ + count >= buff_len_)
        {
            return *this;
        }
        memcpy(buff_ + offset_, pi, count);
        offset_ += count;
        return *this;
    }



    inline ProfSerializer& ProfSerializer::push_blank(int count)
    {
        static const char* const pi = "------------------------------------------------------------";
        constexpr int pi_size = 50;
        static_assert(pi_size >= PROF_NAME_MAX_SIZE, "");
        if (count > pi_size)
        {
            count = pi_size;
        }
        if (count <= 0)
        {
            return *this;
        }
        if (offset_ + count >= buff_len_)
        {
            return *this;
        }
        memcpy(buff_ + offset_, pi, count);
        offset_ += count;
        return *this;
    }

    class ProfStackSerializer : public ProfSerializer
    {
    public:
        static const int BUFF_SIZE = 350;
        static_assert(BUFF_SIZE > PROF_LINE_MIN_SIZE, "");
        ProfStackSerializer() :ProfSerializer(buff_, BUFF_SIZE)
        {
            buff_[0] = '\0';
        }
        ~ProfStackSerializer()
        {

        }

    private:
        char buff_[BUFF_SIZE];//1k  
    };

}


#endif
