#ifndef __HDL_H
#define __HDL_H

#include <stdint.h>
#include "hdl_conf.h"

// Horizontal and vertical line
#define HDL_FEAT_LINE_HV    0b0001
// Diagonal line
#define HDL_FEAT_LINE_DIAG  0b0010
// Text
#define HDL_FEAT_TEXT       0b0100
// Bitmap
#define HDL_FEAT_BITMAP     0b1000
// Arc
#define HDL_FEAT_ARC        0b1000

// Element flags
// Dirty - content changed
#define HDL_FLAG_CONTENT_CHANGED        0b1
// Dirty - bounds changed
#define HDL_FLAG_BOUNDS_CHANGED         0b01


#define HDL_FLEX_ROW            0x01
#define HDL_FLEX_COLUMN         0x02

// Errors
#define HDL_ERR_NO_ROOT     1
#define HDL_ERR_PARSE       2
#define HDL_ERR_MEMORY      3

// Tagnames
#define HDL_TAG_BOX         0
#define HDL_TAG_SWITCH      1

enum HDL_Type {
    HDL_TYPE_NULL       = 0,
    HDL_TYPE_BOOL       = 1,
    HDL_TYPE_FLOAT      = 2,
    HDL_TYPE_STRING     = 3,
    HDL_TYPE_I8         = 4,
    HDL_TYPE_I16        = 5,
    HDL_TYPE_I32        = 6,
    HDL_TYPE_IMG        = 7,
    HDL_TYPE_BIND       = 8,

    // Tell's how many types have been defined
    HDL_TYPE_COUNT
};

// Attribute indices
enum HDL_AttrIndex {
    HDL_ATTR_X          = 0, // X position
    HDL_ATTR_Y          = 1, // Y position
    HDL_ATTR_WIDTH      = 2, // Width
    HDL_ATTR_HEIGHT     = 3, // Height
    HDL_ATTR_FLEX       = 4, // Flex 
    HDL_ATTR_FLEX_DIR   = 5, // Flex dir
    HDL_ATTR_BIND       = 6, // Bindings
    HDL_ATTR_IMG        = 7, // Bitmap image
    HDL_ATTR_PADDING    = 8, // Padding
    HDL_ATTR_ALIGN      = 9, // Content alignment
    HDL_ATTR_SIZE       = 10, // Bitmap size (+ font size)
    HDL_ATTR_DISABLED   = 11, // Disabled
    HDL_ATTR_VALUE      = 12, // Value
    HDL_ATTR_SPRITE     = 13, // Sprite index
};


// HDL Colorspace 
enum HDL_ColorSpace {
    HDL_COLORS_UNKNOWN,
    // MONO (black and white)
    HDL_COLORS_MONO,
    // RGB colors
    HDL_COLORS_24BIT,
    // Color pallette
    HDL_COLORS_PALLETTE
};

struct HDL_Bitmap {
    uint16_t size;
    uint16_t width;
    uint16_t height;
    uint8_t sprite_width;
    uint8_t sprite_height;
    uint8_t colorMode;
    uint8_t *data;
};

// HDL File header
struct __attribute__((packed)) HDL_Header {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t bitmapCount;
    uint8_t vartableCount;
    uint16_t elementCount;
    uint8_t __padding[10]; // Reserved data
};

#ifdef HDL_CONF_USE_KVP_ATTR
// Single HDL attribute
struct HDL_Attr {
    uint8_t key;
    union value
    {
        int intval;
        float floatval;
    };
};
#else
// All default attributes
struct HDL_Attrs {
    // X position
    int16_t x;
    // Y position
    int16_t y;
    // Horizontal size
    uint16_t width;
    // Vertical size
    uint16_t height;

    // Disabled flag
    uint8_t disabled;

    // Flex value
    uint8_t flex;
    // Flex direction
    uint8_t flexDir;
    // Image index
    uint8_t image;

    // Padding
    int16_t padding_x;
    int16_t padding_y;

    // Alignment
    uint8_t align;
    // Size
    uint8_t size;

    // Value - TODO: should be any type
    uint8_t value;

    // Sprite index
    uint8_t sprite;
};
#endif

// Attribute binding
struct __attribute__((packed)) HDL_AttrBind {
    uint8_t key;
    union {
        uint8_t value;
        uint8_t *values;    
    } bind;
    uint8_t count;
};

// HDL Element
struct HDL_Element {
    // Element type
    uint8_t tag;
    
    // Element content
    char *content;
    // Bindings
    uint8_t *bindings;
    // Binding count
    uint8_t bind_count;
    // Flags
    uint8_t flags;
    // Parent element 
    struct HDL_Element *parent;
    // Children
#ifdef HDL_CONF_STATIC_CHILDREN_COUNT
    // 0xFF if the child does not exist
    uint8_t children[HDL_CONF_STATIC_CHILDREN_COUNT];
#else
    // Dynamic children array
    uint8_t *children;
#endif

#ifdef HDL_CONF_USE_KVP_ATTR
    struct HDL_Attr *attrs;
#else
    struct HDL_Attrs attrs;
#endif
    // Bound attributes
    struct HDL_AttrBind bound_attrs[HDL_CONF_MAX_ATTR_BINDINGS];
    uint8_t boundAttrCount;


    // Child count
#ifdef HDL_CONF_16BIT_CHILD_COUNT
    uint16_t child_count;
#else
    uint8_t child_count;
#endif
};

struct HDL_Bounds {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

// HDL display interfaces
struct HDL_Interface {
    // Width of the screen
    uint16_t width;
    // Height of the screen
    uint16_t height;
    // Color space of the screen
    enum HDL_ColorSpace colorSpace;
    // HDL_FEAT_* values OR'ed, telling the features for the display 
    int features;

    // Root element
    struct HDL_Element *root;

    // Array of all elements
    struct HDL_Element *elements;

    // Element count
    uint16_t elementCount;

    void *bindings[HDL_CONF_MAX_BINDINGS];

    struct HDL_Bitmap *bitmaps;
    uint16_t bitmapCount;

    // Text width on size 1 font
    uint8_t textWidth;
    // Text height on size 1 font
    uint8_t textHeight;

    // Display driver interfaces

    // Clear screen
    void (*f_clear)(int16_t x, int16_t y, uint16_t w, uint16_t h);
    // Set color
    void (*f_setColor)(uint8_t r, uint8_t g, uint8_t b);
    // Fast horizontal line
    void (*f_hline)(int16_t sx, int16_t sy, int16_t len);
    // Fast vertical line
    void (*f_vline)(int16_t sx, int16_t sy, int16_t len); 
    // Text
    void (*f_text)(int16_t x, int16_t y, const char *text, uint8_t fontSize);
    // Set pixel
    void (*f_pixel)(int16_t x, int16_t y);

    // Render the whole screen
    void (*f_render)();
    // Render part of the screen, useful for e-paper displays
    void (*f_renderPart)(int16_t x, int16_t y, uint16_t w, uint16_t h);


};

// Creates and initializes an interface
struct HDL_Interface HDL_CreateInterface (uint16_t width, uint16_t height, enum HDL_ColorSpace colorSpace, int features);

// Builds the display
int HDL_Build (struct HDL_Interface *interface, uint8_t *data, uint32_t len);

// Handle HDL updates
int HDL_Update (struct HDL_Interface *interface);

// Cleanup
void HDL_Free (struct HDL_Interface *interface);

// Add a binding
int HDL_SetBinding (struct HDL_Interface *interface, const char *key, uint8_t index, void *binding);

// Get a binding
void *HDL_GetBinding (struct HDL_Interface *interface, uint8_t index);

#endif