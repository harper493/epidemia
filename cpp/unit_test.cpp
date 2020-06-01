#include "unit_test.h"
#include "utility.h"
#include "formatted.h"
#include "command.h"

/************************************************************************
 * Static data
 ***********************************************************************/

unit_test::list *unit_test::the_tests;
unit_test *unit_test::current_test = NULL;
std::ostream *unit_test::log_stream = NULL;
U32 unit_test::errors;

bool unit_test::run_unit_tests = DEBUG_TRUE;
bool unit_test::run_long_unit_tests = false;

/************************************************************************
 * Constructor. Create the object, and enroll it.
 ***********************************************************************/

unit_test::unit_test(const string &n, function<void()> fn, bool l=false)
    : name(n), test_fn(fn), long_test(l)
{
    enroll(this);
};

/************************************************************************
 * Destructor - unlink self
 ***********************************************************************/

unit_test::~unit_test()
{
    if (the_tests) {
        the_tests->erase(the_tests->iterator_to(*this));
    }
}

/************************************************************************
 * Enroll - add this test to the repertoire
 ***********************************************************************/

void unit_test::enroll(unit_test *ut)
{
    if (the_tests==NULL) {
        the_tests = new list();
    }
    the_tests->push_back(*ut);
}

/************************************************************************
 * apply - apply all appropriate tests based on the parameters. Return
 * true all tests were successful.
 ***********************************************************************/

bool unit_test::apply()
{
    auto_ptr<std::ostream> my_log_stream(new std::ofstream("/var/log/stm_unit_test.log"));
    errors = 0;
    log_stream = my_log_stream.get();
    write_log("starting unit tests", false);
    if (run_unit_tests) {
        _apply([](const unit_test &ut){ return !ut.long_test; });
    }
    if (run_long_unit_tests) {
        _apply([](const unit_test &ut){ return ut.long_test; });
    }
    if (errors>0) {
        write_log(formatted("unit tests completed with %d errors", errors), true);
    } else {
        write_log("unit tests completed with no errors", true);
    }
    return errors==0;
}

/************************************************************************
 * Generic function to run selected tests.
 ***********************************************************************/

void unit_test::_apply(function<bool(const unit_test&)> pred)
{
    if (the_tests) {
        for (unit_test &ut : *the_tests) {
            if (pred(ut)) {
                current_test = &ut;
                try {
                    ut_log("starting test");
                    (ut.test_fn)();
                } catch (ut_exception &utexc) {
                    ut_log(formatted("at line %d %s", ut.name, utexc.line_no, utexc.what_str), true);
                } catch (std::exception &exc) {
                    ut_log(formatted("uncaught exception %s",
                                     exc.what()), true);
                }
            }
        }
    }
    current_test = NULL;
}

/************************************************************************
 * match_name - see if the given n is either equal to the given name,
 * or is one of the corresponding class tests
 ***********************************************************************/

bool unit_test::match_name(const string &n, const string &target)
{
    return n==target
        || n==(string("class_") + target)
        || n==(string("class_") + target + "_long");
}

/************************************************************************
 * ut_verbose - return true iff verbose logging is turned
 * (needed to avoid header file circularity)
 ***********************************************************************/

bool unit_test::ut_verbose()
{
    return the_args->get_verbosity() > 0;
}

/************************************************************************
 * ut_log - add a message to the unit test log
 ***********************************************************************/

void unit_test::ut_log(const string &msg, bool to_console)
{
    string line = formatted("test %s: %s", current_test->name, msg);
    write_log(line, to_console);
}

void unit_test::write_log(const string &msg, bool to_console)
{
    (*log_stream) << ptime(bpt::microsec_clock::local_time()) << " " << msg << std::endl;
    if (to_console) {
        std::cout << msg << std::endl;
    }
}
