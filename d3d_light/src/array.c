#include "array.h"

int Array_Init(struct Array *self, unsigned int ElementStride)
{
    self->Data = (unsigned char *)malloc((ARRAY_INCREMENT * ElementStride));
    self->reserved = ARRAY_INCREMENT;
    self->Count = 0;
    self->ElementStride = ElementStride;
    return (self->Data != NULL);
}

void Array_Release(struct Array *self)
{
    free(self->Data);
    ZeroThat(self);
}

void *Array_Append(struct Array *self, unsigned int Count, void *src)
{
    unsigned int accum = self->Count + Count;
    if (accum > self->reserved)
    {
        unsigned int old_reserved = self->reserved;
        while (accum > self->reserved)
            self->reserved += self->ElementStride * ARRAY_INCREMENT;

        self->Data = (unsigned char *)realloc(self->Data, self->reserved);
        memset(&self->Data[old_reserved], 0, (self->reserved - old_reserved));
    }

    unsigned char *dst = &self->Data[self->Count * self->ElementStride];
    if (src)
        memcpy(dst, src, Count * self->ElementStride);
    
    self->Count = accum;
    return dst;
}