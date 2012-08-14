
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include "glsmapint.h"

void
smapSetContextData(SphereMap *smap, void *context)
{
	smap->context = context;
}

void
smapGetContextData(SphereMap *smap, void **context)
{
	*context = smap->context;
}
