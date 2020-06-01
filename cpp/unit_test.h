#ifndef __UNIT_TEST
#define __UNIT_TEST

/************************************************************************
 * Class and macros to enable unit tests to be built into classes
 ***********************************************************************/

#include "common.h"

class unit_test : public bintr::list_base_hook<>
{
    class ut_exception : public std::exception
    {
    private:
        int line_no;
        string what_str;
    public:
        ut_exception(int ln, const string &w) : line_no(ln), what_str(w) { };
        virtual const char *what() const throw() { return what_str.c_str(); };
    friend class unit_test;
    };
public:
    typedef bintr::list<unit_test> list;
private:
    string name;
    function<void()> test_fn;
    bool long_test;
    static U32 errors;
    static bool run_unit_tests;
    static bool run_long_unit_tests;
    static list *the_tests;
    static unit_test *current_test;
    static std::ostream *log_stream;
public:
    unit_test(const string &n, function<void()> fn, bool l);
    ~unit_test();
    static bool apply();
    template<typename... ARGS>
    static void verify(bool cond, int line_no, const string &f, const ARGS &...args)
    {
        if (!cond) {
            ++errors;
            ut_log(formatted("at line %d "+f, line_no, args...), true);
        }
    }
    template<typename... ARGS>
    static void ut_assert(bool cond, int line_no, const string &f, const ARGS &...args)
    {
        if (!cond) {
            throw ut_exception(line_no, formatted(f, args...));
        }
    }
    static void ut_log(const string &str, bool to_console=false);
    template<class V1, class V2>
    static string _ut_equal_message(const string &v1_name, const V1 &v1, const V2 &v2)
    {
        return formatted("%s: should be %s, was %s", v1_name, v2, v1);
    }
    template<class V1, class V2>
    static string _ut_equal_message(const string &v1_name, const V1 &v1, const V2 &v2, const string &what)
    {
        return formatted("%s: should be %s, was %s", what, v2, v1);
    }
    static bool ut_verbose();
    //
    // ut_format is used by the UT_LOG and UT_VERBOSE macros. It is written this way
    // so that if they are just given a simple string, with no additional arguments,
    // they do not try to format the string.
    //
    static string ut_format(const string &str) { return str; };
    template<typename... ARGS>
    static string ut_format(const string &fmt, const ARGS &...args)
    {
        return formatted(fmt, args...);
    }
private:
    static void enroll(unit_test *ut);
    static void _apply(function<bool(const unit_test&)> pred);
    static void write_log(const string &msg, bool to_console);
    static bool match_name(const string &n, const string &target);
};

/************************************************************************
 * Define unit tests using stand-alone functions. The LONG version will
 * only run the test if specifically asked.
 ***********************************************************************/

#define UNIT_TEST(NAME, FN) unit_test _ut_##NAME(#NAME, FN, false);
#define LONG_UNIT_TEST(NAME, FN) unit_test _ut_##NAME(#NAME, FN, true);

/************************************************************************
 * Define unit tests for a specific class. The class must define a
 * static function called unit_test (or long_unit_test);
 ***********************************************************************/

#define UNIT_TEST_CLASS(CLASS) unit_test _ut_class_##CLASS("class_"#CLASS, &CLASS::unit_test, false);
#define LONG_UNIT_TEST_CLASS(CLASS) unit_test _ut_class_##CLASS("class_"#CLASS"_long", &CLASS::long_unit_test, true);

/************************************************************************
 * Macros for verifying results and logging
 *
 * UT_LOG - log a message
 * UT_VERBOSE - log a message if --verbose is set
 * UT_VERIFY - log an error if the condition is not true, and continue
 * UT_CHECK - as UT_VERIFY but construct the failure message from the condition
 * UT_EQUAL - log an error if the two vales are not equal, with optional explanation
 * UT_ASSERT - abandon the test if condition is not true
 ***********************************************************************/

#define UT_LOG(MSG, ...) unit_test::ut_log(unit_test::ut_format(MSG, ##__VA_ARGS__));
#define UT_VERBOSE(MSG, ...) if (unit_test::ut_verbose()) UT_LOG(MSG, ##__VA_ARGS__)
#define UT_VERIFY(COND, MSG, ...) unit_test::verify((COND), __LINE__, MSG, ##__VA_ARGS__);
#define UT_CHECK(COND) UT_VERIFY((COND), "test condition not satisfied: " #COND)
#define UT_EQUAL(V1, V2, ...) UT_VERIFY((V1)==(V2), unit_test::_ut_equal_message(#V1, V1, V2, ##__VA_ARGS__));
#define UT_ASSERT(COND, MSG, ...) unit_test::ut_assert((COND), __LINE__, MSG, ##__VA_ARGS__);

#endif
