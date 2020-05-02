#ifndef __ISTRING
#define __ISTRING

/************************************************************************
 * Case-independent string class. Works exactly like a normal string 
 * except that all comparisons are case-independent.
 *
 * The implementation is a bit messy because a lot of operators have to
 * be created explicitly to avoid ambiguity errors from the compiler.
 *
 * Originally this used the boost case-independent compare functions.
 * But these use locales which results in a huge and expensive mess of deeply
 * nested virtual function calls, so it has been rewritten to do a very
 * crude case conversion, which works perfectly for ASCII characters.
 * We don't care about the rest since case-equivalence is in general
 * a nightmare and not applicable to most character sets anyway.
 * (Sorry Cyrillic). We don't use any of the builtin toupper etc
 * functions since who knows which ones use locales or may do one
 * day.
 ***********************************************************************/

#include "common.h"
#include <boost/algorithm/string/case_conv.hpp>

class istring : public string
{
public:
    istring() : string() { };
    istring(const istring &other) : string(other) { };
    istring(const string &other) : string(other) { };
    istring(const char *str) : string(str) { };
    string get() const { return string(*this); };
    string get_canonical() const { return boost::to_upper_copy(string(*this)); };
    bool empty() const { return string::empty(); }; // needed for MPL
    //
    // comparison operators for otrher type = istring
    //
    bool operator==(const istring &other) const
    {
        return stricmp(other.c_str())==0;
    }
    bool operator!=(const istring &other) const
    {
        return stricmp(other.c_str())!=0;
    }
    bool operator<(const istring &other) const
    {
        return stricmp(other.c_str())<0;
    }
    bool operator>(const istring &other) const
    {
        return stricmp(other.c_str())>0;
    }
    bool operator<=(const istring &other) const
    {
        return stricmp(other.c_str())<=0;
    }
    bool operator>=(const istring &other) const
    {
        return stricmp(other.c_str())>=0;
    }
    //
    // comparison operators for other type = string
    //
    bool operator==(const string &other) const
    {
        return stricmp(other.c_str())==0;
    }
    bool operator!=(const string &other) const
    {
        return stricmp(other.c_str())!=0;
    }
    bool operator<(const string &other) const
    {
        return stricmp(other.c_str())<0;
    }
    bool operator>(const string &other) const
    {
        return stricmp(other.c_str())>0;
    }
    bool operator<=(const string &other) const
    {
        return stricmp(other.c_str())<=0;
    }
    bool operator>=(const string &other) const
    {
        return stricmp(other.c_str())>=0;
    }
    //
    // comparison operators for other type = const char*
    //
    bool operator==(const char *other) const
    {
        return stricmp(other)==0;
    }
    bool operator!=(const char *other) const
    {
        return stricmp(other)!=0;
    }
    bool operator<(const char *other) const
    {
        return stricmp(other)<0;
    }
    bool operator>(const char *other) const
    {
        return stricmp(other)>0;
    }
    bool operator<=(const char *other) const
    {
        return stricmp(other)<=0;
    }
    bool operator>=(const char *other) const
    {
        return stricmp(other)>=0;
    }
private:
    //
    // ensure no locales are involved!
    //
    static char toupper(char ch)
    {
        return ch >= 'a' && ch <='z' ? ch - ('a' - 'A') : ch;
    }
    int stricmp(const char *other) const
    {
        for (const char &ch1 : *this) {
            if (*other==0) {
                return 1;
            }
            char uch1 = toupper(ch1);
            char uch2 = toupper(*other);
            if (uch1<uch2) {
                return -1;
            } else if (uch1>uch2) {
                return 1;
            }
            ++other;
        }
        if (*other) {
            return -1;
        }
        return 0;
    }
};

inline bool operator==(const string &s1, const istring &s2)
{
    return s2==s1;
}

inline bool operator!=(const string &s1, const istring &s2)
{
    return s2!=s1;
}

inline bool operator<(const string &s1, const istring &s2)
{
    return s2>=s1;
}

inline bool operator<=(const string &s1, const istring &s2)
{
    return s2>s1;
}

inline bool operator>(const string &s1, const istring &s2)
{
    return s2<=s1;
}

inline bool operator>=(const string &s1, const istring &s2)
{
    return s2<s1;
}

inline std::ostream& operator<<(std::ostream &str, const istring &i)
{
    str << i.get();
    return str;
}

inline std::istream& operator>>(std::istream &str, istring &i)
{
    string strsn;
    str >> strsn;
    i = istring(strsn);
    return str;
};

/************************************************************************
 * lexical_cast to/from string - just copy
 ***********************************************************************/

namespace boost
{
template<>
inline istring lexical_cast(const string &src)
{
    return src;
}

template<>
inline string lexical_cast(const istring &src)
{
    return src;
}
}

#endif
