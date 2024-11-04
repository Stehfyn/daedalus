#include "pch.h"
#include "framework.h"
#include "daedalus.h"

static int s_initialized;

EXTERNC int daedalus_init()
{
    assert(!s_initialized);



    return s_initialized;
}
