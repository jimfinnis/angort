/**
 * @file
 * Assorted system functions
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <angort/plugins.h>

using namespace angort;

%word sleep 1 (time --) sleep for some time
{
    float t = params[0].getFloat();
    usleep((int)(t*1.0e6f));
}

%init
{
    printf("Initialising SYS plugin, %s %s\n",__DATE__,__TIME__);
}

