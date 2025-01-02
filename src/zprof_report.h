
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

#ifndef ZPROF_REPORT_H_
#define ZPROF_REPORT_H_

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

    static constexpr int kProfNameMaxSize = 50;
    static constexpr int kProfDescMaxSize = 100;
    static constexpr int kProfLineMinSize = 200;
    static constexpr int kProfMaxDepth = 5;


    class Report
    {
    public:
        Report() = delete;
        explicit Report(char* buff, size_t buff_size)
        {
            buff_ = buff;
            buff_len_ = buff_size;
            offset_ = 0;
        }


        inline Report& PushHumanCount(long long count);
        inline Report& PushHumanTime(long long ns);
        inline Report& PushHumanMem(long long bytes);
        inline Report& PushChar(char ch, int repeat = 1);
        inline Report& PushString(const char* str);
        inline Report& PushString(const char* str, size_t size);
        inline Report& PushNowDate();
        inline Report& PushDate(long long date_ms);
        inline Report& PushNumber(unsigned long long number, int wide = 0);
        inline Report& PushNumber(long long number, int wide = 0);

        inline Report& PushIndent(int count);
        inline Report& PushBlank(int count);

        inline void ClosingString();
        bool IsFull() { return offset_ + 1 >= buff_len_; } //saved one char  

    public:
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




    inline Report& Report::PushHumanCount(long long count)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (count > 1000 * 1000)
        {
            PushNumber((unsigned long long)(count / 1000 / 1000));
            PushChar(',');
            PushNumber((unsigned long long)((count / 1000) % 1000), 3);
            PushChar(',');
            PushNumber((unsigned long long)(count % 1000), 3);
            return *this;
        }
        else if (count > 1000)
        {
            PushNumber((unsigned long long)(count / 1000));
            PushChar(',');
            PushNumber((unsigned long long)(count % 1000), 3);
            return *this;
        }
        return PushNumber((unsigned long long)(count));
    }

    inline Report& Report::PushHumanTime(long long ns)
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
            PushString("invalid");
            return *this;
        }
        PushNumber((long long)(ns/ fr));
        buff_[offset_++] = '.';
        PushNumber((unsigned long long)((ns/ mr) % 1000), 3);
        buff_[offset_++] = fc;
        buff_[offset_++] = lc;
        return *this;
    }


    inline Report& Report::PushHumanMem(long long bytes)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (bytes > 1024 * 1024 * 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024 / 1024 / 1024));
            PushChar('.');
            PushNumber((unsigned long long)((bytes / 1024 / 1024) % 1024), 3);
            PushChar('G');
            return *this;
        }
        else if (bytes > 1024 * 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024 / 1024));
            PushChar('.');
            PushNumber((unsigned long long)((bytes / 1024) % 1024), 3);
            PushChar('M');
            return *this;
        }
        else if (bytes > 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024));
            PushChar('.');
            PushNumber((unsigned long long)(bytes % 1024), 3);
            PushChar('K');
            return *this;
        }
        else
        {
            PushNumber((unsigned long long)bytes);
            PushChar('B');
        }
        return *this;
    }

    inline Report& Report::PushChar(char ch, int repeat)
    {
        while (repeat > 0 && offset_ < buff_len_)
        {
            buff_[offset_++] = ch;
            repeat--;
        }
        return *this;
    }

    inline Report& Report::PushNumber(unsigned long long number, int wide)
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

    inline Report& Report::PushNumber(long long number, int wide)
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
        return PushNumber((unsigned long long)number, wide);
    }

    inline Report& Report::PushString(const char* str)
    {
        return PushString(str, strlen(str));
    }
    inline Report& Report::PushString(const char* str, size_t size)
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

    inline Report& Report::PushDate(long long date_ms)
    {
        time_t ts = date_ms / 1000;
        unsigned int precise = (unsigned int)(date_ms % 1000);

        struct tm tt = { 0 };
#ifdef WIN32
        localtime_s(&tt, &ts);
#else 
        localtime_r(&ts, &tt);
#endif

        PushChar('[');
        PushNumber((unsigned long long)tt.tm_year + 1900, 4);
        PushNumber((unsigned long long)tt.tm_mon + 1, 2);
        PushNumber((unsigned long long)tt.tm_mday, 2);
        PushChar(' ');
        PushNumber((unsigned long long)tt.tm_hour, 2);
        PushChar(':');
        PushNumber((unsigned long long)tt.tm_min, 2);
        PushChar(':');
        PushNumber((unsigned long long)tt.tm_sec, 2);
        PushChar('.');
        PushNumber((unsigned long long)precise, 3);
        PushChar(']');
        return *this;
    }

    inline Report& Report::PushNowDate()
    {
        return PushDate(Clock<>::SystemNowMs());
    }


    inline void Report::ClosingString()
    {
        size_t closed_id = offset_ >= buff_len_ ? buff_len_ - 1 : offset_;
        buff_[closed_id] = '\0';
    }

    inline Report& Report::PushIndent(int count)
    {
        static const char* const pi = "                                                            ";
        constexpr int pi_size = 50;
        static_assert(pi_size >= kProfNameMaxSize, "");
        static_assert(pi_size >= kProfMaxDepth*2, "indent is two blank");
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



    inline Report& Report::PushBlank(int count)
    {
        static const char* const pi = "------------------------------------------------------------";
        constexpr int pi_size = 50;
        static_assert(pi_size >= kProfNameMaxSize, "");
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

    class StaticReport : public Report
    {
    public:
        static const int BUFF_SIZE = 350;
        static_assert(BUFF_SIZE > kProfLineMinSize, "");
        StaticReport() :Report(buff_, BUFF_SIZE)
        {
            buff_[0] = '\0';
        }
        ~StaticReport()
        {

        }

    private:
        char buff_[BUFF_SIZE];//1k  
    };

}


#endif
