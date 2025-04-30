

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



#pragma once 
#ifndef ZVECTOR_H
#define ZVECTOR_H

#include <stdint.h>
#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <cstring>

//default use format compatible short type .  
#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif 

//win & unix format incompatible   
#ifdef ZBASE_USE_AHEAD_TYPE
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
#endif

#ifdef ZBASE_USE_DEFAULT_TYPE
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
#endif






#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif


// init new memory with 0xfd    
//#define ZDEBUG_UNINIT_MEMORY

// backed memory immediately fill 0xdf 
//#define ZDEBUG_DEATH_MEMORY  

// open and check fence 
// no support 


template<class pointer, class reference, class value_type>
class zvector_iterator : public std::iterator<std::random_access_iterator_tag, value_type>
{
public:
    zvector_iterator() { p_ = NULL; }
    zvector_iterator(const pointer p) { p_ = p; }
    zvector_iterator(const zvector_iterator& iter) { p_ = iter.p_; }

    zvector_iterator& operator++() { ++p_; return *this; }
    zvector_iterator operator++(int) { zvector_iterator tmp(*this); ++p_; return tmp; }
    zvector_iterator& operator--() { --p_; return *this; }
    zvector_iterator operator--(int) { zvector_iterator tmp(*this); --p_; return tmp; }

    zvector_iterator operator+(int c) const { return zvector_iterator(p_ + c); }
    zvector_iterator operator-(int c) const { return zvector_iterator(p_ - c); }

    zvector_iterator& operator+=(int c) { p_ += c; return *this; }
    zvector_iterator& operator-=(int c) { p_ -= c; return *this; }

    size_t operator-(const zvector_iterator& iter) const { return (p_ - iter.p_); }

    bool operator<(const zvector_iterator& iter) const { return p_ < iter.p_; }
    bool operator<=(const zvector_iterator& iter) const { return p_ <= iter.p_; }
    bool operator>(const zvector_iterator& iter) const { return p_ > iter.p_; }
    bool operator>=(const zvector_iterator& iter) const { return p_ >= iter.p_; }
    bool operator==(const zvector_iterator& iter) const { return p_ == iter.p_; }
    bool operator!=(const zvector_iterator& iter) const { return p_ != iter.p_; }

    reference operator*() const { return *p_; }
    pointer operator ->() const { return p_; }

private:
    pointer p_;
};

/* type_traits:
*
* is_trivially_copyable: in part
    * memset: uninit or no dync heap
    * memcpy: uninit or no dync heap
* shm resume : safely, require heap address fixed
    * has vptr:     no
    * static var:   no
    * has heap ptr: yes
    * has code ptr: no
* thread safe: read safe
*
*/


// sso optimize vector; 
//used small data optimization; 
//merge high-frequency alloc small memory   

template<class _Ty>
using zvector_aligned_space_helper = typename std::conditional<std::is_trivial<_Ty>::value, _Ty, typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type>::type;

template<class _Ty, u32 _Size, u32 _FixedSize = _Size, class _Alloc = std::allocator<zvector_aligned_space_helper<_Ty>>>
class zvector
{
public:
    using value_type = _Ty;
    using size_type = u32;
    constexpr static size_type size_value = _Size;
    using difference_type = ptrdiff_t;
    using pointer = _Ty*;
    using const_pointer = const _Ty*;
    using reference = _Ty&;
    using const_reference = const _Ty&;
    using allocator_type = _Alloc;
public:
    static_assert(_FixedSize <= _Size, "");
    //using iterator = zvector_iterator<pointer, reference, value_type>;
    //using const_iterator = zvector_iterator<const_pointer, const_reference, value_type>;

    using iterator = _Ty*;
    using const_iterator = const _Ty*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using space_type = zvector_aligned_space_helper<_Ty>;
    using space_type_ptr = space_type*;
private:
    pointer ZBASE_ALIAS ptr(size_type i) const noexcept { return reinterpret_cast<pointer>(const_cast<space_type*>(&real_ptr_[i])); }
    reference ref(size_type i) const noexcept { return *ptr(i); }
    size_type distance(iterator l, iterator r) const noexcept { return (size_type)(r - l); }
public:
    zvector() 
    {
        real_ptr_ = &data_[0];
        count_ = 0; 
    }
    ~zvector() 
    { 
        clear(); 
        if (real_ptr_ != &data_[0])
        {
            alloc_.deallocate((space_type*)(real_ptr_), _Size);
            real_ptr_ = &data_[0];
        }
    }
    zvector(std::initializer_list<_Ty> init_list) : zvector()
    {
        check_realloc((size_type)init_list.size());
        count_ = (size_type)init_list.size();
        std::uninitialized_copy_n(init_list.begin(), count_, ptr(0));
    }
    zvector(const zvector< _Ty, _Size, _FixedSize, _Alloc>& other) : zvector()
    {
        check_realloc(other.size());
        count_ = other.size();
        std::uninitialized_copy_n(other.begin(), count_, ptr(0));
    }
    zvector(zvector< _Ty, _Size, _FixedSize, _Alloc>&& other) : zvector()
    {
        if (other.real_ptr_ == &other.data_[0])
        {
            count_ = other.size();
            std::uninitialized_copy_n(other.begin(), count_, ptr(0));
        }
        else
        {
            real_ptr_ = other.real_ptr_;
            other.real_ptr_ = &other.data_[0];
            count_ = other.count_;
            other.count_ = 0;
        }
    }


