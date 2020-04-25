#ifndef __STRING_EXCEPTION
#define __STRING_EXCEPTION

#include "type_definitions.h"
#include "formatted.h"
#include <boost/type_traits.hpp>

/************************************************************************
 * Collection of exception classes for signalling appropriate error back 
 * into Python and Rest code
 ***********************************************************************/

enum http_errors {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_SUCCESS_RESET = 205,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_AUTHORIZED = 401,
    HTTP_PAYMENT_REQUIRED = 402,
    HTTP_ACTION_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_INSUFFICIENT_STORAGE = 507,
};

/************************************************************************
 * Base class, translates to HTTP 400 (Bad Request)
 ***********************************************************************/

class string_exception : public std::exception
{
private:
    U32 status_code = HTTP_BAD_REQUEST;
    string what_str;
public:
    template<typename... ARGS>
    string_exception(const string &f, const ARGS &...args)
    {
        what_str = formatted(f, args...);
    }
    string_exception() { };
    ~string_exception() throw() { };
    string_exception(const string_exception &other)
        : status_code(other.status_code), what_str(other.what_str) { };
    string_exception &operator=(const string_exception &other)
    {
        status_code = other.status_code;
        what_str = other.what_str;
        return *this;
    }
    virtual const char *what() const throw () { return what_str.c_str(); };
    U32 get_http_code() const { return status_code; };
protected:
    template<typename... ARGS>
    string_exception(U32 s, const string &f, const ARGS &...args)
        : status_code(s)
    {
        what_str = formatted(f, args...);
    }
};

/************************************************************************
 * not_allowed, translates to HTTP 405
 ***********************************************************************/

class not_allowed_exception : public string_exception
{
public:
    template<typename... ARGS>
    not_allowed_exception(const string &f, const ARGS &...args)
        : string_exception(HTTP_NOT_ALLOWED, 
                           (f.find(' ')==string::npos ? f + " not permitted for class '%s'" : f),
                            args...) { };
    ~not_allowed_exception() throw() { };
};

/************************************************************************
 * not_authorized, translates to HTTP 401
 ***********************************************************************/

class not_authorized_exception : public string_exception
{
public:
    not_authorized_exception(const string &f="operation not authorized") throw() : string_exception(f) { };
    template<typename... ARGS>
    not_authorized_exception(const string &f, const ARGS &...args)
        : string_exception(HTTP_NOT_AUTHORIZED, f, args...) { };
    ~not_authorized_exception() throw() { };
};

/************************************************************************
 * action_forbidden, translates to HTTP 403
 ***********************************************************************/

class action_forbidden_exception : public string_exception
{
public:
    action_forbidden_exception(const string &f="action forbidden") throw() : string_exception(f) { };
    template<typename... ARGS>
    action_forbidden_exception(const string &f, const ARGS &...args)
        : string_exception(HTTP_ACTION_FORBIDDEN, f, args...) { };
    ~action_forbidden_exception() throw() { };
};

/************************************************************************
 * not_found, translates to HTTP 404
 ***********************************************************************/

class not_found_exception : public string_exception
{
public:
    not_found_exception(const string &f="object not found") throw() : string_exception(f) { };
    template<class T, typename disable_if<boost::is_same<T, const char>,int>::type=0>
    not_found_exception(T*, const string &name)
        : string_exception(HTTP_NOT_FOUND, "%s '%s' does not exist", T::get_type_name(), name) { };
    template<typename... ARGS>
    not_found_exception(const char *f, const ARGS &...args)
        : string_exception(HTTP_NOT_FOUND, f, args...) { };
    not_found_exception(const string &class_name, const string &obj_name)
        : string_exception(HTTP_NOT_FOUND, "%s '%s' does not exist", class_name, obj_name) { };
    ~not_found_exception() throw() { };
};

/************************************************************************
 * no_memory_exception - HTTP 403 code when we can't get memory
 ***********************************************************************/

class no_memory_exception : public string_exception
{
public:
    no_memory_exception(const string &p1, const string &p2="") throw() 
        : string_exception(HTTP_INSUFFICIENT_STORAGE,
                           string("insufficient memory for %s") + (p2.empty() ? "%s" : " '%s'"), p1, p2) { };
    ~no_memory_exception() throw() { };
    template <class C>
    static C *check(C *ptr, const string &p1, const string &p2="")
    {
        if (ptr==NULL) {
            throw no_memory_exception(p1, p2);
        }
        return ptr;
    }
};

/************************************************************************
 * http_exception - specify the status code on creation
 ***********************************************************************/

class http_exception : public string_exception
{
private:
    U32 status_code;
public:
    template<typename... ARGS>
    http_exception(U32 status, const string &f, const ARGS &...args)
        : string_exception(status, f, args...) { };
    ~http_exception() throw() { };
};

/************************************************************************
 * no_value_exception - returned by value parsers to mean "don't change
 * whatevervalue you already have
 ***********************************************************************/

class no_value_exception : public std::exception
{
public:
    no_value_exception() throw() { };
};


#endif
