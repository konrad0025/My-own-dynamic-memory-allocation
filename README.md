# My-own-functions-to-dynamic-memory-allocation
One of the projects to pass the course operating systems

This project is mainly about creating custom version of the popular
<br><b>dynamic memory allocation functions</b> in cstdlib:<br>
malloc<br>calloc<br>realloc<br>free<br>

### How it works?
Functions for memory management are using sbrk() to increments the program's data space
### Functions
<b>heap_malloc</b>, <b>heap_calloc</b>, <b>heap_realloc</b>, <b>heap_free</b> are working similar to their counterparts is cstdlib,<br>
but before we start using them, we have to init heap with <b>heap_setup()</b> and after all destroy heap with <b>heap_clean()</b>
<br><br>
we have also functions named
<b>heap_malloc_aligned</b>, 
<b>heap_calloc_aligned</b>, 
<b>heap_realloc_aligned</b><br>which
are working very similar to not aligned versions, with one exception<br>
they manage only aligned memory space to the full pages size(4*1024)
