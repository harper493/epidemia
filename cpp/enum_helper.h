
/************************************************************************
 * Copyright (C) 2017 Saisei Networks Inc. All rights reserved.
 ***********************************************************************/

#ifndef __ENUM_HELPER
#define __ENUM_HELPER

/************************************************************************
 * This family of classes provides a simple means to turn enum values into
 * strings and vice versa.
 *
 * First, declare your name, e.g.
 *
 * class your_class
 * {
 *      enum your_enum_e {
 *          YE_FIRST = 0,
 *          YE_SECOND,
 *      }
 * };
 *
 * In the same header file, include the line:
 *
 * DECLARE_SHOW_ENUM(your_enum_e)
 *
 * (or if the enum is a set of flag values intended to be or'ed together,
 * use DECLARE_SHOW_FLAGS)
 *
 * Then in the cpp file, list the values, e.g.
 *
 * SHOW_ENUM(your_enum_e)
 *     SHOW_ENUM_VAL(your_class::YE_,FIRST)
 *     SHOW_ENUM_VAL(your_class::YE_,SECOND)
 * SHOW_ENUM_END(your_enum_e)
 *
 * The macro SHOW_ENUM_VAL_FORBIDDEN creates a value which will be displayed
 * on output but not valid for input nor shown by enumerate.
 *
 * SHOW_VALUE_HIDDEN is the same except the value is acceptable
 * on input, but is not shown by enumerate.
 *
 * The macro SHOW_ENUM_RANGE(low,high) indicates that an explicit numeric value
 * in the indicated range is acceptable (actually 'low' is ignored for now).
 *
 * Now you can parse and display values like:
 *
 * your_enum_e ye;
 * ye = enum_helper<your_enum_e>().parse("first");
 * string display = enum_helper<your_enum_e>().str(ye);
 *
 * Normally you would bury these incantanations in static member
 * functions - take a look at the interface class for some examples.
 *
 * The parser is fairly flexible. It will accept upper,
 * lower or mixed case. For flags, it will allow values to be combined
 * using '+', "," or '|', e.g. "one_flag+another_flag".
 *
 * The reporter class figures this out automatically, and so does
 * the python infrastructure. YOu don't have to tell them anything
 * explicit to get them to make the appropriate conversions.
 *
 * (Yes, it would be nice to combine this somehow with the actual enum
 * declaration, but without using M4 I really don't see how).
 * 
 ***********************************************************************/

#include "common.h"
#include "string_exception.h"
#include <boost/algorithm/string.hpp>

#define FORBIDDEN_MARKER '!'
#define HIDDEN_MARKER '?'

struct enum_helper_traits { };

class enum_helper_base : public enum_helper_traits
{
protected:
    typedef std::pair<U32,const char*> one_value;
    typedef vector<one_value> values_t;
    template<class ENUM>
    void _assign_enum(ENUM &target, U32 value) const;
    string _str(U32 v, const values_t & values, bool flags) const;
    string _enumerate(const values_t &values) const;
    U32 _parse(const string &val, const values_t &values) const;
    U32 _parse_flags(const string &val, const values_t &values) const;
    bool _parse_one(U32 &result, const string &val, const values_t &values) const;
};

template<class ENUM>
class enum_helper : public enum_helper_base
{
public:
    bool useful() const { return false; };
    template<class C>
    string str(const C&v) const { return ""; };
    string enumerate() const { return ""; };
    ENUM parse(const string &val) const { U32 v = 0; return *(ENUM*)&v; }; \
    bool parse(ENUM &result, const string &val) const { return false; }; \
    ENUM parse_flags(const string &val) const { U32 v = 0; return *(ENUM*)&v; }; \
};

#define _DECLARE_SHOW_ENUM(ENUM, FLAGS)                                 \
    template<>                                                          \
    class enum_helper<ENUM> : public enum_helper_base {                 \
    public :                                                            \
    enum { flags=FLAGS?1:0 };                                           \
    bool useful() const { return true; };                               \
    static enum_helper_base::values_t values;                           \
    string str(U32 v) const { return _str(v, values, flags); };         \
    string str(ENUM v) const { return _str((U32)v, values, flags); };   \
    string enumerate() const { return _enumerate(values); };            \
    ENUM parse(const string &val) const                                 \
        { U32 v = flags ? _parse_flags(val, values) : _parse(val, values); \
            ENUM e; this->_assign_enum(e, v); return e; };               \
    bool parse(ENUM &result, const string &val) const                   \
        { U32 v; debug_assert(!flags);                                  \
            bool b = _parse_one(v, val, values);                        \
            if (b) this->_assign_enum(result, v);                        \
            return b;                                                   \
        };                                                              \
    };                                                                  


#define DECLARE_SHOW_ENUM(ENUM) _DECLARE_SHOW_ENUM(ENUM, false)
#define DECLARE_SHOW_FLAGS(ENUM) _DECLARE_SHOW_ENUM(ENUM, true)

#define DECLARE_SHOW_ENUM_CLASS(ENUM)                                   \
    _DECLARE_SHOW_ENUM(ENUM, false)                                     \
    inline std::ostream &operator<<(std::ostream &ostr, ENUM e)         \
    { ostr << enum_helper<ENUM>().str(e); return ostr; };               \
    inline std::istream &operator>>(std::istream &istr, ENUM &e)        \
    { string v; istr>>v; e = enum_helper<ENUM>().parse(v); return istr; };


#define SHOW_ENUM(ENUM) \
    enum_helper_base::values_t enum_helper<ENUM>::values {

#define SHOW_ENUM_VAL(PFX,NAME) one_value {(U32)PFX##NAME,#NAME},
#define SHOW_ENUM_VAL_FORBIDDEN(PFX,NAME) one_value {(U32)PFX##NAME,"!"#NAME},
#define SHOW_ENUM_VAL_HIDDEN(PFX,NAME) one_value {(U32)PFX##NAME,"?"#NAME},
#define SHOW_ENUM_RANGE(LOW,HIGH) one_value {0,"+"#HIGH},

#define SHOW_ENUM_END(ENUM) \
    }; \

/************************************************************************
 * Inline functions
 ***********************************************************************/

/************************************************************************
 * assign - use some ugly casts to allow a U32 value to be correctly
 * placed into an enum, even a class enum
 ***********************************************************************/

template<class ENUM>
inline void enum_helper_base::_assign_enum(ENUM &target, U32 value) const
{
    target = static_cast<ENUM>(value);
}

#endif
