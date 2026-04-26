/*
 * tinflate  -  tiny inflate
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 * http://www.ibsensoftware.com/
 */

#include "tinf.h"

typedef struct {
   unsigned short table[16];  /* table of code length counts */
   unsigned short trans[288]; /* code -> symbol translation table */
} TINF_TREE;

typedef struct {
   const unsigned char *source;
   unsigned int tag;
   unsigned int bitcount;

   unsigned char *dest;
   unsigned int *destLen;

   TINF_TREE ltree; /* dynamic length/symbol tree */
   TINF_TREE dtree; /* dynamic distance tree */
} TINF_DATA;

static TINF_TREE sltree; /* fixed length/symbol tree */
static TINF_TREE sdtree; /* fixed distance tree */

static unsigned char length_bits[30];
static unsigned short length_base[30];
static unsigned char dist_bits[30];
static unsigned short dist_base[30];

static const unsigned char clcidx[] = {
   16, 17, 18, 0, 8, 7, 9, 6,
   10, 5, 11, 4, 12, 3, 13, 2,
   14, 1, 15
};

static void tinf_build_bits_base(unsigned char *bits, unsigned short *base, int delta, int first)
{
   int i, sum;
   for (i = 0; i < delta; ++i) bits[i] = 0;
   for (i = 0; i < 30 - delta; ++i) bits[i + delta] = (unsigned char)(i / delta);
   for (sum = first, i = 0; i < 30; ++i) {
      base[i] = (unsigned short)sum;
      sum += 1 << bits[i];
   }
}

static void tinf_build_fixed_trees(TINF_TREE *lt, TINF_TREE *dt)
{
   int i;
   for (i = 0; i < 7; ++i) lt->table[i] = 0;
   lt->table[7] = 24; lt->table[8] = 152; lt->table[9] = 112;
   for (i = 0; i < 24; ++i) lt->trans[i] = (unsigned short)(256 + i);
   for (i = 0; i < 144; ++i) lt->trans[24 + i] = (unsigned short)i;
   for (i = 0; i < 8; ++i) lt->trans[24 + 144 + i] = (unsigned short)(280 + i);
   for (i = 0; i < 112; ++i) lt->trans[24 + 144 + 8 + i] = (unsigned short)(144 + i);
   for (i = 0; i < 5; ++i) dt->table[i] = 0;
   dt->table[5] = 32;
   for (i = 0; i < 32; ++i) dt->trans[i] = (unsigned short)i;
}

static void tinf_build_tree(TINF_TREE *t, const unsigned char *lengths, unsigned int num)
{
   unsigned short offs[16];
   unsigned int i, sum;
   for (i = 0; i < 16; ++i) t->table[i] = 0;
   for (i = 0; i < num; ++i) t->table[lengths[i]]++;
   t->table[0] = 0;
   for (sum = 0, i = 0; i < 16; ++i) {
      offs[i] = (unsigned short)sum;
      sum += t->table[i];
   }
   for (i = 0; i < num; ++i) {
      if (lengths[i]) t->trans[offs[lengths[i]]++] = (unsigned short)i;
   }
}

static int tinf_getbit(TINF_DATA *d)
{
   unsigned int bit;
   if (!d->bitcount--) {
      d->tag = *d->source++;
      d->bitcount = 7;
   }
   bit = d->tag & 0x01;
   d->tag >>= 1;
   return bit;
}

static unsigned int tinf_read_bits(TINF_DATA *d, int num, int base)
{
   unsigned int val = 0;
   if (num) {
      unsigned int limit = 1 << (num);
      unsigned int mask;
      for (mask = 1; mask < limit; mask *= 2)
         if (tinf_getbit(d)) val += mask;
   }
   return val + base;
}

static int tinf_decode_symbol(TINF_DATA *d, TINF_TREE *t)
{
   int sum = 0, cur = 0, len = 0;
   do {
      cur = 2*cur + tinf_getbit(d);
      ++len;
      sum += t->table[len];
      cur -= t->table[len];
   } while (cur >= 0);
   return t->trans[sum + cur];
}

