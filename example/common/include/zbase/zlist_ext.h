
/*
* zlist_ext License
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



#ifndef  ZLIST_EXT_H
#define ZLIST_EXT_H
#include <iterator>
#include <memory>
#include <cstddef>


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

//#define ZLIST_EXT_USED_FENCE

#if __GNUG__
#define MAY_ALIAS __attribute__((__may_alias__))
#else
#define MAY_ALIAS
#endif

template<class list_type>
struct const_zlist_ext_iterator;

template<class list_type>
struct zlist_ext_iterator : public std::iterator<std::bidirectional_iterator_tag, typename list_type::value_type>
{
    using node_type = typename list_type::node_type;
    using difference_type = typename list_type::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename list_type::value_type;
    using pointer = typename list_type::pointer;
    using reference = typename list_type::reference;

    zlist_ext_iterator(const node_type* const head, u32 id) { head_ = const_cast<node_type*>(head); id_ = id; }
    zlist_ext_iterator(const zlist_ext_iterator<list_type>& other) { head_ = const_cast<node_type*>(other.head_); id_ = other.id_; }
    zlist_ext_iterator(const const_zlist_ext_iterator<list_type>& other) { head_ = const_cast<node_type*>(other.head_); id_ = other.id_; }
    zlist_ext_iterator() :zlist_ext_iterator(NULL, 0) {}
    zlist_ext_iterator& operator ++()
    {
        id_ = (head_ + id_)->next;
        return *this;
    }
    zlist_ext_iterator& operator --()
    {
        id_ = (head_ + id_)->front;
        return *this;
    }
    zlist_ext_iterator operator ++(int)
    {
        zlist_ext_iterator old = *this;
        id_ = (head_ + id_)->next;
        return old;
    }
    zlist_ext_iterator operator --(int)
    {
        zlist_ext_iterator old = *this;
        id_ = (head_ + id_)->front;
        return old;
    }

    pointer operator ->() const
    {
        return list_type::node_cast(*(head_ + id_));
    }
    reference operator *() const
    {
        return *list_type::node_cast(*(head_ + id_));
    }

public:
    node_type* head_;
    u32 id_;
};
template<class list_type>
bool operator == (const zlist_ext_iterator<list_type>& n1, const zlist_ext_iterator<list_type>& n2)
{
    return n1.head_ == n2.head_ && n1.id_ == n2.id_;
}
template<class list_type>
bool operator != (const zlist_ext_iterator<list_type>& n1, const zlist_ext_iterator<list_type>& n2)
{
    return !(n1 == n2);
}


//分段双向链表; 第一段为固定内存, 第二段为动态内存, 均为定长.  
//_Size == _FixedSize 大小相等时为全静态, 此时与zlist的区别在于, zlist的node和value绑在一起, value小时 zlist因不需要取指针性能更好,  value大时 zlist_ext因分离数据性能会更好一些.  
//_FixedSize == 0 时为全动态   
//这里使用了指针, 用在共享内存时候需要保证指针地址固定, 以及修改动态内存分配接口,.   
template<class _Ty, size_t _Size, size_t _FixedSize, class _Alloc = std::allocator<typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type>>
class zlist_ext
{
public:
    struct node_type;
    using value_type = _Ty;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = _Ty*;
    using const_pointer = const _Ty*;
    using reference = _Ty&;
    using const_reference = const _Ty&;
    using allocator_type = _Alloc;

    static const u32 FENCE_VAL = 0xdeadbeaf;
    static const u64 MAX_SIZE = _Size;
    static const u64 LIST_SIZE = _Size + 1;
    static const u64 END_ID = _Size;
    static_assert(_Size > 0, "");
    static_assert(_FixedSize > 0, "");
    static_assert(_FixedSize <= _Size, "");

    using iterator = zlist_ext_iterator<zlist_ext<_Ty, _Size, _FixedSize, _Alloc>>;
    using const_iterator = iterator;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using space_type = typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type;

public:
    struct node_type
    {
#ifdef ZLIST_EXT_USED_FENCE
        u32 fence;
#endif
        u32 front;
        u32 next;
        space_type *space;
    };
    static _Ty* MAY_ALIAS node_cast(node_type& node) { return reinterpret_cast<_Ty*>(node.space); }
public:

    zlist_ext()
    {
        init();
    }
    ~zlist_ext()
    {
        if (!std::is_trivial<_Ty>::value)
        {
            clear();
        }
        if (dync_space_ != NULL)
        {
            alloc_.deallocate(dync_space_, _Size - _FixedSize);
            dync_space_ = NULL;
        }
    }

    zlist_ext(std::initializer_list<_Ty> init_list)
    {
        init();
        assign(init_list.begin(), init_list.end());
    }

    zlist_ext(const zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& other)
    {
        if (data() == other.data())
        {
            return;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_)
        {
            init(other);
        }
        else
        {
            init();
            assign(other.begin(), other.end());
        }
    }

    zlist_ext(zlist_ext<_Ty, _Size, _FixedSize, _Alloc>&& other)
    {
        if (data() == other.data())
        {
            return;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_)
        {
            plat_init(std::move(other));
        }
        else
        {
            init();
            assign(other.begin(), other.end());
        }
    }

    zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& operator = (const zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& other)
    {
        if (data() == other.data())
        {
            return *this;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_)
        {
            clear();
            plat_init(other);
        }
        else
        {
            clear();
            assign(other.begin(), other.end());
        }
        return *this;
    }
    zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& operator = (zlist_ext<_Ty, _Size, _FixedSize, _Alloc>&& other)
    {
        if (data() == other.data())
        {
            return *this;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_)
        {
            clear();
            plat_init(std::move(other));
        }
        else
        {
            clear();
            assign(other.begin(), other.end());
        }
        return *this;
    }



    //std::array api
    iterator begin() noexcept { return iterator(&data_[0], used_head_id_); }
    const_iterator begin() const noexcept { return const_iterator(&data_[0], used_head_id_); }
    const_iterator cbegin() const noexcept { return const_iterator(&data_[0], used_head_id_); }

    iterator end() noexcept { return iterator(&data_[0], END_ID); }
    const_iterator end() const noexcept { return const_iterator(&data_[0], END_ID); }
    const_iterator cend() const noexcept { return const_iterator(&data_[0], END_ID); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }


    reference front() { return *node_cast(data_[used_head_id_]); }
    const_reference front() const { return *node_cast(data_[used_head_id_]); }
    reference back() { return *node_cast(data_[data_[END_ID].front]); }
    const_reference back() const { *node_cast(data_[data_[END_ID].front]); }

//       static constexpr u32 static_buf_size(u32 obj_count) { return sizeof(zlist_ext<_Ty, 1>) + sizeof(node_type) * (obj_count - 1); }


    const size_type size() const noexcept { return used_count_; }
    const size_type max_size()  const noexcept { return MAX_SIZE; }
    const bool empty() const noexcept { return !used_count_; }
    const bool full() const noexcept { return free_id_ == END_ID && exploit_offset_ >= END_ID; }
    size_type capacity() const { return max_size(); }
    const node_type* data() const noexcept { return data_; }
    const bool is_valid_node(void* addr) const noexcept
    {
        u64 ufixed_addr = (u64)&fixed_space_[0];
        u64 udyn_addr = (u64)dync_space_;
        u64 uaddr = (u64)addr;
        if (uaddr >= ufixed_addr && uaddr < ufixed_addr + sizeof(space_type) * _FixedSize)
        {
            return true;
        }
        if (udyn_addr != 0)
        {
            if (uaddr >= udyn_addr && uaddr < udyn_addr + sizeof(space_type) * (_Size - _FixedSize))
            {
                return true;
            }
        }
        return false;
    }

    void clear()
    {
        u32 node_id = used_head_id_;
        if (!std::is_trivial<_Ty>::value)
        {
            while (node_id != END_ID)
            {
                node_cast(data_[node_id])->~_Ty();
                node_id = data_[node_id].next;
            }
        }
        space_type *ds = dync_space_ ;
        init();
        dync_space_ = ds;
    }

    void fill(const _Ty& value)
    {
        clear();
        insert(end(), max_size(), value);
    }


private:
    void init()
    {
        used_count_ = 0;
        free_id_ = END_ID;
        exploit_offset_ = 0;
        data_[END_ID].next = END_ID;
        data_[END_ID].front = END_ID;
#ifdef ZLIST_EXT_USED_FENCE
        data_[END_ID].fence = FENCE_VAL;
#endif
        data_[END_ID].space = NULL;
        used_head_id_ = END_ID;
        dync_space_ = NULL;
    }
    void plat_init(const zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& temp)
    {
        used_count_ = temp.used_count_;
        free_id_ = temp.free_id_;
        exploit_offset_ = temp.exploit_offset_;
        used_head_id_ = temp.used_head_id_;
        
        data_[END_ID] = temp.data_[END_ID];
        memcpy(data_, temp.data_, temp.exploit_offset_ * sizeof(node_type));

        u32 copy_size = exploit_offset_ < _FixedSize ? exploit_offset_ : _FixedSize;
        memcpy(fixed_space_, temp.fixed_space_, copy_size * sizeof(space_type));
        for (u32 i = 0; i < copy_size; i++)
        {
            data_[i].space = &fixed_space_[i];
        }

        dync_space_ = NULL;
        if (temp.dync_space_ && temp.exploit_offset_ > _FixedSize)
        {
            dync_space_ = alloc_.allocate(_Size - _FixedSize);
            memcpy(dync_space_, temp.dync_space_, (temp.exploit_offset_ - _FixedSize) * sizeof(space_type));
            for (u32 i = _FixedSize; i < temp.exploit_offset_ ; i++)
            {
                data_[i].space = &dync_space_[i - _FixedSize];
            }
        }

    }

    void plat_init(zlist_ext<_Ty, _Size, _FixedSize, _Alloc>&& temp)
    {
        used_count_ = temp.used_count_;
        free_id_ = temp.free_id_;
        exploit_offset_ = temp.exploit_offset_;
        used_head_id_ = temp.used_head_id_;
        dync_space_ = temp.dync_space_;
        data_[END_ID] = temp.data_[END_ID];
        memcpy(data_, temp.data_, temp.exploit_offset_ * sizeof(node_type));
        

        u32 copy_size = exploit_offset_ < _FixedSize ? exploit_offset_ : _FixedSize;
        memcpy(fixed_space_, temp.fixed_space_, copy_size * sizeof(space_type));
        for (u32 i = 0; i < copy_size; i++)
        {
            data_[i].space = &fixed_space_[i];
        }
        /*
        if (temp.dync_space_ && temp.exploit_offset_ > _FixedSize)
        {
            for (u32 i = _FixedSize; i < temp.exploit_offset_; i++)
            {
                data_[i].space = &dync_space_[i - _FixedSize];
            }
        }
        */
        temp.init();
    }


    bool push_free_node(u32 id)
    {
        node_type& node = data_[id];
#ifdef ZDEBUG_DEATH_MEMORY
        memset(node.space, 0xfd, sizeof(space_type));
#endif // ZDEBUG_DEATH_MEMORY
        node.next = free_id_;
        node.front = END_ID;
        free_id_ = id;
        return true;
    }
    u32 pop_free_node()
    {
        if (free_id_ != END_ID)
        {
            u32 new_id = free_id_;
            free_id_ = data_[new_id].next;
            return new_id;
        }
        if (exploit_offset_ < END_ID)
        {
            u32 new_id = exploit_offset_++;
#ifdef ZLIST_EXT_USED_FENCE
            data_[new_id].fence = FENCE_VAL;
            data_[new_id + 1].fence = FENCE_VAL;
#endif
            if (new_id < _FixedSize)
            {
                data_[new_id].space = &fixed_space_[new_id];
                return new_id;
            }
            if (dync_space_ == NULL)
            {
                dync_space_ = alloc_.allocate(_Size - _FixedSize);
                //dync_space_ = new space_type[_Size - _FixedSize];
            }
            data_[new_id].space = &dync_space_[new_id - _FixedSize];
            return new_id;
        }
        return END_ID;
    }

    bool pick_used_node(u32 id)
    {
        if (id >= END_ID)
        {
            return false;
        }
#ifdef ZLIST_EXT_USED_FENCE
        if (data_[id + 1].fence != FENCE_VAL)
        {
            return false;
        }
#endif
        node_type& node = data_[id];
#ifdef ZLIST_EXT_USED_FENCE
        if (node.fence != FENCE_VAL)
        {
            return false;
        }
#endif
        if (used_head_id_ >= END_ID)
        {
            return false; //empty
        }
        if (!std::is_trivial<_Ty>::value)
        {
            node_cast(node)->~_Ty();
        }

        if (used_head_id_ == id)
        {
            used_head_id_ = node.next;
            data_[used_head_id_].front = END_ID;
        }
        else
        {
            data_[node.front].next = node.next;
            data_[node.next].front = node.front;
        }
        used_count_--;
        return true;
    }
    bool pick_and_release_used_node(u32 id)
    {
        if (pick_used_node(id))
        {
            return push_free_node(id);
        }
        //LogError() << "release used node error";
        return false;
    }
    void inject(u32 pos_id, u32 new_id)
    {
        data_[new_id].next = pos_id;
        data_[new_id].front = data_[pos_id].front;
        if (pos_id == used_head_id_)
        {
            used_head_id_ = new_id;
        }
        else
        {
            data_[data_[pos_id].front].next = new_id;
        }

        data_[pos_id].front = new_id;
        used_count_++;
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(data_[new_id].space, 0xfd, sizeof(space_type));
#endif // ZDEBUG_UNINIT_MEMORY
    }
    u32 inject(u32 id, const _Ty& value)
    {
        u32 new_id = pop_free_node();
        if (new_id == END_ID)
        {
            return END_ID;
        }
        inject(id, new_id);
        new (data_[new_id].space) _Ty(value);
        return new_id;
    }
    template< class... Args >
    u32 inject_emplace(u32 id, Args&&... args)
    {
        u32 new_id = pop_free_node();
        if (new_id == END_ID)
        {
            return END_ID;
        }
        inject(id, new_id);
        new (data_[new_id].space) _Ty(args ...);
        return new_id;
    }
