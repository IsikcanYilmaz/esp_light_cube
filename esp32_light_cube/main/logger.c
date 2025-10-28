#include <stdio.h>
#include <stdarg.h>

// TODO // Debug levels and modules. Right now this thing is basically just printf

void logprint( const char* format, ... ) 
{
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    va_end( args );
}
