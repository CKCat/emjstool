/*
 * tinf  -  tiny inflate library (inflate, gzip, zlib)
 * version 1.00
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 * http://www.ibsensoftware.com/
 */

#ifndef TINF_H_INCLUDED
#define TINF_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define TINF_OK             0
#define TINF_DATA_ERROR    (-3)

void tinf_init();
int tinf_uncompress(void *dest, unsigned int *destLen, const void *source);
int tinf_zlib_uncompress(void *dest, unsigned int *destLen, const void *source, unsigned int sourceLen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TINF_H_INCLUDED */