public:
    iterator insert(iterator pos, const _Ty& value)
    {
        return iterator(&data_[0], inject(pos.id_, value));
    }

    template< class... Args >
    iterator emplace(iterator pos, Args&&... args)
    {
        return iterator(&data_[0], inject_emplace(pos.id_, args...));
    }

    iterator insert(iterator pos, size_type count, const _Ty& value)
    {
        if (used_count_ + count > max_size())
        {
            return end();
        }
        if (count == 0)
        {
            return end();
        }
        for (size_t i = 0; i < count; i++)
        {
            pos.id_ = inject(pos.id_, value);
        }
        return pos;
    }

    //[first, last)
    template<class other_iterator>
    iterator insert(iterator pos, other_iterator first, other_iterator last)
    {
        if (first == last)
        {
            return end();
        }
        --last;
        while (first != last)
        {
            pos = insert(pos, *(last--));
        }
        pos = insert(pos, *last);
        return pos;
    }

    template<class other_iterator>
    iterator assign(other_iterator first, other_iterator last)
    {
        clear();
        return insert(end(), first, last);
    }


    void push_back(const _Ty& value) { inject(END_ID, value); }
    bool pop_back() { return pick_and_release_used_node(data_[END_ID].front); }
    void push_front(const _Ty& value) { inject(used_head_id_, value); }
    bool pop_front() { return pick_and_release_used_node(used_head_id_); }

    iterator erase(iterator pos)
    {
        u32 pos_id = pos.id_;
        pos++;
        if (!pick_and_release_used_node(pos_id))
        {
            pos = end();
        }
        return pos;
    }

    //[first,last)
    iterator erase(iterator first, iterator last)
    {
        while (first != last)
        {
            erase(first++);
        }
        return last;
    }


    template< class... Args >
    iterator emplace_back(Args&&... args)
    {
        return emplace(end(), args...);
    }
    template< class... Args >
    iterator emplace_front(Args&&... args)
    {
        return emplace(begin(), args...);
    }

