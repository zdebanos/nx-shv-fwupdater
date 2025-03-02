/*******************************************************************
  uLan Utilities Library - C library of basic reusable constructions

  ul_gsacust.h	- generic sorted arrays

  (C) Copyright 2001 by Pavel Pisa - Originator

  The uLan utilities library can be used, copied and modified under
  next licenses
    - GPL - GNU General Public License
    - LGPL - GNU Lesser General Public License
    - MPL - Mozilla Public License
    - and other licenses added by project originators
  Code can be modified and re-distributed under any combination
  of the above listed licenses. If contributor does not agree with
  some of the licenses, he/she can delete appropriate line.
  Warning, if you delete all lines, you are not allowed to
  distribute source code and/or binaries utilizing code.
  
  See files COPYING and README for details.

 *******************************************************************/

#ifndef _UL_GSACUST_H
#define _UL_GSACUST_H

#include "ul_gsa.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constant version of custom GSA arrays. It does not support runtime modifications. */
#define GSA_CONST_CUST_IMP(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc, cust_ins_fl) \
\
int \
cust_prefix##_bsearch_indx(const cust_array_t *array, cust_key_t const *key, \
	                   int mode, unsigned *indx) \
{\
  unsigned a, b, c;\
  int r;\
  if(!array->cust_array_field.items || !array->cust_array_field.count){\
    *indx=0;\
    return 0;\
  }\
  a=0;\
  b=array->cust_array_field.count;\
  while(1){\
    c=(a+b)/2;\
    r=cust_cmp_fnc(cust_prefix##_indx2key(array, c), key);\
    if(!r) if(!(mode&GSA_FAFTER)) break;\
    if(r<=0)\
      a=c+1;\
     else\
      b=c;\
    if(a==b){\
      *indx=a;\
      return mode&GSA_FAFTER;\
    }\
  }\
  if(mode&GSA_FFIRST){\
    /* equal items can be in range a to b-1 */\
    /* routine looks for first one */\
    b=c;\
    do{\
      c=(a+b)/2;\
      r=cust_cmp_fnc(cust_prefix##_indx2key(array, c), key);\
      if(r)\
	a=c+1;\
       else\
	b=c;\
    }while(a!=b);\
    c=b;\
  }\
  *indx=c;\
  return 1;\
}


/* Dynamic version with full support of insert and delete functions. */
#define GSA_CUST_IMP(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc, cust_ins_fl) \
\
GSA_CONST_CUST_IMP(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
	cust_array_field, cust_item_key, cust_cmp_fnc, cust_ins_fl) \
\
int \
cust_prefix##_insert(cust_array_t *array, cust_item_t *item)\
{\
  unsigned indx;\
  if(cust_prefix##_bsearch_indx(array, &item->cust_item_key, cust_ins_fl, &indx))\
    if(!cust_ins_fl) return -1;\
  return cust_prefix##_insert_at(array,item,indx);\
}\
\
int \
cust_prefix##_delete(cust_array_t *array, const cust_item_t *item)\
{\
  unsigned indx;\
  if(!cust_prefix##_bsearch_indx(array, &item->cust_item_key, GSA_FFIRST,&indx))\
    return -1;\
  while(cust_prefix##_indx2item(array, indx)!=item){\
    if(++indx>=array->cust_array_field.count) return -1;\
    if(cust_cmp_fnc(cust_prefix##_indx2key(array, indx),\
                    &item->cust_item_key))  return -1;\
  }\
  return cust_prefix##_delete_at(array,indx);\
}


/* Static version with limited support of insert and delete functions. */
#define GSA_STATIC_CUST_IMP(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc, cust_ins_fl) \
\
GSA_CUST_IMP(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
	cust_array_field, cust_item_key, cust_cmp_fnc, cust_ins_fl) \
\
void \
cust_prefix##_init_array_field(cust_array_t *array)\
{\
  array->cust_array_field.count=0;\
  array->cust_array_field.alloc_count=0;\
  array->cust_array_field.items=NULL;\
}\
\
int \
cust_prefix##_insert_at(cust_array_t *array, cust_item_t *item, unsigned indx)\
{\
  unsigned cnt=array->cust_array_field.count; \
  cust_item_t **p; \
\
  if(indx>cnt) indx=cnt;\
  if(cnt+1>=array->cust_array_field.alloc_count)\
    return -1;\
\
  p=array->cust_array_field.items+indx;\
  memmove(p+1,p,(char*)(array->cust_array_field.items+cnt)-(char*)p);\
  array->cust_array_field.count=cnt+1;\
  *p=item;\
  return 0;\
}\
\
int \
cust_prefix##_delete_at(cust_array_t *array, unsigned indx)\
{\
  unsigned cnt=array->cust_array_field.count;\
  cust_item_t **p;\
  if(indx>=cnt) return -1;\
  p=array->cust_array_field.items+indx;\
  array->cust_array_field.count=--cnt;\
  memmove(p,p+1,(array->cust_array_field.items+cnt-p)*sizeof(void *));\
  return 0;\
}\


#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /* _UL_GSACUST_H */
