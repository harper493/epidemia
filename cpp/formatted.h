
/************************************************************************
 * Copyright (C) 2017 Saisei Networks Inc. All rights reserved.
 ***********************************************************************/

#ifndef __FORMATTED
#define __FORMATTED

#include "common.h"

/************************************************************************
 * Function to encapsulate (format("...") % ...).str() design pattern
 ***********************************************************************/

inline void _formatted(format &fmt) // recursion terminator
{
}

template<typename ARG, typename... ARGS>
inline void _formatted(format &fmt, const ARG &arg1, const ARGS &...args)
{
    fmt % arg1;
    _formatted(fmt, args...);
}

template<typename... ARGS>
inline string formatted(const string &fmt, const ARGS &...args)
{
    format f(fmt);
    _formatted(f, args...);
    return f.str();
}

#endif
