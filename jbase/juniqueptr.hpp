// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JAUTOPTR_HPP_HEADER_GUARD
#define JAUTOPTR_HPP_HEADER_GUARD

#include <algorithm>
#include <functional>
#include <utility>

namespace jjm
{


struct InvokeFree
{
    template <typename pointer_t> 
    void operator() (pointer_t p) const
    {
        ::free(p); 
    }
};

struct InvokeDelete
{   
    template <typename pointer_t> 
    void operator() (pointer_t p) const
    {   
        //Ensure that we're not deleting a pointer to void or a pointer to
        //an incomplete type. 
        typedef char staticassert[sizeof(*p) ? 1 : -1];
        (void) sizeof(staticassert);

        delete p;
    }
};

struct InvokeDeleteArray
{   
    template <typename pointer_t> 
    void operator() (pointer_t p) const
    {   
        //Ensure that we're not deleting a pointer to void or a pointer to
        //an incomplete type. 
        typedef char staticassert[sizeof(*p) ? 1 : -1];
        (void) sizeof(staticassert);

        delete[] p; 
    }
};


//Note: if you were using std::auto_ptr<T>, you want to use UniquePtr<T*>. 
//Note: UniquePtr<pimpl*> is safe to use for the pimpl aka compiler firewall 
//idiom. This will fail with a loud annoying compiler error if it tries to 
//delete a pointer to an incomplete type with InvokeDelete. 
template <typename T, typename deleter_t = InvokeDelete>
class UniquePtr
{
private:
    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {} //TODO apparently visual studios sucks, and can convert pointer-to-member-function to pointer-to-void, figure out some other way to do this
public:
    explicit UniquePtr(T x = T()) : p(x) {}
    ~UniquePtr() { deleter_t()(p); }
    T release() { T x = p; p = T(); return x; }
    void reset(T x = T()) { deleter_t()(p); p = x; }
    void swap(UniquePtr& x) { using std::swap; swap(p, x.p); }
    T & get() { return p; }
    T const& get() const { return p; }
    operator unspecified_bool_type () const { return p ? unspecified_bool_true : 0; }
    bool operator! () const { return ! static_cast<bool>(*this); }
    friend bool operator == (UniquePtr const& a, UniquePtr const& b) { return a.p == b.p; }
    friend bool operator != (UniquePtr const& a, UniquePtr const& b) { return !(a==b); }
    friend bool operator <  (UniquePtr const& a, UniquePtr const& b) { return std::less<T>()(a.get(), b.get()); }
    friend bool operator <= (UniquePtr const& a, UniquePtr const& b) { return a<b || a==b; }
    friend bool operator >  (UniquePtr const& a, UniquePtr const& b) { return b<a; }
    friend bool operator >= (UniquePtr const& a, UniquePtr const& b) { return b<a || a==b; }
private:
    UniquePtr(UniquePtr const& ); //not defined, not copyable
    UniquePtr operator= (UniquePtr const& ); //not defined, not copyable
    T p;
};
template <typename T, typename deleter_t> 
    void swap(UniquePtr<T, deleter_t>& a, UniquePtr<T, deleter_t>& b) 
        { a.swap(b); }


template <typename T, typename deleter_t>
class UniquePtr<T*, deleter_t>
{
private:
    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {}
public:
    explicit UniquePtr(T* x = 0) : p(x) {}
    ~UniquePtr() { deleter_t()(p); }
    T* release() { T* x = p; p = 0; return x; }
    void reset(T* x = 0) { deleter_t()(p); p = x; }
    void swap(UniquePtr& x) { using std::swap; swap(p, x.p); }
    T* get() const { return p; }
    operator unspecified_bool_type () const { return p ? unspecified_bool_true : 0; }
    bool operator! () const { return ! static_cast<bool>(*this); }
    T& operator* () const { return *p; }
    T* operator-> () const { return p; }
    friend bool operator == (UniquePtr const& a, UniquePtr const& b) { return a.p == b.p; }
    friend bool operator != (UniquePtr const& a, UniquePtr const& b) { return !(a==b); }
    friend bool operator <  (UniquePtr const& a, UniquePtr const& b) { return std::less<T*>()(a.get(), b.get()); }
    friend bool operator <= (UniquePtr const& a, UniquePtr const& b) { return a<b || a==b; }
    friend bool operator >  (UniquePtr const& a, UniquePtr const& b) { return b<a; }
    friend bool operator >= (UniquePtr const& a, UniquePtr const& b) { return b<a || a==b; }
private:
    UniquePtr(UniquePtr const& ); //not defined, not copyable
    UniquePtr operator= (UniquePtr const& ); //not defined, not copyable
    T* p;
};


template <typename deleter_t>
class UniquePtr<void*, deleter_t>
{
private:
    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {} 
public:
    explicit UniquePtr(void* x = 0) : p(x) {}
    ~UniquePtr() { deleter_t()(p); }
    void* release() { void* x = p; p = 0; return x; }
    void reset(void* x = 0) { deleter_t()(p); p = x; }
    void swap(UniquePtr& x) { using std::swap; swap(p, x.p); }
    void* get() const { return p; }
    operator unspecified_bool_type () const { return p ? unspecified_bool_true : 0; }
    bool operator! () const { return ! static_cast<bool>(*this); }
    friend bool operator == (UniquePtr const& a, UniquePtr const& b) { return a.p == b.p; }
    friend bool operator != (UniquePtr const& a, UniquePtr const& b) { return !(a==b); }
    friend bool operator <  (UniquePtr const& a, UniquePtr const& b) { return std::less<void*>()(a.get(), b.get()); }
    friend bool operator <= (UniquePtr const& a, UniquePtr const& b) { return a<b || a==b; }
    friend bool operator >  (UniquePtr const& a, UniquePtr const& b) { return b<a; }
    friend bool operator >= (UniquePtr const& a, UniquePtr const& b) { return b<a || a==b; }
private:
    UniquePtr(UniquePtr const& ); //not defined, not copyable
    UniquePtr operator= (UniquePtr const& ); //not defined, not copyable
    void* p;
};


template <typename T, typename deleter_t = InvokeDeleteArray >
class UniqueArray;
template <typename T, typename deleter_t>
class UniqueArray<T*, deleter_t>
{
private:
    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {}
public:
    explicit UniqueArray(T* x = 0) : p(x) {}
    ~UniqueArray() { deleter_t()(p); }
    T* release() { T* x = p; p = 0; return x; }
    void reset(T* x = 0) { deleter_t()(p); p = x; }
    void swap(UniqueArray& x) { using std::swap; swap(p, x.p); }
    T* get() const { return p; }
    operator unspecified_bool_type () const { return p ? unspecified_bool_true : 0; }
    bool operator! () const { return ! static_cast<bool>(*this); }
    T&  operator[] (std::size_t n) const { return p[n]; }
    friend bool operator == (UniqueArray const& a, UniqueArray const& b) { return a.p == b.p; }
    friend bool operator != (UniqueArray const& a, UniqueArray const& b) { return !(a==b); }
    friend bool operator <  (UniqueArray const& a, UniqueArray const& b) { return std::less<T*>(a, b); }
    friend bool operator <= (UniqueArray const& a, UniqueArray const& b) { return a<b || a==b; }
    friend bool operator >  (UniqueArray const& a, UniqueArray const& b) { return b<a; }
    friend bool operator >= (UniqueArray const& a, UniqueArray const& b) { return b<a || a==b; }
private:
    UniqueArray(UniqueArray const& ); //not defined, not copyable
    UniqueArray operator= (UniqueArray const& ); //not defined, not copyable
    T* p;
};
template <typename T, typename deleter_t> 
    void swap(UniqueArray<T, deleter_t>& a, UniqueArray<T, deleter_t>& b) 
        { a.swap(b); }



} //namespace jjm

#endif
