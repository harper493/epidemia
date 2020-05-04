#ifndef __SIZED_ARRAY
#define __SIZED_ARRAY

#include "common.h"

/************************************************************************
 * sized_array class - this defines a class which is a blend of
 * std::vector and std::array. It has a fixed sized storage, without
 * any dynamic allocation, but knows how much is in use and
 * has a useful iterator and size() function.
 *
 * Because it has no additional storage, it is cache and prefetch friendly
 * (which is why it is here). Obviously it has the limit that
 * it cannot accomodate more entries than its fixed maxmimum
 * size.
 *
 * The implementation here provides only the fucntions which are neede
 * by the rest of the code - e.g. no random access iterator.
 ***********************************************************************/

template<class C, size_t MAX_SIZE>
class sized_array
{
public:
    typedef C value_type;
    typedef C& reference_type;
    typedef size_t difference_type;
    typedef array<C, MAX_SIZE> array_t;
    typedef sized_array<C,MAX_SIZE> my_type;
    template<class CC, class BASE_ITER>
    class _iterator : public std::forward_iterator_tag
    {
    public:
        typedef C value_type;
        typedef C &reference_type;
        typedef std::forward_iterator_tag iterator_category;
        typedef _iterator<CC, BASE_ITER> iter_t;
    private:
        CC *my_collection;
        BASE_ITER my_iter;
    public:
        _iterator(CC &coll)
            : my_collection(coll.empty() ? NULL : &coll),
              my_iter(my_collection->my_data.begin()) { };
        _iterator()
            : my_collection(NULL) { };
        _iterator(const iter_t &other)
            : my_collection(other.my_collection),  my_iter(other.my_iter) { };
        _iterator operator=(const iter_t &other)
        {
            my_collection = other.my_collection;
            my_iter = other.my_iter;
        }
        bool operator==(const iter_t &other)
        {
            if (other.my_collection==NULL) {
                return my_collection==NULL;
            } else {
                return my_collection==other.my_collection && my_iter==other.my_iter;
            }
        }
        bool operator!=(const iter_t &other)
        {
            return !(*this==other);
        }
        value_type &operator*() const { return *my_iter; };
        value_type &operator->() const { return *my_iter; };
        _iterator &operator++()
        {
            if (my_collection) {
                my_iter++;
                if ((my_iter - my_collection->my_data.begin())==my_collection->size()) {
                    my_collection = NULL;
                }
            }
            return *this;
        }
        _iterator operator++(int)
        {
            iter_t result=*this;
            ++*this;
            return result;
        };
    };
    typedef _iterator<const sized_array, typename array_t::const_iterator> const_iterator;
    typedef _iterator<sized_array, typename array_t::iterator> iterator;
private:
    array_t my_data;
    size_t my_size = 0;
public:
    sized_array() { };
    template<class COLL, typename enable_if<boost::is_same<typename COLL::value_type, C>,int>::type=0>
    sized_array(const COLL &other)
    {
        my_size = 0;
        for (const C &c : other) {
            push_back(c);
        }
    }
    template<class COLL, typename enable_if<boost::is_same<typename COLL::value_type, C>,int>::type=0>
    sized_array(const COLL  &&other)
    {
        my_size = 0;
        for (const C &&c : other) {
            emplace_back(std::move(c));
        }
    }
    ~sized_array()
    {
        for (size_t i=0; i<my_size; ++i) {
            delete my_data[i];
        }
    }
    size_t size() const
    {
        return my_size;
    }
    bool empty() const
    {
        return my_size==0;
    }
    void push_back(const value_type &v)
    {
        my_data[my_size] = v;
        ++my_size;
    }
    C &back()
    {
        return my_data[my_size-1];
    }
    const C &back() const
    {
        return my_data[my_size-1];
    }
    C &operator[](size_t idx)
    {
        return my_data[idx];
    }
    const C &operator[](size_t idx) const
    {
        return my_data[idx];
    }
    iterator begin()
    {
        return iterator(*this);
    }
    iterator end()
    {
        return iterator();
    }
    const_iterator begin() const
    {
        return const_iterator(*this);
    }
    const_iterator end() const
    {
        return const_iterator();
    }
#if 0
    friend class my_type::_iterator<const sized_array, typename array_t::const_iterator>;
    friend class my_type::_iterator<sized_array, typename array_t::iterator>;
#endif
};

#endif
