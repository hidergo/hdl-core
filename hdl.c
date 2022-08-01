#include "hdl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef CONFIG_ZEPHYR_HDL
    #include <kernel.h>
    // Use k_malloc on zephyr
    #define HMALLOC     k_malloc
    #define HFREE       k_free

#else
    // Default malloc on other platforms
    #define HMALLOC     malloc
    #define HFREE       free

#endif

// Alignment definitions
#define HDL_ALIGN_X_CENTER  0x00
#define HDL_ALIGN_X_LEFT    0x01
#define HDL_ALIGN_X_RIGHT   0x02

#define HDL_ALIGN_Y_MIDDLE  0x00
#define HDL_ALIGN_Y_TOP     0x01
#define HDL_ALIGN_Y_BOTTOM  0x02


const uint8_t TYPE_SIZES[] = {
    0, /* HDL_TYPE_NULL */
    1, /* HDL_TYPE_BOOL */
    4, /* HDL_TYPE_FLOAT */
    0, /* HDL_TYPE_STRING */
    1, /* HDL_TYPE_I8 */
    2, /* HDL_TYPE_I16 */
    4, /* HDL_TYPE_I32 */
    1, /* HDL_TYPE_IMG */
    1, /* HDL_TYPE_BIND */
};

struct HDL_Interface HDL_CreateInterface (uint16_t width, uint16_t height, enum HDL_ColorSpace colorSpace, int features) {
    struct HDL_Interface interface;
    // Zero interface values
    memset(&interface, 0, sizeof(struct HDL_Interface));

    // Set parameters
    interface.width = width;
    interface.height = height;
    interface.colorSpace = colorSpace;
    interface.features = features;
    // Default text width and height
    interface.textWidth = 8;
    interface.textHeight = 8;

    return interface;
}

int _hdl_is_format_spec (char c) {
    return  c == 'd' || c == 'i' || c == 'u' || c == 'o' || 
            c == 'x' || c == 'X' || c == 'f' || c == 'e' || 
            c == 'E' || c == 'g' || c == 'p' || c == 'c' || 
            c == 's';
}

/**
 * @brief Get string width and height with newlines
 * 
 * @param str 
 * @return int 
 */
int _hdl_str_size (char *str, uint16_t *w, uint16_t *h) {
    *w = 0;
    *h = 1;
    int lw = 0;
    for(int i = 0; i < strlen(str); i++) {
        if(str[i] != '\n') {
            lw++;
        }
        else {
            if(lw > *w) {
                *w = lw;
            }
            lw = 0;
            (*h)++;
        }
    }
    if(lw > *w) {
        *w = lw;
    }

    return 0;
}

int _hdl_sprintf_bindings (char *buffer, struct HDL_Interface *interface, struct HDL_Element *element) {
    if(element->content == NULL)
        return 1;
    
    char savedChar = 0;
    int len = strlen(element->content);

    char *start_w = buffer;
    char *start_r = element->content;
    uint8_t state = 0;
    uint8_t bind_index = 0;
    for(int i = 0; i < len; i++) {
        if(state == 0) {
            if(element->content[i] == '%') {
                if(i > 1 && element->content[i - 1] == '\\') {
                    // Escaped format, ignore
                    continue;
                }
                state = 1;
            }
        }
        else if(state == 1) {
            if(_hdl_is_format_spec(element->content[i])) {
                // Format specifier, set next character to terminating character
                // Save old character
                savedChar = element->content[i + 1];
                element->content[i + 1] = 0;
                int lenw = 0;
                switch(element->content[i]) {
                    case 'd':
                    case 'i':
                    case 'u':
                    case 'o':
                    case 'x':
                    case 'X':
                    {
                        // Format INTEGER
                        lenw = sprintf(start_w, start_r, *(int*)interface->bindings[element->bindings[bind_index]]);
                        break;
                    }
                    case 'f':
                    case 'e':
                    case 'E':
                    case 'g':
                    {
                        // Format FLOAT
                        lenw = sprintf(start_w, start_r, *(float*)interface->bindings[element->bindings[bind_index]]);
                        break;
                    }
                    case 'p':
                    {
                        // Format POINTER
                        lenw = sprintf(start_w, start_r, (void*)interface->bindings[element->bindings[bind_index]]);
                        break;
                    }
                    case 'c':
                    {
                        // Format CHARACTER
                        lenw = sprintf(start_w, start_r, *(char*)interface->bindings[element->bindings[bind_index]]);
                        break;
                    }
                    case 's':
                    {
                        // Format STRING
                        lenw = sprintf(start_w, start_r, (char*)interface->bindings[element->bindings[bind_index]]);
                        break;
                    }
                }
                element->content[i + 1] = savedChar;
                start_r = element->content + i + 1;
                start_w = start_w + lenw;

                bind_index++;

                state = 0;
            }
        }
    }
    return 0;
}

