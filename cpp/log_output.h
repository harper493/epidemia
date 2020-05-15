#ifndef __LOG_OUTPUT
#define __LOG_OUTPUT

#include "common.h"

class log_output
{
public:
    struct column
    {
        string format_str;
        format format_;
        string heading;
        size_t width = 0;
        column(const log_output &logger, const string &fmt, const string &hdg);
    };
    typedef vector<column> columns_t;
    typedef pair<string,string> pairss;
    typedef vector<pairss> column_defs;
private:
    string filename;
    std::ostream *my_ostr = NULL;
    bool my_stream;
    bool csv;
    bool do_flush = false;
    columns_t columns;
    columns_t::iterator column_iter;
    bool first = true;
public:
    log_output();
    log_output(const string &fn, bool csv_, bool do_heading, const vector<pairss> &col_info)
    {
        create(fn, csv_, do_heading, col_info);
    }
    void create(const string &fn, bool csv_, bool do_heading, const vector<pairss> &col_info);
    ~log_output()
    {
        close();
    }
    bool good() const { return my_ostr && my_ostr->good(); };
    void close();
    template<typename... ARGS>
    void put_line(const ARGS &...args)
    {
        start_line();
        _put_element(args...);
        write_line();
    }
    void write_line(const string &line="")
    {
        (*my_ostr) << line << "\n";
    }
    void flush()
    {
        my_ostr->flush();
    }
private:
    template<class C>
    void _put(column &col, const C &val)
    {
        if (csv && !first) {
            (*my_ostr) << ",";
        } else {
            first = false;
        }
        col.format_ % val;
        (*my_ostr) << col.format_.str();
    }
    template<typename C, typename... ARGS>
    void _put_element(const C &arg, const ARGS &...args)
    {
        if (column_iter!=columns.end()) {
            _put(*column_iter++, arg);
            _put_element(args...);
        }
    }
    void _put_element() { };     // recursion stopper
    void start_line();
friend class column;
};

#endif
