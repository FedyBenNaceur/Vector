#pragma once

#include <memory>
#include <type_traits>

//Fedy Ben Naceur---------------------M1 Data Science

// PLEASE READ CAREFULLY

// vector_t --------------------------------------------------------------------

// Properly implementing a vector type is not a trivial task. There are some
// corner cases related to working with uninitialized memory, or managing
// lifetime of moved objects.

// The vector_t class manages two things:
// - a memory buffer,
// - the lifetime of the object it holds.

// The lifetime of a value starts at its construction, and stops at its
// destruction. A value that was moved from is still considered alive and
// therefore should be destroyed like any other value.

// The invariants of the vector_t class are the following:
// - _size should indicate the number of values that are alive,
// - _capacity should indicate the number of values that can be stored in the
// buffer,
// - _size <= _capacity,
// - _data should be either valid or set to nullptr, but never point to an
// invalid address (eg. a to buffer that was deallocated),
// - two vectors should never share the same buffer.

// We expect these invariants to be true even for values that were moved from.

// Warning: destruction and deallocation are two separate things. Destruction is
// related to value lifetime, and deallocation is related to memory management.

// Pro-tip: It might be useful to implement reserve() or resize() before the
// copy constructor or assign operator...

// Lifetime management & uninitialized storage ---------------------------------

// The number of values that are alive (ie. have been constructed)
// is held by _size.

//  - Construction

// Assigning to uninitialized memory (eg. memory that you just allocated) leads
// to undefined behavior. To construct a value on uninitialized memory, you must
// use std::construct_at. It takes an address as first parameter, then the
// constructor arguments.
// (https://en.cppreference.com/w/cpp/memory/construct_at)
// NB: std::construct_at is a C++20 library feature, we provide an
// implementation to ensure it's available to those who must compile in C++17.

// Some std::construct_at examples:

// Default constructing the value of rank i in the buffer:
// std::construct_at(_data + i);

// Copy constructing the value of rank i in the buffer:
// std::construct_at(_data + i, other);

// Move constructing the value of rank i in the buffer:
// std::construct_at(_data + i, std::move(other));

//  - Destruction

// To destroy values, you should use std::destroy_at, std::destroy_n, or
// std::destroy.

// https://en.cppreference.com/w/cpp/memory/destroy_at
// https://en.cppreference.com/w/cpp/memory/destroy_n
// https://en.cppreference.com/w/cpp/memory/destroy

// NB: std::destroy_n and std::destroy will accept pointers as iterators.

// [1]: Some types might be trivially default constructible or trivially
// destructible
// (https://en.cppreference.com/w/cpp/language/destructor#Trivial_destructor).
// This means that their default constructor or destructor does not do anything,
// which means calling std::destroy or std::construct_at for these values is
// basically useless. You will get bonus points if you implement compile-time
// behavior to avoid useless constructions/destructions based on type traits
// such as std::is_trivially_destructible
// (https://en.cppreference.com/w/cpp/types/is_destructible) or
// std::is_trivially_default_constructible.

// Consider destroyed values like uninitialized values. If you want to reuse the
// storage of a value that has been previously destroyed, you should initialize
// it first using std::construct_at.

// If you wish to read more about object lifetime:
// https://en.cppreference.com/w/cpp/language/lifetime

// Memory management -----------------------------------------------------------

// The number of values that can be stored in the current buffer
// is held by _capacity.

// All the memory management should be done through _allocator.

//  - Allocation

// Allocating basically means asking your OS for heap storage. What you get when
// calling _allocator.allocate(n) is a buffer of uninitialized memory. No object
// is constructed at this point, and it's up to you to construct objects in that
// storage using std::construct_at.

// You cannot assign to uninitialized storage. The assign operator (operator=)
// is expected to be called on values in a coherent state. Assigning on
// uninitialized storage therefore has undefined befavior.

//  - Deallocation

// Deallocating essentially means giving allocated memory back to the OS. After
// calling _allocator.deallocate(_data, _capacity), the memory can not be used
// anymore. Deallocating memory does not destroy the objects, and destruction
// must be done before deallocating.

// C++17-compatible, non-constexpr implementation of std::construct_at
// (std::construct_at is a C++20 library feature)
#ifndef __cpp_lib_constexpr_dynamic_alloc
namespace std {
template <typename T, typename... Args>
inline T *construct_at(T *p, Args &&...args) {
  return ::new (const_cast<void *>(static_cast<const volatile void *>(p)))
      T(std::forward<Args>(args)...);
}
} // namespace std
#endif

template<typename T>
struct vector_t {
private:
    /// Pointer to the memory buffer.
    /// It should be either valid or equal nullptr.
    T *_data;

    /// Size of the vector.
    /// Holds the number of alive values.
    std::size_t _size;

    /// Capacity of the memory buffer.
    std::size_t _capacity;

    /// Memory allocator.
    std::allocator<T> _allocator;

public:
    /// Default constructor that initializes an empty vector with no capacity
    vector_t() noexcept: _data(nullptr), _size(0), _capacity(0) {}

    /// The following constructor should initialize a vector of given size. The
    /// capacity should be the same as the size, and all the elements must be
    /// default constructed[1].

    // Calling the default constructor first to ensure the vector
    // is well initialized. Member functions such as resize or reserve
    // should never be used on uninitialized objects.
    explicit vector_t(std::size_t s) : _size(s), _capacity(s) {
        //allocating memory
        _data = _allocator.allocate(s);
        //default constructing
        for (std::size_t i = 0; i < _size; i++) {
            std::construct_at(_data + i);
        }
    }

