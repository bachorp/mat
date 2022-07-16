#pragma once

#include <vector>
#include <string>
#include <optional>
#include <utility>

#include <ostream>

using std::optional;
using std::pair;
using std::string;
using std::vector;

#define _ [[maybe_unused]] const auto &&_

constexpr int num_digits(int n, unsigned b = 10)
{
    int d = 0;
    do
    {
        n /= b;
        ++d;
    } while (n);
    return d;
}

struct int_iterator
{
    int_iterator(int i) : i(i) {}

    int operator*() const { return i; }

    int_iterator &operator++()
    {
        ++i;
        return *this;
    }

    bool operator!=(const int_iterator &other) const { return i != other.i; }

private:
    int i;
};

struct range
{
    range(int from, int to) : from(from), to(to) {}

    explicit range(unsigned n) : range(0, n) {}

    template <class T>
    range(const vector<T> &v) : range(v.size()) {}

    size_t size() const { return to - from; }

    int_iterator begin() const { return int_iterator(from); }

    int_iterator end() const { return int_iterator(to); }

    bool contains(int i) const { return from <= i && i < to; }

private:
    int from, to;
};

template <typename T1, typename T2>
std::pair<T2, T1> swap(const std::pair<T1, T2> &p)
{
    return {p.second, p.first};
}

template <typename T>
std::optional<T> &operator+=(std::optional<T> &o, const T &v)
{
    if (o)
        o.emplace(o.value() + v);
    return o;
}

template <typename T>
std::optional<T> &operator+=(std::optional<T> &o, const std::optional<T> &v)
{
    if (o && v)
        o.emplace(o.value() + v.value());
    return o;
}

template <typename T>
std::optional<T> operator+(const std::optional<T> &lhs, const std::optional<T> &rhs)
{
    if (lhs && rhs)
        return lhs.value() + rhs.value();
    return lhs;
}

struct ColorCode
{
    enum E
    {
        FG_DEFAULT = 39,
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_LIGHT_GRAY = 37,
        FG_DARK_GRAY = 90,
        FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92,
        FG_LIGHT_YELLOW = 93,
        FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95,
        FG_LIGHT_CYAN = 96,
        FG_WHITE = 97,

        BG_DEFAULT = 49,
        BG_BLACK = 40,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_YELLOW = 43,
        BG_BLUE = 44,
        BG_MAGENTA = 45,
        BG_CYAN = 46,
        BG_LIGHT_GRAY = 47,
        BG_DARK_GRAY = 100,
        BG_LIGHT_RED = 101,
        BG_LIGHT_GREEN = 102,
        BG_LIGHT_YELLOW = 103,
        BG_LIGHT_BLUE = 104,
        BG_LIGHT_MAGENTA = 105,
        BG_LIGHT_CYAN = 106,
        BG_WHITE = 107,
    } e;

    ColorCode(E e) : e(e) {}
};

constexpr int NUM_COLORS = 14;

const std::vector<ColorCode> FG = {
    ColorCode::FG_DEFAULT,
    ColorCode::FG_GREEN,
    ColorCode::FG_RED,
    ColorCode::FG_BLUE,
    ColorCode::FG_YELLOW,
    ColorCode::FG_MAGENTA,
    ColorCode::FG_CYAN,
    ColorCode::FG_LIGHT_GREEN,
    ColorCode::FG_LIGHT_RED,
    ColorCode::FG_LIGHT_BLUE,
    ColorCode::FG_LIGHT_YELLOW,
    ColorCode::FG_LIGHT_MAGENTA,
    ColorCode::FG_LIGHT_CYAN,
    ColorCode::FG_LIGHT_GRAY,
    ColorCode::FG_DARK_GRAY,
};

const std::vector<ColorCode> BG = {
    ColorCode::BG_DEFAULT,
    ColorCode::BG_GREEN,
    ColorCode::BG_RED,
    ColorCode::BG_BLUE,
    ColorCode::BG_YELLOW,
    ColorCode::BG_MAGENTA,
    ColorCode::BG_CYAN,
    ColorCode::BG_LIGHT_GREEN,
    ColorCode::BG_LIGHT_RED,
    ColorCode::BG_LIGHT_BLUE,
    ColorCode::BG_LIGHT_YELLOW,
    ColorCode::BG_LIGHT_MAGENTA,
    ColorCode::BG_LIGHT_CYAN,
    ColorCode::BG_LIGHT_GRAY,
    ColorCode::BG_DARK_GRAY,
};

std::ostream &operator<<(std::ostream &os, ColorCode code)
{
    return os << "\033[" << code.e << "m";
}

struct Csv
{
    std::map<string, string> fields;

    const vector<string> &columns;
    std::ostream &out;

    Csv(const vector<string> &columns, std::ostream &out) : columns(columns), out(out) {}

    template <typename T>
    bool set(const string &&field, const T &value)
    {
        std::stringstream ss;
        ss << value;
        return fields.insert({field, ss.str()}).second;
    }

    void clear()
    {
        fields.clear();
    }

    static inline const auto sep = ",";

    void write_header()
    {
        auto i = columns.begin();
        auto e = columns.end();
        if (i == e)
            return;
        out << *i;
        for (++i; i != e; ++i)
            out << sep << *i;
        out << std::endl;
    }

    void write()
    {
        auto i = columns.begin();
        auto e = columns.end();
        if (i == e)
            return;
        out << fields[*i];
        for (++i; i != e; ++i)
            out << sep << fields[*i];
        out << std::endl;
    }
};

template <typename container>
unsigned hashCode(container c) // From a Java implementation
{
    if (!c.size())
        return 0;

    unsigned res = 1;
    for (unsigned e : c)
        res = 31 * res + e;

    return res;
}

template <typename rait, typename rng>
void shuffle_(rait first, rait last, rng &&g)
{
    auto size = last - first;
    for (auto i = size - 1; i > 0; --i)
        std::swap(first[i], first[g() % size]);
}
