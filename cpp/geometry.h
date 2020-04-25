#ifndef __GEOMETRY
#define __GEOMETRY

#include "common.h"

class offset;

class point : public boost::equality_comparable<point>,
              public boost::less_than_comparable<point>,
              public streamable_traits
{
private:
    float x = 0;
    float y = 0;
public:
    point() { };
    point(float x_, float y_) : x(x_), y(y_) { };
    point(const point &other) : x(other.x), y(other.y) { };
    point &operator=(const point &other) { x=other.x; y=other.y; return *this; };
    float get_x() const { return x; };
    float get_y() const { return y; };
    bool operator==(const point &other) const { return x==other.x && y==other.y; };
    bool operator<(const point &other) const { return x<other.x || (x==other.x && y<other.y); };
    offset operator-(const point &other) const;
    point operator+(const offset &off) const;
    point operator-(const offset &off) const;
    float distance(const point &other) const;
    float bearing(const point &other) const;
    void parse(const string &str);
    string str() const;
friend class offset;
};

class offset : public boost::equality_comparable<offset>,
               public boost::less_than_comparable<offset>,
               public boost::additive<offset>,
               public boost::multiplicative2<offset, float>,
               public streamable_traits
{
private:
    float x = 0;
    float y = 0;
public:
    offset() { };
    offset(float x_, float y_) : x(x_), y(y_) { };
    offset(const offset &other) : x(other.x), y(other.y) { };
    offset &operator=(const offset &other) { x=other.x; y=other.y; return *this; };
    float get_x() const { return x; };
    float get_y() const { return y; };
    bool operator==(const offset &other) { return x==other.x && y==other.y; };
    bool operator<(const offset &other) { return x<other.x || (x==other.x && y<other.y); };
    offset &operator+=(const offset &other) { x+=other.x; y+=other.y; return *this; };
    offset &operator-=(const offset &other) { x=other.x; y=other.y; return *this; };
    offset operator-() const { return offset(-x, -y); };
    offset &operator*=(float mul) { x*=mul; y*=mul; return *this; };
    offset &operator/=(float div) { x/=div; y/=div; return *this; };
    point operator+(const point &pt) const { return pt + *this; };
    point operator-(const point &pt) const { return pt - *this; };
    offset rotate(float degrees) const;
    float length() const;
    float angle() const;
    void parse(const string &str);
    string str() const;
friend class point;
};

inline offset point::operator-(const point &other) const
{
    return offset(x - other.x, y - other.y);
}

inline point point::operator+(const offset &off) const
{
    return point(x + off.x, y + off.y);
}

inline point point::operator-(const offset &off) const
{
    return point(x - off.x, y - off.y);
}

inline float point::distance(const point &other) const
{
    return (other - *this).length();
}

inline float point::bearing(const point &other) const
{
    return (other - *this).angle();
}

#endif