private:
    template<class Comp>
    iterator comp_bound(iterator first, iterator last, const _Ty& value, Comp comp)
    {
        while (first != last)
        {
            if (comp(*first, value))
            {
                first++;
                continue;
            }
            break;
        }
        return first;
    }
public:
    template<class Less = std::less<_Ty>>
    iterator lower_bound(iterator first, iterator last, const _Ty& value, Less less = Less())
    {
        return comp_bound(first, last, value, less);
    }
    template<class Greater = std::greater<_Ty>>
    iterator upper_bound(iterator first, iterator last, const _Ty& value, Greater greater = Greater())
    {
        return comp_bound(first, last, value, greater);
    }
private:
    u32 used_count_;
    u32 free_id_;
    u32 exploit_offset_;
    u32 used_head_id_;
    allocator_type alloc_;
    node_type data_[LIST_SIZE];
    space_type fixed_space_[_FixedSize > 0? _FixedSize : 1];
    space_type* dync_space_;// space_type dync_space_[Size - _FixedSize];_
};

template<class _Ty, size_t _Size, size_t _FixedSize, class _Alloc>
bool operator==(const zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& a1, const zlist_ext<_Ty, _Size, _FixedSize, _Alloc>& a2)
{
    return a1.data() == a2.data();
}




#endif