static void tinf_decode_trees(TINF_DATA *d, TINF_TREE *lt, TINF_TREE *dt)
{
   TINF_TREE code_tree;
   unsigned char lengths[288+32];
   unsigned int hlit, hdist, hclen;
   unsigned int i, num, length;

   hlit = tinf_read_bits(d, 5, 257);
   hdist = tinf_read_bits(d, 5, 1);
   hclen = tinf_read_bits(d, 4, 4);

   for (i = 0; i < 19; ++i) lengths[i] = 0;
   for (i = 0; i < hclen; ++i) {
      unsigned int clen = tinf_read_bits(d, 3, 0);
      lengths[clcidx[i]] = (unsigned char)clen;
   }
   tinf_build_tree(&code_tree, lengths, 19);
   for (num = 0; num < hlit + hdist; ) {
      int sym = tinf_decode_symbol(d, &code_tree);
      switch (sym) {
      case 16: {
            unsigned char prev = lengths[num - 1];
            for (length = tinf_read_bits(d, 2, 3); length; --length) lengths[num++] = prev;
         }
         break;
      case 17:
         for (length = tinf_read_bits(d, 3, 3); length; --length) lengths[num++] = 0;
         break;
      case 18:
         for (length = tinf_read_bits(d, 7, 11); length; --length) lengths[num++] = 0;
         break;
      default:
         lengths[num++] = (unsigned char)sym;
         break;
      }
   }
   tinf_build_tree(lt, lengths, hlit);
   tinf_build_tree(dt, lengths + hlit, hdist);
}

static int tinf_inflate_block_data(TINF_DATA *d, TINF_TREE *lt, TINF_TREE *dt)
{
   unsigned char *start = d->dest;
   for (;;) {
      int sym = tinf_decode_symbol(d, lt);
      if (sym == 256) {
         *d->destLen += (unsigned int)(d->dest - start);
         return TINF_OK;
      }
      if (sym < 256) {
         *d->dest++ = (unsigned char)sym;
      } else {
         int length, dist, offs;
         int i;
         sym -= 257;
         length = tinf_read_bits(d, length_bits[sym], length_base[sym]);
         dist = tinf_decode_symbol(d, dt);
         offs = tinf_read_bits(d, dist_bits[dist], dist_base[dist]);
         for (i = 0; i < length; ++i) d->dest[i] = d->dest[i - offs];
         d->dest += length;
      }
   }
}

static int tinf_inflate_uncompressed_block(TINF_DATA *d)
{
   unsigned int length, invlength;
   unsigned int i;
   length = d->source[1]; length = 256*length + d->source[0];
   invlength = d->source[3]; invlength = 256*invlength + d->source[2];
   if (length != (~invlength & 0x0000ffff)) return TINF_DATA_ERROR;
   d->source += 4;
   for (i = length; i; --i) *d->dest++ = *d->source++;
   d->bitcount = 0;
   *d->destLen += length;
   return TINF_OK;
}

static int tinf_inflate_fixed_block(TINF_DATA *d) { return tinf_inflate_block_data(d, &sltree, &sdtree); }
static int tinf_inflate_dynamic_block(TINF_DATA *d) { tinf_decode_trees(d, &d->ltree, &d->dtree); return tinf_inflate_block_data(d, &d->ltree, &d->dtree); }

void tinf_init()
{
   tinf_build_fixed_trees(&sltree, &sdtree);
   tinf_build_bits_base(length_bits, length_base, 4, 3);
   tinf_build_bits_base(dist_bits, dist_base, 2, 1);
   length_bits[28] = 0; length_base[28] = 258;
}

int tinf_uncompress(void* dest, unsigned int* destLen, const void* source)
{
   TINF_DATA d;
   int bfinal;
   d.source = (const unsigned char *)source;
   d.bitcount = 0;
   d.dest = (unsigned char *)dest;
   d.destLen = destLen;
   *destLen = 0;
   do {
      unsigned int btype;
      int res;
      bfinal = tinf_getbit(&d);
      btype = tinf_read_bits(&d, 2, 0);
      switch (btype) {
      case 0: res = tinf_inflate_uncompressed_block(&d); break;
      case 1: res = tinf_inflate_fixed_block(&d); break;
      case 2: res = tinf_inflate_dynamic_block(&d); break;
      default: return TINF_DATA_ERROR;
      }
      if (res != TINF_OK) return TINF_DATA_ERROR;
   } while (!bfinal);
   return TINF_OK;
}
