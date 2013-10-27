  /* Opaque buffer element type.  This would be defined by the application. */
typedef struct { unsigned long time; unsigned long diff; float ratio; } Knock;
  
/* Circular buffer object */
typedef struct {
    int         size;   /* maximum number of elements           */
    int         start;  /* index of oldest element              */
    int         end;    /* index at which to write new element  */
    Knock      *elems;  /* vector of elements                   */
} CircularBuffer;