    //std::array api
    iterator begin() noexcept { return ptr(0); }
    const_iterator begin() const noexcept { return ptr(0); }
    const_iterator cbegin() const noexcept { return ptr(0); }

    iterator end() noexcept { return ptr(0) + count_; }
    const_iterator end() const noexcept { return ptr(0) + count_; }
    const_iterator cend() const noexcept { return ptr(0) + count_; }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    reference at(size_type pos) { return ref(pos); }
    const_reference at(size_type pos) const { return ref(pos); }
    reference operator[](size_type pos) { return ref(pos); }
    const_reference operator[](size_type pos) const { return ref(pos); }

    reference front() { return *begin(); }
    const_reference front() const { return *begin(); }
    reference back() { return *ptr(count_ - 1); }
    const_reference back() const { return *ptr(count_ - 1);}

    pointer data() noexcept { return ptr(0); }
    const_pointer data() const noexcept { return ptr(0); }

    constexpr size_type capacity() const { return _Size; }
    constexpr size_type max_size()  const noexcept { return _Size; }
    const size_type size() const noexcept { return count_; }
    const bool empty() const noexcept { return begin() == end(); }
    const bool full() const noexcept { return size() == max_size(); }

    iterator find(const _Ty& v) noexcept { return std::find(begin(), end(), v); }
    const_iterator find(const _Ty& v) const noexcept { return std::find(begin(), end(), v); }
    reverse_iterator rfind(const _Ty& v) noexcept { return std::find(rbegin(), rend(), v); }
    const_reverse_iterator rfind(const _Ty& v) const noexcept { return std::find(rbegin(), rend(), v); }


    iterator check_realloc(size_type expect, iterator iter_pos)
    {
        if (real_ptr_ != &data_[0])
        {
            return iter_pos;
        }
        if (expect <= _FixedSize)
        {
            return iter_pos;
        }
        size_type idx = (size_type)(iter_pos - ptr(0));
        space_type_ptr tmp_ptr = (space_type_ptr)alloc_.allocate(_Size);
        std::uninitialized_copy_n(ptr(0), count_, reinterpret_cast<pointer>(tmp_ptr));  
        if (!std::is_trivial<_Ty>::value)
        {
            _Ty* pbegin = ptr(0);
            for (size_type i = 0; i < count_; i++)
            {
                pbegin[i].~_Ty();
            }
        }
        real_ptr_ = tmp_ptr;
        return ptr(0) + idx;
    }

    void check_realloc(size_type expect)
    {
        (void)check_realloc(expect, ptr(0));
    }


    void fill(const _Ty& value, size_type fill_count = _Size)
    {
        check_realloc(fill_count);
        for (size_type i = 0; i < _Size && i < count_; i++)
        {
            ref(i) = value;
        }
#ifdef ZDEBUG_UNINIT_MEMORY
        if (_Size > count_)
        {
            memset(ptr(count_), 0xfd, (_Size - count_) * sizeof(_Ty));
        }
#endif
        for (size_type i = count_; i < _Size; i++)
        {
            new (ptr(i)) _Ty(value);
        }
        count_ = _Size;
    }

    void clear()
    {
        if (!std::is_trivial<_Ty>::value)
        {
            _Ty* pbegin = ptr(0);
            for (size_type i = 0; i < count_; i++)
            {
                pbegin[i].~_Ty();
            }
        }
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(data_, 0xfd, _FixedSize * sizeof(_Ty));
        if (real_ptr_ != &data_[0])
        {
            memset(ptr(0), 0xfd, max_size() * sizeof(_Ty));
        }
#endif 
        count_ = 0;
    }

    template<class T = _Ty>
    void push_back(const typename std::enable_if<std::is_trivial<T>::value, T>::type& value)
    {
        if (full())
        {
            return;
        }
        check_realloc(count_ + 1);
        ref(count_++) = value;
    }

