#ifndef __HDL_CONF
#define __HDL_CONF

// Define HDL_CONF_16BIT_CHILD_COUNT if an element has more than 255 children
// #define HDL_CONF_16BIT_CHILD_COUNT

// Define HDL_CONF_STATIC_CHILDREN_COUNT <number> if you want to have the children 
// in statically allocated array, uses more memory but doesn't need malloc
#define HDL_CONF_STATIC_CHILDREN_COUNT 16

// Define HDL_CONF_USE_KVP_ATTR if you want custom attributes
// #define HDL_CONF_USE_KVP_ATTR

// Max count of bindings
#define HDL_CONF_MAX_BINDINGS   16

// Max count of bound attributes
#define HDL_CONF_MAX_ATTR_BINDINGS  8

#endif