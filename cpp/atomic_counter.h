#ifndef __ATOMIC_COUNTER
#define __ATOMIC_COUNTER

#include "common.h"
#include <iostream>

template <class C>
class atomic_counter_base
{
public:
    typedef C base_type; 
private:
    volatile C value;
public:
    atomic_counter_base(C v=0) : value(v) { };
    operator C() const { return value; };
    C get() const { return value; };
    C operator+=(C n) { return __sync_add_and_fetch(&value, n); };
    C operator-=(C n)  { return __sync_sub_and_fetch(&value, n); };
    template<class CC=C, typename enable_if<typename boost::is_unsigned<C>::type,int>::type=0>
    void subtract_if_positive(C n) 
    { 
        bool retry = false;
        do {
            retry = false;
            C v = value;
            if (v >= n) {
                retry = !__sync_bool_compare_and_swap(&value, v, v-n);
            }
        } while (retry);
    }
    C operator++() { return (*this) += 1; };
    C operator--() { return (*this) -= 1; };
    C operator++(int) { return __sync_fetch_and_add(&value, 1); };
    bool operator<(const atomic_counter_base<C> &other) const { return value<other.value; };
    C operator+(const atomic_counter_base<C> &other) const { return value + other.value; };
    bool compare_and_set(C oldv, C newv) 
        { return __sync_bool_compare_and_swap(&value, oldv, newv); };
    void set(C v) { value = v; };
    atomic_counter_base<C> &operator=(C v) { value = v; return *this; };
};

template<class C>
inline std::istream &operator>>(std::istream &istr, atomic_counter_base<C> &ac)
{
    string s;
    istr >> s;
    ac = lexical_cast<C>(s);
    return istr;
}

typedef atomic_counter_base<U32> atomic_counter;
typedef atomic_counter_base<U64> atomic_counter_64;

#endif
