
/*
* zmapping License
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

#pragma once
#ifndef _ZMAPPING_H_
#define _ZMAPPING_H_

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
#include <cstddef>

#ifdef WIN32

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
#include <sys/mman.h>
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
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif


using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;


//comment: grep -i commit /proc/meminfo   #查看overcommit value和当前commit   
//comment: sar -r											#查看kbcommit   %   
//comment: cat /proc/981/oom_score		#查看OOM分数  


//内存水位线
//watermark[min] = min_free_kbytes/4*zone.pages/zone.allpages    # cat /proc/sys/vm/min_free_kbytes    #32M    total/1000 左右,   通常在128k~65M之间  
//watermark[low] = watermark[min] * 5 / 4      
//watermark[high] = watermark[min] * 3 / 2

//当前水位线  cat /proc/zoneinfo | grep -E "Node|min|low|high "       #注意是页;  一般4k   每个zone都有自己的水位线   剩余内存超过HIGH代表内存剩余较多.  

//other   
//overcommit :   page table
//filesystem: page cache & buffer cache   
//kswapd: 水位线  
//内存不足会触发内存回收.   




class zmapping
{
private:
#ifdef WIN32
	HANDLE mapping_fd_;
	HANDLE mapping_obj_;
#else
	int mapping_fd_;
#endif // WIN32
	char* file_data_;
	u64   file_size_;

public:
	zmapping()
	{
#ifdef WIN32
		mapping_obj_ = NULL;
		mapping_fd_ = NULL;
#else
		mapping_fd_ = -1;
#endif 

		file_data_ = NULL;
		file_size_ = 0;
	}
	~zmapping()
	{
		unmap_res();
	}
	const char* data() { return file_data_; }
	u64 data_len() { return file_size_; }
	u64 file_size() { return file_size_; }

	s32 mapping_res(const char* file_path, bool remapping = false)
	{
		s32 ret = 0;
		if (remapping)
		{
			unmap_res();
		}
		ret = mapping_res_default(file_path, remapping);
		ret |= mapping_res_win32(file_path, remapping);
		return ret;
	}

	s32 mapping_res_default(const char* file_path, bool remapping)
	{
#ifndef WIN32	
		if (mapping_fd_ != -1)
		{
			return 1;
		}
		struct stat sb;
		mapping_fd_ = open(file_path, O_RDONLY);
		if (mapping_fd_ == -1)
		{
			return 10;
		}

		if (fstat(mapping_fd_, &sb) == -1)
		{
			close(mapping_fd_);
			mapping_fd_ = -1;
			return 11;
		}
		file_data_ = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, mapping_fd_, 0);
		file_size_ = sb.st_size;
#endif 
		return 0;
	}


	s32 mapping_res_win32(const char* file_path, bool remapping)
	{
#ifdef WIN32	
		if (mapping_fd_ != NULL)
		{
			return 1;
		}

		mapping_fd_ = ::CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (mapping_fd_ == INVALID_HANDLE_VALUE)
		{
			return 1;
		}
		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(mapping_fd_, &file_size))
		{
			::CloseHandle(mapping_fd_);
			mapping_fd_ = NULL;
			return 2;
		}

		mapping_obj_ = CreateFileMapping(mapping_fd_, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mapping_obj_ == NULL)
		{
			::CloseHandle(mapping_fd_);
			mapping_fd_ = NULL;
			return 2;
		}
		file_data_ = (char*)::MapViewOfFile(mapping_obj_, FILE_MAP_READ, 0, 0, 0);
		if (file_data_)
		{
			file_size_ = (u64)file_size.QuadPart;
		}
		else
		{
			::CloseHandle(mapping_fd_);
			::CloseHandle(mapping_obj_);
			mapping_fd_ = NULL;
			mapping_obj_ = NULL;
			return 3;
		}
#endif 
		return 0;
	}

	s32 is_mapped()
	{
#ifdef WIN32
		return mapping_fd_ != NULL;
#else
		return mapping_fd_ != -1;
#endif 
	}

	s32 unmap_res()
	{
		if (mapping_fd_ == 0)
		{
			return 1;
		}
#ifndef WIN32
		if (file_data_ != NULL && file_size_ != 0)
		{
			munmap(file_data_, file_size_);
		}
		close(mapping_fd_);
		file_data_ = NULL;
		file_size_ = 0;
		mapping_fd_ = -1;
#else
		if (file_data_)
		{
			::UnmapViewOfFile(file_data_);
		}
		::CloseHandle(mapping_obj_);
		::CloseHandle(mapping_fd_);
		file_data_ = NULL;
		file_size_ = 0;
		mapping_obj_ = NULL;
		mapping_fd_ = NULL;
#endif // 0
		return 0;
	}
};



#endif