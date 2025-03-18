#pragma once
#include <cstdint>
#include <utility>
template <typename T>
struct DynamicArray
{
    int size = 0;
    int capacity = 0;
    T* data;

    DynamicArray()
    {
        ;
    }

    DynamicArray(const T* other, const uint32_t& _size)
    {
        data = new T[_size];
        memcpy(data, other, sizeof(T)*_size);
        size = _size;
        capacity = _size;
    }

    DynamicArray(int _size)
    {
        data = new T[_size];
        size = _size;
        capacity = size;
    }

    T* begin() const { return data; };
    T* end() const { return data + size; };

    void clear()
    {
        size = 0;
    }

    //this will overwrite data
    T* create(uint32_t _size)
    {
        if (size == 0) return nullptr;
        delete [] data;
        data = new T[_size];
        capacity = _size;
        size = _size;
        return data;
    }

    T& push_back(T&& val)
    {
        if (size == capacity)
        {
            bool empty = false;
            if (capacity == 0) {
                capacity++;
                empty = true;
            }
            T* newData = new T[capacity * 2];
            memcpy(newData, data, sizeof(T)*size);
            
            if (!empty)
                delete [] data;

            data = newData;
            capacity *= 2;
        }
        data[size++] = std::move(val);
        return data[size-1];
    }

    T& push_back(const T& val)
    {
        if (size == capacity)
        {
            bool empty = false;
            if (capacity == 0) {
                capacity++;
                empty = true;
            }
            T* newData = new T[capacity * 2];
            memcpy(newData, data, sizeof(T)*size);
            
            if (!empty)
                delete [] data;

            data = newData;
            capacity *= 2;
        }
        data[size++] = val;
        return data[size-1];
    }

    T& operator[](uint32_t idx)
    {
        return data[idx];
    }

    void operator=(DynamicArray<T>&& other)
    {
        data = other.data;
        other.data = nullptr;
        size = other.size;
        capacity = other.capacity;
    }

    ~DynamicArray()
    {
        delete [] data;
    }
};

template <typename T, uint32_t N>
struct StaticArray
{
    T data[N];
    int size = N;
    const int capacity = N;

    T& operator[](uint32_t idx)
    {
        return data[idx];
    }

    const T* begin() const { return data; };
    const T* end() const { return data + size; };

    void operator=(const T other[N])
    {
        memcpy(data, other, sizeof(T)*N);
    }
};
