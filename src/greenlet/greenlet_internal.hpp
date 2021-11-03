/* -*- indent-tabs-mode: nil; tab-width: 4; -*- */
#ifndef GREENLET_INTERNAL_H
#define GREENLET_INTERNAL_H
#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-function"
#    pragma clang diagnostic ignored "-Wmissing-field-initializers"
#    pragma clang diagnostic ignored "-Wunused-variable"
#endif

/**
 * Implementation helpers.
 *
 * C++ templates and inline functions should go here.
 */
#include "greenlet_compiler_compat.hpp"
#include "greenlet_cpython_compat.hpp"
#include "greenlet_exceptions.hpp"
#include "greenlet_greenlet.hpp"

#define GREENLET_MODULE
namespace greenlet {
    class ThreadState;
    class ExceptionState;
};
struct _PyMainGreenlet;
class SwitchingState;
#define main_greenlet_ptr_t struct _PyMainGreenlet*
#define switching_state_ptr_t SwitchingState*
#define exception_state_ptr_t greenlet::ExceptionState


#include "greenlet.h"

#include <vector>
#include <memory>
#include <stdexcept>


extern PyTypeObject PyGreenlet_Type;

// Define a special type for the main greenlets. This way it can have
// a thread state pointer without having to carry the expense of a
// NULL field around on every other greenlet.
// At the Python level, the main greenlet class is
// *almost* indistinguisable from plain greenlets.
typedef struct _PyMainGreenlet
{
    PyGreenlet super;
    greenlet::ThreadState* volatile thread_state;
} PyMainGreenlet;

// GCC and clang support mixing designated and non-designated
// initializers; recent MSVC requires ``/std=c++20`` to use
// designated initializer, and doesn't permit mixing. And then older
// MSVC doesn't support any of it.
static PyTypeObject PyMainGreenlet_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "greenlet.main_greenlet", // tp_name
    sizeof(PyMainGreenlet)
};




namespace greenlet
{

    // This allocator is stateless; all instances are identical.
    // It can *ONLY* be used when we're sure we're holding the GIL
    // (Python's allocators require the GIL).
    template <class T>
    struct PythonAllocator : public std::allocator<T> {

        PythonAllocator(const PythonAllocator& other)
            : std::allocator<T>()
        {
            UNUSED(other);
        }

        PythonAllocator(const std::allocator<T> other)
            : std::allocator<T>(other)
        {}

        template <class U>
        PythonAllocator(const std::allocator<U>& other)
            : std::allocator<T>(other)
        {
        }

        PythonAllocator() : std::allocator<T>() {}

        T* allocate(size_t number_objects, const void* hint=0)
        {
            UNUSED(hint);
            void* p;
            if (number_objects == 1)
                p = PyObject_Malloc(sizeof(T));
            else
                p = PyMem_Malloc(sizeof(T) * number_objects);
            return static_cast<T*>(p);
        }

        void deallocate(T* t, size_t n)
        {
            void* p = t;
            if (n == 1) {
                PyObject_Free(p);
            }
            else
                PyMem_Free(p);
        }

    };

    typedef std::vector<PyGreenlet*, PythonAllocator<PyGreenlet*> > g_deleteme_t;

};


/**
  * Forward declarations needed in multiple files.
  */
static PyMainGreenlet* green_create_main();
static PyObject* green_switch(PyGreenlet* self, PyObject* args, PyObject* kwargs);

#ifdef __clang__
#    pragma clang diagnostic pop
#endif


#endif

// Local Variables:
// flycheck-clang-include-path: ("../../include" "/opt/local/Library/Frameworks/Python.framework/Versions/3.10/include/python3.10")
// End:
