

#ifndef UTILSPP_TRAITS_H
#define UTILSPP_TRAITS_H

#ifndef __INTEL_COMPILER
#ifdef __GNUG__
#pragma interface
#endif
#endif

#include "mach_types.h"
#include "flavors.h"

// Inspiration STL ...

struct true_type { };

struct false_type { };

template <class _Tp>
struct type_traits { 
    typedef true_type     this_dummy_member_must_be_first;
    /* Do not remove this member. It informs a compiler which
       automatically specializes type_traits that this
       type_traits template is special. It just makes sure that
       things work if an implementation is using a template
       called type_traits for something unrelated. */
 
    typedef false_type    has_trivial_default_constructor;
    typedef false_type    has_trivial_copy_constructor;
    typedef false_type    has_trivial_assignment_operator;
    typedef false_type    has_trivial_destructor;
    typedef false_type    is_POD_type;
    typedef false_type    have_no_all_operators;
    typedef false_type    is_not_int_compatible;
    typedef false_type    is_not_float_compatible;
    typedef false_type    is_not_float_pointers_compatible;
    typedef false_type    is_ORAFF_compatible;
    
};

TEMPLATE_NULL struct type_traits<bool *> {
    typedef true_type    has_trivial_default_constructor;
    typedef true_type    has_trivial_copy_constructor;
    typedef true_type    has_trivial_assignment_operator;
    typedef true_type    has_trivial_destructor;
    typedef true_type    is_POD_type;
    typedef true_type    have_no_all_operators;
    typedef true_type    is_not_int_compatible;
    typedef true_type    is_not_float_compatible;
    typedef true_type    is_not_float_pointers_compatible;
    typedef false_type   is_ORAFF_compatible;
};


TEMPLATE_NULL struct type_traits<float_p> {
    typedef true_type    has_trivial_default_constructor;
    typedef true_type    has_trivial_copy_constructor;
    typedef true_type    has_trivial_assignment_operator;
    typedef true_type    has_trivial_destructor;
    typedef true_type    is_POD_type;
    typedef true_type    have_no_all_operators; 
    typedef true_type    is_not_int_compatible;
    typedef false_type   is_not_float_compatible;
    typedef false_type   is_not_float_pointers_compatible;
    typedef false_type   is_ORAFF_compatible;
};

TEMPLATE_NULL struct type_traits<int_p> {
    typedef true_type    has_trivial_default_constructor;
    typedef true_type    has_trivial_copy_constructor;
    typedef true_type    has_trivial_assignment_operator;
    typedef true_type    has_trivial_destructor;
    typedef true_type    is_POD_type;
    typedef false_type   have_no_all_operators;
    typedef false_type   is_not_int_compatible;
    typedef true_type    is_not_float_compatible;
    typedef true_type    is_not_float_pointers_compatible;
    typedef true_type    is_ORAFF_compatible;
};

TEMPLATE_NULL struct type_traits<u_int_p> {
    typedef true_type    has_trivial_default_constructor;
    typedef true_type    has_trivial_copy_constructor;
    typedef true_type    has_trivial_assignment_operator;
    typedef true_type    has_trivial_destructor;
    typedef true_type    is_POD_type;
    typedef false_type   have_no_all_operators;
    typedef false_type   is_not_int_compatible;
    typedef true_type    is_not_float_compatible; 
    typedef true_type    is_not_float_pointers_compatible;
    typedef false_type   is_ORAFF_compatible;
};

TEMPLATE_NULL struct type_traits<short*> {
    typedef true_type    has_trivial_default_constructor;
    typedef true_type    has_trivial_copy_constructor;
    typedef true_type    has_trivial_assignment_operator;
    typedef true_type    has_trivial_destructor;
    typedef true_type    is_POD_type;
    typedef false_type   have_no_all_operators; 
    typedef false_type   is_not_int_compatible;
    typedef true_type    is_not_float_compatible;
    typedef true_type    is_not_float_pointers_compatible;
    typedef false_type   is_ORAFF_compatible;
};

/*
  TEMPLATE_NULL struct type_traits<complex_float_p> {
  typedef true_type    has_trivial_default_constructor;
  typedef true_type    has_trivial_copy_constructor;
  typedef true_type    has_trivial_assignment_operator;
  typedef true_type    has_trivial_destructor;
  typedef true_type    is_POD_type;
  typedef false_type   have_no_all_operators;
  typedef true_type    is_not_int_compatible;
  typedef false_type   is_not_float_compatible;
  typedef true_type    is_not_float_pointers_compatible;
  typedef false_type   is_ORAFF_compatible;
  };
*/

#endif