    //copy constructor
    vector_t(vector_t const &other) : _size(other._size), _capacity(other._capacity) {
        _data = _allocator.allocate(other._capacity);
        for (std::size_t i = 0; i < _size; i++) {
            std::construct_at(_data + i, *(other._data + i));
        }
    }

    //move constructor
    vector_t(vector_t &&other) noexcept: _size(other._size), _capacity(other._capacity) {
        _data = _allocator.allocate(_capacity);
        _data = other._data;
        other._data = nullptr;
        other._size = 0;
    }

    //copy assignment operator
    vector_t &operator=(vector_t const &other) {
        //copy the size and the capacity of the rhs variable
        _size = other._size;
        _capacity = other._capacity;
        //allocate memory using the size the rhs variable
        _data = _allocator.allocate(_capacity);
        //copy constructing values
        for (std::size_t i = 0; i < _size; i++) {
            std::construct_at(_data + i, *(other._data + i));
        }
        return *this;
    }

    //Move assignment operator
    vector_t &operator=(vector_t &&other) noexcept {
        _size = other.size();
        _capacity = other._capacity;
        _data = _allocator.allocate(_capacity);
        //Point the _data to the rhs buffer
        _data = other._data;
        other._data = nullptr;
        //put the size at 0 to not have invalid state
        other._size = 0;
        return *this;
    }

    /// Returns a pointer as an iterator to the beginning of the vector.
    T *begin() { return _data; }

    /// Returns a pointer as an iterator to the end of the vector.
    T *end() { return _data + _size; }

    /// Returns a constant pointer as an iterator to the beginning of the vector.
    T const *begin() const { return _data; }

    /// Returns a constant pointer as an iterator to the end of the vector.
    T const *end() const { return _data + _size; }

    /// Returns the size of the vector.
    std::size_t size() const { return _size; }

    /// Non-const element access for getting and modifying elements.
    T &operator[](std::size_t i) { return _data[i]; }

    /// Read-only element access.
    T const &operator[](std::size_t i) const { return _data[i]; }

    /// emplace_back constructs a new element at the end of the vector.
    /// The arguments are expanded and forwarded to std::construct_at just after
    /// the pointer parameter (https://en.cppreference.com/w/cpp/utility/forward).
    /// If the capacity is zero, you must reserve enough memory for 16 elements.
    /// Otherwise, if the capacity is insufficient, you must reserve double the
    /// current capacity to avoid allocating memory too frequently.
    /// NB: emplace_back can be used to move/copy elements to the vector just like
    /// push_back.

    template<typename... Args>
    void emplace_back(Args &&...args) {
        //reserving twice the capacity if we filled the whole buffer
        if (_size == _capacity)
            reserve(2 * _capacity);
        //default reserve 16 for empty vectors
        if (_capacity == 0)
            reserve(16);
        //putting values at the end of the buffer
        std::construct_at(end(), std::forward<Args>(args)...);
        _size++;
    }

    /// Reserve changes the capacity of the vector.
    /// - It should not change the size of the vector, which means that it should
    /// not change the number of live values,
    /// - if new_capacity < _size, then the new buffer should be resized to _size,
    /// - and if the new capacity is the same as the current, then just do
    /// nothing,
    /// - calling std::reallocate with memory allocated by std::allocator may
    /// result in a runtime error. For that reason you are expected to *not* use
    /// it,
    /// - therefore you are expected to allocate a new buffer, move the values to
    /// that new buffer, and then destroy[1] the values from the old buffer before
    /// deallocating it (values that have been moved should be destroyed too).

    void reserve(std::size_t new_capacity) {
        if (new_capacity < _size) {
            //resizing the vector if the new size is smaller than the new capacity
            resize(_size);
            _capacity = _size;
            return;
        }
        if (new_capacity > _capacity) {
            //allocate enough space for the new capacity and moving values from the old buffer
            T *new_buffer = _allocator.allocate(new_capacity);
            for (std::size_t i = 0; i < _size; i++) {
                std::construct_at(new_buffer + i, std::move(_data[i]));
            }
            //destroying and deallocating old values
            std::destroy(begin(), end());
            _allocator.deallocate(_data, _capacity);
            _data = new_buffer;
            _capacity = new_capacity;
        }
    }

    /// Resize should set the size of the vector, destroying or
    /// default constructing values as necessary[1].
    /// It should also reserve memory as needed.
    void resize(std::size_t new_size) {
        if (new_size > _size) {
            if (new_size > _capacity) {
                //reserve new memory if the new size exceeds the capacity
                reserve(new_size);
            }
            //default constructing values
            for (std::size_t i = _size; i < new_size; i++) {
                std::construct_at(_data + i);
            }
        }
        if (new_size < _size) {
            //if new size is smaller than the previous size we destroy old data
            std::destroy_n(_data + new_size, _size - new_size);
        }
        _size = new_size;
    }

    /// The destructor should destroy[1] all the values that are alive and
    /// deallocate the memory buffer, if there is one.
    ~vector_t() {
        //checking if the pointer is not null
        if (_data) {
            //destroying and deallocating the memory buffer
            std::destroy_n(_data, _size);
            _allocator.deallocate(_data, _capacity);
            _data = nullptr;
        }
    }
};
