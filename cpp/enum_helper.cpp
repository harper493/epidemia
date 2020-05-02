/************************************************************************
 * Copyright (C) 2017 Saisei Networks Inc. All rights reserved.
 ***********************************************************************/

#include "enum_helper.h"
#include "utility.h"
#include "string_exception.h"
#include "istring.h"

string enum_helper_base::_str(U32 v, const values_t &values, bool flags) const
{
    string result;
    if (flags && v!=0) {
        for (const one_value &ov : values) {
            if (v & ov.first) {
                string s(ov.second);
                if ((s[0]==FORBIDDEN_MARKER) || (s[0]==HIDDEN_MARKER)) {
                    s = s.substr(1);
                }
                join_to(result, s, ",");
                v &= ~ov.first;
            }
        }
        if (v) {
            join_to(result, formatted("0x%x", v), "+");
        }
    } else {
        for (const one_value &ov : values) {
            if (v==ov.first) {
                if (ov.second[0]!='+') {
                    result = ov.second;
                    if ((result[0]==FORBIDDEN_MARKER) || (result[0]==HIDDEN_MARKER)){
                        result = result.substr(1);
                    }
                    break;
                }
            }
        }
        if (result.empty()) {
            result = lexical_cast<string>(v);
        }
    }
    return result;
}

/************************************************************************
 * Enumerate all possible values, returning a string separated by '|'
 ***********************************************************************/

string enum_helper_base::_enumerate(const values_t &values) const
{
    string result;
    for (const one_value &ov : values) {
        if (ov.second[0]!=FORBIDDEN_MARKER && ov.second[0]!=HIDDEN_MARKER) {
            join_to(result, ov.second, "|");
        }
    }
    return result;
}

/************************************************************************
 * Parse a value for an enum. If the string is empty, return the first
 * item from the list (which should of course be the default).
 *
 * For a flag-style variable, or together values separated by one
 * of ",+|&"
 ***********************************************************************/

bool enum_helper_base::_parse_one(U32 &result, const string &val, const values_t &values) const 
{
    bool found = false;
    result = 0;
    U32 limit = 0;
    if (val.empty()) {
        result = values[0].first;
        found = true;
    } else {
        for (const one_value &ov : values) {
            if (ov.second[0]=='+') {
                limit = lexical_cast<U32>(string(ov.second).substr(1));
            } else {
                string target;
                if (isalnum(ov.second[0])) {
                    target = ov.second;
                } else {
                    target = ov.second+1;
                    if (ov.second[0]==FORBIDDEN_MARKER) {
                        continue;
                    }
                }
                if (boost::istarts_with(target, val)) {
                    result = ov.first;
                    if (istring(val)==target) {
                        found = true;
                        break;
                    } else {
                        if (found) {
                            throw string_exception("enumeration value '%s' is ambiguous", val);
                        } else {
                            found = true;
                        }
                    }
                }
            }
        }
        if (!found) {
            if (limit>0) {
                try {
                    result = lexical_cast<U32>(val);
                    found = true;
                } catch (...) {
                }
            }
            if (found && result > limit) {
                throw string_exception("numeric value %d too large for enumeration", result);
            }
        }
    }
    return found;    
}

U32 enum_helper_base::_parse_flags(const string &val, const values_t &values) const
{
    vector<string> vals = trim(split(val, ",+|&"));
    U32 result = 0;
    if (val != "0") {
        for (const string &v : vals) {
            U32 r;
            if (_parse_one(r, v, values)) {
                result |= r;
            } else {
                throw string_exception("unknown value '%s' for flag", val);
            }
        }
    }
    return result;
}
    
U32 enum_helper_base::_parse(const string &val, const values_t &values) const
{
    U32 result;
    if (_parse_one(result, val, values) ) {
        return result; 
    } else {
        throw string_exception("unknown value '%s' for enumeration", val);
    }
}
    
