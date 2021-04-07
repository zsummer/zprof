
/*
* zhash_map License
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

#include "base_def.h"
#include "fn_log.h"

#ifndef  ZHASH_MAP_H
#define ZHASH_MAP_H


namespace zsummer
{
    namespace shm_arena
    {
        template<class Bucket, class Key, class _Ty, size_t _Size>
        struct HashMapIterator
        {
            using value_type = std::pair<const Key, _Ty>;
            Bucket*  bucket_;
            Bucket** buckets_;
            Bucket*  end_bucket_;
            operator Bucket* () const { return bucket_; }
            operator const Bucket* ()const { return bucket_; }
            Bucket* next()
            {
                if (bucket_ == end_bucket_)
                {
                    return bucket_;
                }
                if (bucket_ && bucket_->next && bucket_->next != end_bucket_ && bucket_->next != bucket_[bucket_->bid])
                {
                    return bucket_->next;
                }
                u32 bid = bucket_->bid + 1;
                while (bid < _Size)
                {
                    if (bucket_[bid])
                    {
                        return bucket_[bid];
                    }
                    bid++;
                }
                return end_bucket_;
            }
            
            HashMapIterator& operator ++()
            {
                bucket_ = next();
                return *this;
            }
            HashMapIterator operator ++(int)
            {
                Bucket* old = bucket_;
                bucket_ = next();
                return old;
            }

            value_type* operator ->()
            {
                return (value_type*)&bucket_->val;
            }
            value_type& operator *()
            {
                return bucket_->val;
            }
            HashMapIterator()
            {
                bucket_ = NULL;
                buckets_ = NULL;
                end_bucket_ = NULL;
            }
            HashMapIterator(Bucket* bucket, Bucket** buckets, Bucket* end_bucket) 
            { 
                bucket_ = bucket; 
                buckets_ = buckets; 
                end_bucket_ = end_bucket;
            }
            HashMapIterator(const HashMapIterator& other)
            {
                bucket_ = other.bucket_;
                buckets_ = other.buckets_;
                end_bucket_ = other.end_bucket_;
            }
        };
        template<class Bucket, class Key, class _Ty, size_t _Size>
        bool operator == (const HashMapIterator<Bucket, Key, _Ty, _Size>& n1, const HashMapIterator<Bucket, Key, _Ty, _Size>& n2)
        {
            return (Bucket*)n1 == (Bucket*)n2;
        }
        template<class Bucket, class Key, class _Ty, size_t _Size>
        bool operator != (const HashMapIterator<Bucket, Key, _Ty, _Size>& n1, const HashMapIterator<Bucket, Key, _Ty, _Size>& n2)
        {
            return !(n1 == n2);
        }

        template<class Key,
            class _Ty,
            size_t _Size,
            class Hash = std::hash<Key>,
            class KeyEqual = std::equal_to<Key>>
        class zhash_map
        {
        public:
            struct bucket
            {
                u32 fence;
                u32 bid;
                size_t ukey;
                bucket* next;
                std::pair<Key, _Ty> val;
            };
            using bucket_pointer = bucket*;

            

        public:
            
            using key_type = Key;
            using mapped_type = _Ty;
            using value_type = std::pair<const key_type, mapped_type>;
            using reference = value_type&;
            using const_reference = const value_type&;
            using iterator = HashMapIterator<bucket, key_type, mapped_type, _Size>;
            using const_iterator = const iterator;
            using size_type = size_t;
            static const size_type free_bucket_count_ = _Size*2;

            constexpr size_type max_size() { return _Size; }
            constexpr size_type bucket_count() { return free_bucket_count_; }
            constexpr size_type max_bucket_count() { return free_bucket_count_; }
            static const u32 fence_val = 0xdeadbeaf;
            
            Hash hasher;
            KeyEqual key_equal;
        private:
            bucket_pointer buckets_[free_bucket_count_];
            bucket end_bucket_;
            bucket free_buckets_[free_bucket_count_];
            bucket_pointer free_head_;
            size_t count_;
            iterator mi(bucket* b) { return iterator(b, buckets_, &end_bucket_); }
            void reset()
            {
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
                memset(buckets_, 0, sizeof(buckets_));
                memset(&end_bucket_, 0, sizeof(end_bucket_));
#if __GNUG__ && __GNUC__ >= 5
#pragma GCC diagnostic pop
#endif

                end_bucket_.next = &end_bucket_;
                free_head_ = &free_buckets_[0];
                for (size_t i = 0; i < free_bucket_count_; i++)
                {
                    free_buckets_[i].next = &free_buckets_[0] + i + 1;
                }
                free_buckets_[free_bucket_count_ - 1].next = &end_bucket_;
                count_ = 0;
            }
            bucket_pointer pop_free()
            {
                bucket_pointer ret = &end_bucket_;
                if (free_head_ == &end_bucket_)
                {
                    return ret;
                }
                ret = free_head_;
                free_head_ = free_head_->next;
                count_++;
                return ret;
            }
            void push_free(bucket_pointer p)
            {
                //p->val = value_type();
                p->next = free_head_->next;
                free_head_ = p;
                count_--;
            }
            iterator next_b(u32 bid)
            {
                while (bid < _Size && buckets_[bid] == NULL)
                {
                    bid++;
                }
                if (bid >= _Size)
                {
                    return mi(&end_bucket_);
                }
                return mi(buckets_[bid]);
            }

            bucket* erase_bucket(bucket* b)
            {
                if (b == &end_bucket_)
                {
                    return &end_bucket_;
                }

                bucket* last = buckets_[b->bid];
                if (b == last)
                {
                    buckets_[b->bid] = b->next;
                    last = buckets_[b->bid];
                    push_free(b);
                    return last;
                }
                bucket* cur = last->next;
                while (cur != b)
                {
                    last = cur;
                    cur = cur->next;
                }
                last->next = b->next;
                push_free(b);
                return last->next;
            }

            std::pair<iterator, bool> insert_v(const value_type& val, bool assign)
            {
                iterator finder = find(val.first);
                if (finder != end())
                {
                    if (assign)
                    {
                        ((bucket*)finder)->val = val;
                    }
                    return { finder, false };
                }

                size_t ukey = hasher(val.first);
                size_t bid = ukey % free_bucket_count_;

                bucket_pointer newp = pop_free();
                if (newp == &end_bucket_)
                {
                    return { end(), false };
                }

                //newp->val = val;
                newp->val.first = val.first;
                newp->fence = fence_val;
                newp->ukey = ukey;
                newp->bid = (u32)bid;
                if (buckets_[bid] == NULL || buckets_[bid] == &end_bucket_)
                {
                    buckets_[bid] = newp;
                    newp->next = &end_bucket_;
                }
                else
                {
                    newp->next = buckets_[bid];
                    buckets_[bid] = newp;
                }
                return { mi(buckets_[bid]), true };
            }
        public:
            iterator begin() noexcept { return next_b(0); }
            const_iterator begin() const noexcept { return next_b(0); }
            const_iterator cbegin() const noexcept { return next_b(0); }

            iterator end() noexcept { return mi(&end_bucket_); }
            const_iterator end() const noexcept { return mi(&end_bucket_); }
            const_iterator cend() const noexcept { return mi(&end_bucket_); }

        public:
            zhash_map()
            {
                reset();
            }
            zhash_map(std::initializer_list<value_type> init)
            {
                reset();
                for (const auto&v : init)
                {
                    insert(v);
                }
            }            
            ~zhash_map()
            {
                
            }
            void clear()
            {
                reset();
            }
            const size_type size() const noexcept { return count_; }
            const bool empty() const noexcept { return !size(); }
            const bool full() const noexcept { return size() == max_size(); }
            size_type bucket_size(size_type bid)
            {
                size_type count = 0;
                if (bid >= _Size)
                {
                    return count;
                }
                bucket* b = buckets_[bid];
                while (b && b != &end_bucket_)
                {
                    count++;
                    b = b->next;
                }
                return count;
            }
            float load_factor() const
            {
                return size() / 1.0f / bucket_count();
            }
            std::pair<iterator, bool> insert(const value_type& val)
            {
                return insert_v(val, false);
            }
            mapped_type& operator[](const key_type& key)
            {
                std::pair<iterator, bool> ret = insert_v(std::make_pair(key, mapped_type()), true);
                if (ret.first != end())
                {
                    return ret.first->second;
                }
                throw std::overflow_error("mapped_type& operator[](const key_type& key)");
            }

            iterator find(const key_type& key)
            {
                size_t ukey = hasher(key);
                size_t bid = ukey % free_bucket_count_;
                bucket_pointer head = buckets_[bid];
                while (head != NULL && head != &end_bucket_)
                {
                    if (head->ukey != ukey || !key_equal(head->val.first, key))
                    {
                        head = head->next;
                        continue;
                    }
                    return mi(head);
                }
                return end();
            }

            iterator erase(iterator iter)
            {
                u32 bid = iter.bucket_->bid;
                bucket* next = erase_bucket(iter);
                if (next == &end_bucket_ && bid <_Size)
                {
                    return next_b(bid);
                }
                return mi(next);
            }

            iterator erase(const key_type& key)
            {
                size_t ukey = hasher(key);
                size_t bid = ukey % free_bucket_count_;
                bucket_pointer last = buckets_[bid];
                do
                {
                    if (last == NULL || last == &end_bucket_)
                    {
                        break;
                    }
                    if (last->bid != bid || last->ukey != ukey || last->val.first != key)
                    {
                        last = last->next;
                        continue;
                    }
                    last = erase_bucket(last);
                } while (true);
                return next_b((u32)bid);
            }


        };

    }
}


#endif