    template<class T = _Ty>
    void push_back(const typename std::enable_if<!std::is_trivial<T>::value, T>::type & value)
    {
        if (full())
        {
            return;
        }
        check_realloc(count_ + 1);
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(ptr(count_), 0xfd, sizeof(_Ty));
#endif // ZDEBUG_UNINIT_MEMORY
        new (ptr(count_++)) _Ty(value);
    }

    template<class T = _Ty>
    void pop_back(const typename std::enable_if<std::is_trivial<T>::value>::type*  = 0)
    {
#ifdef ZDEBUG_DEATH_MEMORY
        memset(ptr(count_ - 1), 0xdf, sizeof(_Ty));
#endif  
        count_--;
    }

    template<class T = _Ty>
    void pop_back(const typename std::enable_if<!std::is_trivial<T>::value>::type*  =0)
    {
        ptr(count_ - 1)->~_Ty();
#ifdef ZDEBUG_DEATH_MEMORY
        memset(ptr(count_ - 1), 0xdf, sizeof(_Ty));
#endif  
        count_--;
    }

    template<class T = _Ty>
    iterator inject(iterator in_pos, size_type count, const typename std::enable_if<std::is_trivial<T>::value>::type* = 0)
    {
        in_pos = check_realloc(count_ + count, in_pos);
        iterator pos = in_pos;
        iterator old_end = end();
        count_ += count;
        if (pos == old_end)
        {
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&*pos, 0xfd, count * sizeof(_Ty));
#endif 
            return pos;
        }
        memmove((space_type*)&*in_pos + count, (space_type*)&*in_pos, sizeof(space_type) * (old_end - in_pos));
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(&*pos, 0xfd, count * sizeof(_Ty));
#endif 
        return pos;
    }
    template<class T = _Ty>
    iterator inject(iterator in_pos, size_type count, const typename std::enable_if<!std::is_trivial<T>::value>::type* = 0)
    {
        in_pos = check_realloc(count_ + count, in_pos);
        iterator pos = in_pos;
        iterator old_end = end();
        count_ += count;
        if (pos == old_end)
        {
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(&*pos, 0xfd, count * sizeof(_Ty));
#endif 
            return pos;
        }
        iterator new_end = end();
        iterator src = old_end;
        iterator target = new_end;
        while (src != pos && target != old_end)
        {
            new (&*(--target)) _Ty(*(--src));
        }

        while (src != pos)
        {
            *(--target) = *(--src);
        }

        src = pos;
        target = pos + count;
        while (src != target)
        {
            src++->~_Ty();
        }
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(&*pos, 0xfd, count * sizeof(_Ty));
#endif 
        return pos;
    }

    //[first,last)
    template<class T = _Ty>
    iterator erase(iterator first, iterator last, const typename std::enable_if<std::is_trivial<T>::value>::type* = 0)
    {
        if (first >= end() || first < begin())
        {
            return end();
        }

        if (last >= end())
        {
            count_ -= distance(first, end());
            return end();
        }
        size_type island_count = distance(last, end());
        memmove((space_type*)&*first, (space_type*)&*last, island_count * sizeof(space_type));
        iterator new_end = (iterator)(first + island_count);
#ifdef ZDEBUG_DEATH_MEMORY
        memset(&*new_end, 0xdf, distance(new_end, end()) * sizeof(_Ty));
#endif // ZDEBUG_DEATH_MEMORY
        count_ -= distance(new_end, end());
        return end();
    }
    template<class T = _Ty>
    iterator erase(iterator first, iterator last, const typename std::enable_if<!std::is_trivial<T>::value>::type* = 0)
    {
        if (first >= end() || first < begin())
        {
            return end();
        }

        size_type island_count = distance(last, end());
        iterator cp_first = first;
        iterator cp_last = last;
        for (size_type i = 0; i < island_count; i++)
        {
            *cp_first++ = *cp_last++;
        }
        iterator erase_first = cp_first;
        while (erase_first != end())
        {
            erase_first->~_Ty();
            ++erase_first;
        }
#ifdef ZDEBUG_DEATH_MEMORY
        memset(&*cp_first, 0xdf, distance(cp_first, end()) * sizeof(_Ty));
#endif // ZDEBUG_DEATH_MEMORY

        count_ -= distance(cp_first, end());
        return end();
    }

    iterator erase(iterator pos)
    {
        return erase(pos, pos + 1);
    }

    template<class T = _Ty>
    iterator insert(iterator pos, size_type count, const typename std::enable_if <std::is_trivial<T>::value, _Ty>::type& value)
    {
        if (pos < begin() || pos > end() || count_ + count > max_size() || count == 0)
        {
            return end();
        }
        iterator new_iter = inject(pos, count);
        pos = new_iter;
        for (size_t i = 0; i < count; i++)
        {
            *pos++ = value;
        }
        return new_iter;
    }

    template<class T = _Ty>
    iterator insert(iterator pos, size_type count, const typename std::enable_if <!std::is_trivial<T>::value, _Ty>::type& value)
    {
        if (pos < begin() || pos > end() || count_ + count > max_size() || count == 0)
        {
            return end();
        }
        iterator new_iter = inject(pos, count);
        pos = new_iter;
        for (size_t i = 0; i < count; i++)
        {
            new (pos++) _Ty(value);
        }
        return new_iter;
    }

    template<class T = _Ty>
    iterator insert(iterator pos, const typename std::enable_if <std::is_trivial<T>::value, _Ty>::type& value)
    {
        if (pos < begin() || pos > end() || full())
        {
            return end();
        }
        iterator new_iter = inject(pos, 1);
        pos = new_iter;
        *pos++ = value;
        return new_iter;
    }

    template<class T = _Ty>
    iterator insert(iterator pos, const typename std::enable_if <!std::is_trivial<T>::value, _Ty>::type& value)
    {
        if (pos < begin() || pos > end() || full())
        {
            return end();
        }
        iterator new_iter = inject(pos, 1);
        pos = new_iter;
        new (&* (pos++)) _Ty(value);
        return new_iter;
    }

    template<class Iter>
    iterator assign(Iter first, Iter last)
    {
        clear();
        check_realloc((size_type)std::distance(first, last));
        iterator pos = begin();
        if (!std::is_trivial< _Ty>::value)
        {
            while (first != last && count_ < max_size())
            {
                new (&*(pos++)) _Ty(*first++);
                count_++;
            }
        }
        else
        {
            while (first != last && count_ < max_size())
            {
                *pos++ = *first++;
                count_++;
            }
        }
        return begin();
    }

    iterator assign(size_type count, const _Ty& value)
    {
        clear();
        return insert(end(), count, value);
    }

    template< class... Args >
    iterator emplace(iterator pos, Args&&... args)
    {
        if (pos < begin() || pos > end() || count_ + 1 > max_size() || 1 == 0)
        {
            return end();
        }
        iterator iter = inject(pos, 1);
        new (&*iter) _Ty(args...);
        return iter;
    }

    template< class... Args >
    iterator emplace_back(Args&&... args)
    {
        return emplace(end(), args...);
    }

    zvector< _Ty, _Size, _FixedSize, _Alloc>& operator=(const zvector< _Ty, _Size, _FixedSize, _Alloc>& other)
    {
        if (this == &other)
        {
            return *this;
        }
        check_realloc(other.size());
        if (size() <= other.size())
        {
            std::copy_n(other.begin(), count_, ptr(0));
            std::uninitialized_copy_n(other.begin() + count_, other.size() - count_, ptr(count_));
            count_ = other.count_;
            return *this;
        }
        std::copy_n(other.begin(), other.size(), ptr(0));
        for (size_type i = other.size(); i < count_; i++)
        {
            ptr(i)->~_Ty();
        }
        count_ = other.size();
        return *this;
    }

    zvector< _Ty, _Size, _FixedSize, _Alloc>& operator=(zvector< _Ty, _Size, _FixedSize, _Alloc>&& other)
    {
        if (this == &other)
        {
            return *this;
        }

        // clear this and reset memory   
        if (real_ptr_ != &data_[0] || !std::is_trivial<_Ty>::value)
        {
            clear();
            if (real_ptr_ != data_)
            {
                alloc_.deallocate((space_type*)(real_ptr_), _Size);
                real_ptr_ = &data_[0];
            }
        }

        if (other.real_ptr_ != &other.data_[0])
        {
            real_ptr_ = other.real_ptr_;
            count_ = other.count_;
            other.real_ptr_ = other.data_;
            other.count_ = 0;
            return *this;
        }

        //assert: other size is less than Fixed size. ref real_ptr   
        std::uninitialized_copy_n(other.begin(), other.size(), ptr(0));
        count_ = other.count_;
        return *this;
    }

private://gdb view 
    //using real_type = _Ty[_Size];
    //__attribute__((noinline))  const real_type* real_ptr()const { return (real_type*)real_ptr_; }
private:
    space_type data_[_FixedSize > 0? _FixedSize : 1];
    space_type_ptr real_ptr_;
    size_type count_;
    allocator_type alloc_;
};


template<class _Ty, u32 _Size, u32 _FixedSize, class _Alloc>
bool operator==(const zvector< _Ty, _Size, _FixedSize, _Alloc>& a1, const zvector< _Ty, _Size, _FixedSize, _Alloc>& a2)
{
    return a1.data() == a2.data();
}



#endif