int _hdl_handleBoundAttrs (struct HDL_Interface *interface, struct HDL_Element *element) {
    for(int i = 0; i < element->boundAttrCount; i++) {
        struct HDL_AttrBind *battr = &element->bound_attrs[i];
        switch(battr->key) {
            case HDL_ATTR_X:
                element->attrs.x = *(int16_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_Y:
                element->attrs.y = *(int16_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_WIDTH:
                element->attrs.width = *(uint16_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_HEIGHT:
                element->attrs.height = *(uint16_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_FLEX:
                element->attrs.flex = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_FLEX_DIR:
                element->attrs.flexDir = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_BIND:
                // Should not be bound
                return 1;
                break;
            case HDL_ATTR_IMG:
                element->attrs.image = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_PADDING:
                if(battr->count == 1) {
                    element->attrs.padding_x = *(int16_t*)HDL_GetBinding(interface, battr->bind.value);
                    element->attrs.padding_y = *(int16_t*)HDL_GetBinding(interface, battr->bind.value);
                }
                else {
                    element->attrs.padding_x = *(int16_t*)HDL_GetBinding(interface, battr->bind.values[0]);
                    element->attrs.padding_y = *(int16_t*)HDL_GetBinding(interface, battr->bind.values[1]);
                }
                break;
            case HDL_ATTR_ALIGN:
                element->attrs.align = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_SIZE:
                element->attrs.size = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
            case HDL_ATTR_DISABLED:
                element->attrs.disabled = *(uint8_t*)HDL_GetBinding(interface, battr->bind.value);
                break;
        }
    }
    return 0;
}

int _hdl_handleElement (struct HDL_Interface *interface, struct HDL_Element *element) {
    int flags = element->flags;

    // Update element bound attributes
    _hdl_handleBoundAttrs(interface, element);

    if(element->attrs.disabled)
        return flags;

    uint16_t totalFlex = 0;

    // Calculate children flex
    #ifdef HDL_CONF_STATIC_CHILDREN_COUNT
    for(int i = 0; i < HDL_CONF_STATIC_CHILDREN_COUNT; i++) {
#else
    for(int i = 0; i < element->child_count; i++) {
#endif
        if(element->children[i] == 0xFF)
            continue;

        struct HDL_Element *child = &interface->elements[element->children[i]];
        if(child->attrs.disabled)
            continue;

        totalFlex += child->attrs.flex;
    }

    int16_t curFlexX = element->attrs.x;
    int16_t curFlexY = element->attrs.y;
    
    // Loop through children
#ifdef HDL_CONF_STATIC_CHILDREN_COUNT
    for(int i = 0; i < HDL_CONF_STATIC_CHILDREN_COUNT; i++) {
#else
    for(int i = 0; i < element->child_count; i++) {
#endif
        if(element->children[i] == 0xFF)
            continue;

        struct HDL_Element *child = &interface->elements[element->children[i]];
        if(child->attrs.disabled)
            continue;

        child->attrs.x = curFlexX;
        child->attrs.y = curFlexY;

        if(element->attrs.flexDir == HDL_FLEX_ROW) {
            int16_t addF = (uint16_t)ceilf((float)child->attrs.flex / (float)totalFlex * element->attrs.width);
            // Set child width
            child->attrs.width = addF;
            // Height from parent
            child->attrs.height = element->attrs.height;

            curFlexX += addF;
        }
        else if(element->attrs.flexDir == HDL_FLEX_COLUMN) {
            int16_t addF = (uint16_t)ceilf((float)child->attrs.flex / (float)totalFlex * element->attrs.height);
            // Set child height
            child->attrs.height = addF;
            // Width from parent
            child->attrs.width = element->attrs.width;

            curFlexY += addF;
        }


        flags |= _hdl_handleElement(interface, child);

    }

    // DEBUG lines
    /*
    interface->f_hline(element->x, element->y, element->width);
    interface->f_hline(element->x, element->y + element->height, element->width);

    interface->f_vline(element->x, element->y, element->height);
    interface->f_vline(element->x + element->width, element->y, element->height);
    */

    // Set alignment point
    int16_t align_x = 0;
    int16_t align_y = 0;

    uint8_t hzAlign = element->attrs.align >> 4;
    uint8_t vtAlign = element->attrs.align & 0xF;

    uint16_t contW = 0;
    uint16_t contH = 0;

    char content_buffer[256];

    if(element->content != NULL) {
        if(element->bind_count > 0) {
            _hdl_sprintf_bindings(content_buffer, interface, element);
        }
        else {
            strcpy(content_buffer, element->content);
        }
        // Get string size
        _hdl_str_size(content_buffer, &contW, &contH);
        contW *= (interface->textWidth + 1) * element->attrs.size;
        contH *= (interface->textHeight + 1) * element->attrs.size;
    }
    if(element->attrs.image != 0xFF) {
        // Get image size
        uint16_t imgWidth = interface->bitmaps[element->attrs.image].width * element->attrs.size;
        uint16_t imgHeight = interface->bitmaps[element->attrs.image].height * element->attrs.size;

        if(contW < imgWidth) {
            contW = imgWidth;
        }
        if(contH < imgHeight) {
            contH = imgHeight;
        }
    }

    // Y alignment, 0/default is middle
    switch(vtAlign) {
        case HDL_ALIGN_Y_TOP:
        {
            // Top
            align_y = 0;
            break;
        }
        case HDL_ALIGN_Y_BOTTOM:
        {
            // Bottom
            align_y = element->attrs.height - contH;
            break;
        }
        default:
        {
            // Middle
            align_y = (element->attrs.height / 2) - (contH / 2);
            break;
        }
    }
    // X alignment, 0/default is center
    switch(hzAlign) {
        case HDL_ALIGN_X_LEFT:
        {
            align_x = 0;
            break;
        }
        case HDL_ALIGN_X_RIGHT:
        {
            align_x = element->attrs.width - contW;
            break;
        }
        default:
        {
            // Center
            align_x = (element->attrs.width / 2) - (contW / 2);
            break;
        }
    }

    int16_t aligned_x = align_x + element->attrs.x + element->attrs.padding_x;
    int16_t aligned_y = align_y + element->attrs.y + element->attrs.padding_y;


    if(interface->f_text != NULL && element->content != NULL) {
        if(element->bind_count > 0) {
            interface->f_text(aligned_x, aligned_y, content_buffer, element->attrs.size);
        }
        else {
            interface->f_text(aligned_x, aligned_y, content_buffer, element->attrs.size);
        }
    }
    if(interface->f_pixel != NULL && element->attrs.image != 0xFF) {
        if(interface->bitmapCount > element->attrs.image) {
            struct HDL_Bitmap *bmp = &interface->bitmaps[element->attrs.image];
            int px = 0;
            for(int y = 0; y < bmp->height; y++) {
                for(int x = 0; x < bmp->width; x++) {
                    uint8_t bmpData = bmp->data[px / 8] & (1 << (7 - (px % 8)));
                    if(bmpData) {
                        for(int sx = 0; sx < element->attrs.size; sx++) {
                            for(int sy = 0; sy < element->attrs.size; sy++) {
                                interface->f_pixel(aligned_x + (x * element->attrs.size) + sx, aligned_y + (y * element->attrs.size) + sy);
                            }
                        }
                    }
                    px++;
                }
            }
        }
    }
    return flags;
}

// Initializes an element to default values
void HDL_InitElement (struct HDL_Element *element) {
    if(element == NULL)
        return;

    memset(element, 0, sizeof(struct HDL_Element));
    element->attrs.flex = 1;
    element->attrs.flexDir = HDL_FLEX_ROW;
    element->attrs.image = 0xFF;
    element->attrs.size = 1;
    
    // Set children to 0xFF
#ifdef HDL_CONF_STATIC_CHILDREN_COUNT
    memset(element->children, 0xFF, HDL_CONF_STATIC_CHILDREN_COUNT);
#else
    // TODO:
#endif

}

int HDL_SetBinding (struct HDL_Interface *interface, const char *key, uint8_t index, void *binding) {

    if(index >= HDL_CONF_MAX_BINDINGS) {
        // Binding out of bounds
        printf("Error: Binding '%s' out of bounds\r\n", key);
        return 1;
    }

    interface->bindings[index] = binding;

    // TODO: Use key for easier access...

    return 0;
}

void *HDL_GetBinding (struct HDL_Interface *interface, uint8_t index) {

    if(index >= HDL_CONF_MAX_BINDINGS) {
        // Binding out of bounds
        printf("Error: Binding out of bounds\r\n");
        return NULL;
    }

    return interface->bindings[index];
}

// Parses a single element
int _hdl_buildElement (struct HDL_Interface *interface, struct HDL_Element *parent, int *elementIndex, uint8_t *data, int *pc) {

    struct HDL_Element *el = &interface->elements[(*elementIndex)++];
    // Initialize element (zero and set defaults)
    HDL_InitElement(el);
    
    // Tag is the first byte
    el->tag = (uint8_t)data[(*pc)];
    (*pc)++;

    // Set parent
    el->parent = parent;
    
    // Set content
    if(data[*pc] != 0) {
        int contentLength = strlen((const char*)&data[(*pc)]);
        el->content = HMALLOC(contentLength + 1);
        memcpy(el->content, &data[(*pc)], contentLength);
        el->content[contentLength] = 0;

        (*pc) += contentLength + 1;
    }
    else {
        el->content = NULL;
        (*pc)++;
    }

    // Attribute count
    uint8_t attrs = (uint8_t)data[(*pc)];
    (*pc)++;

    for(int a = 0; a < attrs; a++) {
        enum HDL_AttrIndex attrKey = (enum HDL_AttrIndex)data[(*pc)++];
        enum HDL_Type attrType = (enum HDL_Type)data[(*pc)++];
        uint8_t count = data[(*pc)++];

        if(attrType == HDL_TYPE_BIND) {
            // Bound attribute is always uint8_t
            el->bound_attrs[el->boundAttrCount].key = attrKey;
            el->bound_attrs[el->boundAttrCount].count = count;
            if(count == 1) {
                el->bound_attrs[el->boundAttrCount].bind.value = *(uint8_t*)&data[*pc];
                (*pc)++;
            }
            else {
                el->bound_attrs[el->boundAttrCount].bind.values = HMALLOC(sizeof(uint8_t) * count);
                for(int i = 0; i < count; i++) {
                    el->bound_attrs[el->boundAttrCount].bind.values[i] = *(uint8_t*)&data[*pc];
                    (*pc)++;
                }
            }
            el->boundAttrCount++;

            continue;
        }


        // Type checks
        uint8_t typeFail = 0;

        switch(attrKey) {
            // Numbers
            // Integers (8bit)
            case HDL_ATTR_FLEX:
            case HDL_ATTR_FLEX_DIR:
            case HDL_ATTR_SIZE:
            case HDL_ATTR_ALIGN:
            {
                // Should be single 8-bit integer or binding
                if(count > 1 || (attrType != HDL_TYPE_I8 && attrType != HDL_TYPE_BIND)) {
                    // Incorrect value, ignored
                    typeFail = 1;
                }
                break;
            }
            // Image/8-bit integer
            case HDL_ATTR_IMG:
            {
                // Should be single u8
                if(count > 1 || attrType != HDL_TYPE_IMG) {
                    // Incorrect value, ignored
                    typeFail = 1;
                }
                break;
            }
            // Boolean
            case HDL_ATTR_DISABLED:
            {
                // Boolean
                if((attrType != HDL_TYPE_BOOL && attrType != HDL_TYPE_BIND)) {
                    typeFail = 1;
                }
                break;
            }
            // Integers (8/16bit)
            case HDL_ATTR_X:
            case HDL_ATTR_Y:
            case HDL_ATTR_WIDTH:
            case HDL_ATTR_HEIGHT:
            {
                // Should be single 8/16-bit integer
                if(count > 1 || (attrType != HDL_TYPE_I8 && attrType != HDL_TYPE_I16 && attrType != HDL_TYPE_BIND)) {
                    // Incorrect value, ignored
                    typeFail = 1;
                }
                break;
            }
            // Integers (8bit int array)
            case HDL_ATTR_BIND:
            {
                if(attrType != HDL_TYPE_I8) {
                    // Incorrect value, ignored
                    typeFail = 1;
                }
                break;
            }
            // Integers (8/16bit int array)
            case HDL_ATTR_PADDING:
            {
                if((attrType != HDL_TYPE_I8 && attrType != HDL_TYPE_I16 && attrType != HDL_TYPE_BIND)) {
                    // Incorrect value, ignored
                    typeFail = 1;
                }
                break;
            }
        }

        // Save single integer value here
        int32_t tmpVal = 0;
        switch(attrType) {
            case HDL_TYPE_BOOL:
            case HDL_TYPE_IMG:
            case HDL_TYPE_I8:
            case HDL_TYPE_BIND:
            {
                tmpVal = *(int8_t*)&data[*pc];
                break;
            }
            case HDL_TYPE_I16:
            {
                tmpVal = *(int16_t*)&data[*pc];
                break;
            }
            case HDL_TYPE_I32:
            {
                tmpVal = *(int32_t*)&data[*pc];
                break;
            }
        }

        if(!typeFail) {
            switch(attrKey) {
                case HDL_ATTR_X:
                {
                    el->attrs.x = tmpVal;
                    break;
                }
                case HDL_ATTR_Y:
                {
                    el->attrs.y = tmpVal;
                    break;
                }
                case HDL_ATTR_WIDTH:
                {
                    el->attrs.width = tmpVal;
                    break;
                }
                case HDL_ATTR_HEIGHT:
                {
                    el->attrs.height = tmpVal;
                    break;
                }
                case HDL_ATTR_FLEX:
                {
                    el->attrs.flex = tmpVal;
                    break;
                }
                case HDL_ATTR_FLEX_DIR:
                {
                    el->attrs.flexDir = tmpVal;
                    break;
                }
                case HDL_ATTR_BIND:
                {
                    el->bindings = HMALLOC(sizeof(uint8_t) * count);
                    el->bind_count = count;
                    for(int x = 0; x < count; x++) {
                        el->bindings[x] = (uint8_t)((uint8_t*)&data[(*pc)])[x];
                    }
                    break;
                }
                case HDL_ATTR_IMG:
                {
                    el->attrs.image = tmpVal;
                    break;
                }
                case HDL_ATTR_PADDING:
                {
                    if(count > 1) {
                        // Paddings seperately
                        if(attrType == HDL_TYPE_I8) {
                            el->attrs.padding_x = (uint8_t)((uint8_t*)&data[(*pc)])[0];
                            el->attrs.padding_y = (uint8_t)((uint8_t*)&data[(*pc)])[1];
                        }
                        else {
                            el->attrs.padding_x = (uint16_t)((uint16_t*)&data[(*pc)])[0];
                            el->attrs.padding_y = (uint16_t)((uint16_t*)&data[(*pc)])[1];
                        }
                    }
                    else {
                        // Both paddings from single value
                        el->attrs.padding_x = tmpVal;
                        el->attrs.padding_y = tmpVal;
                    }
                    break;
                }
                case HDL_ATTR_ALIGN:
                {
                    el->attrs.align = tmpVal;
                    break;
                }
                case HDL_ATTR_DISABLED:
                {
                    el->attrs.disabled = tmpVal;
                    break;
                }
                case HDL_ATTR_SIZE:
                {
                    el->attrs.size = tmpVal;
                    break;
                }

            }
        }

        // Increment pc
        if(attrType < sizeof(TYPE_SIZES)) {
            if(attrType == HDL_TYPE_STRING) {
                while(data[(*pc)++] != 0) {
                    // Loop until null termination
                }
            }
            else {
                (*pc) += TYPE_SIZES[attrType] * count;
            }
        }
    }

    if(parent == NULL) {
        interface->root = el;
        // Set width and height to maximum if root element
        if(el->attrs.height == 0 && el->attrs.width == 0) {
            el->attrs.width = interface->width;
            el->attrs.height = interface->height;
        }
    }

    // Child count
    el->child_count = (uint8_t)data[(*pc)];
    (*pc)++;

    for(int i = 0; i < el->child_count; i++) {
        el->children[i] = (uint8_t)(*elementIndex);
        int err = _hdl_buildElement(interface, el, elementIndex, data, pc);
        if(err) {

            return HDL_ERR_PARSE;
        }
    }

    return 0;
}

int _hdl_buildBitmap (struct HDL_Interface *interface, struct HDL_Bitmap *bmp, uint8_t *data, int *pc) {

    bmp->size = *(uint16_t*)&data[*pc];
    (*pc) += 2;
    bmp->width = *(uint16_t*)&data[*pc];
    (*pc) += 2;
    bmp->height = *(uint16_t*)&data[*pc];
    (*pc) += 2;
    bmp->colorMode = data[*pc];
    (*pc) += 1;

    bmp->data = HMALLOC(bmp->size);
    memcpy(bmp->data, &data[*pc], bmp->size);

    (*pc) += bmp->size;
    return 0;
}

int HDL_Build (struct HDL_Interface *interface, uint8_t *data, uint32_t len) {
    
    if(len < sizeof(struct HDL_Header)) {
        // File too short
        return HDL_ERR_PARSE;
    }
    // Header should be at the start
    struct HDL_Header *header = (struct HDL_Header*)data;

    // Start point just after header
    int pc = sizeof(struct HDL_Header);

    // Allocate bitmaps
    interface->bitmapCount = header->bitmapCount;
    interface->bitmaps = (struct HDL_Bitmap*)HMALLOC(sizeof(struct HDL_Bitmap) * interface->bitmapCount);

    if(interface->bitmaps == NULL)
        return HDL_ERR_MEMORY;

    // Allocate vartable TODO:

    // Allocate elements
    interface->elementCount = header->elementCount;
    interface->elements = (struct HDL_Element*)HMALLOC(sizeof(struct HDL_Element) * interface->elementCount);

    if(interface->elements == NULL)
        return HDL_ERR_MEMORY;

    int err = 0;
    for(int i = 0; i < interface->bitmapCount; i++) {
        if((err = _hdl_buildBitmap(interface, &interface->bitmaps[i], data, &pc))) {

            return err;
        }
    }

    int elementIndex = 0;
    // Start parsing elements
    err = _hdl_buildElement(interface, NULL, &elementIndex, data, &pc);

    return err;
}

int HDL_Update (struct HDL_Interface *interface) {

    if(interface->root == NULL)
        return HDL_ERR_NO_ROOT;

    // TODO: clear screen?
    interface->f_clear(0, 0, interface->width, interface->height);

    _hdl_handleElement(interface, interface->root);

    // Use partial refresh rather than full refresh if defined
    if(interface->f_renderPart != NULL) {
        // TODO: render only changed parts!
        interface->f_renderPart(0, 0, interface->width, interface->height);
    }
    else {
        interface->f_render();
    }

    return 0;
}

void _hdl_freeElement (struct HDL_Element *element) {
    if(element->bindings != NULL)
        HFREE(element->bindings);

    if(element->content != NULL)
        HFREE(element->content);
}

void HDL_Free (struct HDL_Interface *interface) {
    // Free elements data
    for(int i = 0; i < interface->elementCount; i++) {
        _hdl_freeElement(&interface->elements[i]);
    }
    // Free elements
    if(interface->elements != NULL) {
        HFREE(interface->elements);
    }
}