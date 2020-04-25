#include "geometry.h"
#include "formatted.h"
#include "string_exception.h"
#include <tgmath.h>

const static float pi = acos(0) * 2;
const static float deg2rad = pi / 180;
const static float rad2deg = 180 / pi;

/************************************************************************
 * str() - convert to string
 ***********************************************************************/

string point::str() const
{
    return formatted("(%.3f,%.3f)", x, y);
}

/************************************************************************
 * parse - parse a string of the format latitude,longitude, optionally
 * surrounded by parentheses.
 ***********************************************************************/

void point::parse(const string &value)
{
    if (value.empty()) {
        x = 0;
        y = 0;
    } else {
        regex rx("(?:\\()?([0-9.+-]+),([0-9.+-]+)(?:\\))?");
        smatch s;
        bool good = false;
        if (regex_match(value, s, rx)) {
            try {
                x = lexical_cast<float>(s[1]);
                y = lexical_cast<float>(s[2]);
                good = true;
            } catch(...) { };
        }
        if (!good) {
            throw string_exception("syntax error in point '%s'", value);
        }
    }
}

/************************************************************************
 * same for offset - cheat by using the point operators
 ***********************************************************************/

void offset::parse(const string &str)
{
    point pt;
    pt.parse(str);
    x = pt.x;
    y = pt.y;
}

string offset::str() const
{
    point pt(x, y);
    return pt.str();
}

/************************************************************************
 * vector functions for offset
 ***********************************************************************/

float offset::length() const
{
    return sqrt(x*x + y*y);
}

float offset::angle() const
{
    return atan2(x, y) * rad2deg;
}

offset offset::rotate(float degrees) const
{
    const float rads = degrees * deg2rad;
    return offset(x * cos(rads) - y * sin(rads),
                  x * sin(rads) + y * cos(rads));
}


