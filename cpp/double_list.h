#ifndef __DOUBLE_LIST
#define __DOUBLE_LIST

#include "common.h"
#include "prefetcher.h"

template<class C, int DEPTH=C::prefetch_depth, void(PREFETCHER)(C*,int)=&C::prefetch>
class double_list
{
public:
    typedef vector<C*> vec_t;
    typedef prefetcher<vec_t, DEPTH, PREFETCHER> prefetcher_t;
private:
    vec_t from_list;
    vec_t to_list;
friend class iterator;
public:
    void insert(C *c)
    {
        to_list.push_back(c);
    }
    void reset()
    {
        from_list = std::move(to_list);
        to_list.reserve(from_list.size() * 2);
    }
    size_t size() const { return from_list.size(); }
    size_t next_size() const { return to_list.size(); }
    bool empty() const { return from_list.empty(); }
    bool next_empty() const { return to_list.empty(); }
    class iterator : public std::forward_iterator_tag
    {
    public:
        typedef C* value_type;
        typedef C*& reference_type;
        typedef std::forward_iterator_tag iterator_category;
    private:
        auto_ptr<prefetcher_t> my_prefetcher;
        typename prefetcher_t::iterator my_iter;
    public:
        iterator() { };
        iterator(double_list &dl)
            : my_prefetcher(new prefetcher_t(dl.from_list)), my_iter(my_prefetcher->begin())
        {
        };
        iterator(iterator &&other)
            : my_prefetcher(std::move(other.my_prefetcher)), my_iter(other.iter)
        { };
        bool operator==(const iterator &other) const
        {
            return my_iter==other.my_iter;
        }
        bool operator!=(const iterator &other) const
        {
            return !(*this==other);
        }
        value_type operator*() const
        {
            return *my_iter;
        }
        value_type operator->() const
        {
            return *my_iter;
        }
        iterator &operator++()
        {
            ++my_iter;
            return *this;
        }
    };
public:
    iterator begin() { return iterator(*this); };
    iterator end() { return iterator(); };
};

#endif
