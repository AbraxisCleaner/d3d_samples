#pragma once
#include "pch.h"
#include <new> // For placement new

// A dynamically resizing array that maintains extra capacity to 
//   avoid unnecessary allocations.
template <typename T, uint _increment = 8>
class TArray
{
    T *m_ptr;
    uint m_count;
    uint m_cap;

public:
    TArray() 
    {
        m_cap = _increment;
        m_count = 0;
        m_ptr = (T *)malloc(m_cap * sizeof(T));
        new(m_ptr) T[m_cap];
    }
    ~TArray()
    {
        if (m_ptr) 
        {
            for (auto i = 0; i != m_count; ++i)
                m_ptr[i].~T();
            free(m_ptr);
            ZeroThat(this);
        }
    }

    inline void ShrinkCapacity()
    {
        auto old_capacity = m_cap;
        while (m_cap > m_count)
            m_cap -= _increment;

        if (m_cap == old_capacity)
            return;

        for (auto i = m_cap; i != old_capacity; ++i)
            m_ptr[i].~T();

        m_ptr = (T *)realloc(m_ptr, m_cap * sizeof(T));
    }

    inline T *Append(uint count = 1, T *src = nullptr)
    {
        auto accum = m_count + count;
        if (accum > m_cap)
        {
            auto old_cap = m_cap;
            while (accum > m_cap)
                m_cap += _increment;

            m_ptr = (T *)realloc(m_ptr, m_cap * sizeof(T));
            new(&m_ptr[old_cap]) T[m_cap - old_cap];
        }

        auto dst = end();
        if (src)
            for (auto i = 0; i != count; ++i)
                dst[i] = src[i];
        
        m_count = accum;
        return dst;
    }

    inline T *Push(uint count = 1, T *src = nullptr)
    {
        auto accum = m_count + count;
        if (accum > m_cap)
        {
            auto old_cap = m_cap;
            while (accum > m_cap)
                m_cap += _increment;
            
            m_ptr = (T *)realloc(m_ptr, m_cap * sizeof(T));
            new(&m_ptr[old_cap]) T[m_cap - old_cap];
        }

        auto dst = begin();
        memcpy(&m_ptr[count], m_ptr, m_count * sizeof(T));
        new(dst) T[count];

        if (src)
            for (auto i = 0; i != count; ++i)
                dst[i] = src[i];
        
        m_count = accum;
        return dst;
    }

    __forceinline T *begin() { return m_ptr; }
    __forceinline T *end() { return &m_ptr[m_count]; }

    uint GetCapacity() { return m_cap; }
    uint GetData() { return m_ptr; }
    constexpr uint GetElementSize() { return sizeof(T); }
    constexpr uint GetIncrementSize() { return _increment; }

public:
    T &operator [](uint it) { return m_ptr[it]; }
    operator T *() { return m_ptr; }
    operator void *() { return m_ptr; }
};