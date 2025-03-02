/*******************************************************************
  uLan Utilities Library - C library of basic reusable constructions

  ul_gsacust.c	- generic sorted arrays

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

#include <string.h>
#include "ul_utmalloc.h"
#include "ul_gsa.h"

#define GSA_ALLOC_STEP 8
#define GSA_DEALLOC_STEP 32

void
gsa_cust_init_array_field(gsa_array_field_t *array)
{
  array->count=0;
  array->alloc_count=0;
  array->items=NULL;
}


int
gsa_cust_insert_at(gsa_array_field_t *array, void *item, unsigned where)
{
  unsigned acnt=array->alloc_count;
  unsigned cnt=array->count;
  void **items, **p;
  if(where>cnt) where=cnt;
  if((cnt+1>=acnt)||!array->items)
  {
    if(!array->items || !acnt){
      acnt=cnt+GSA_ALLOC_STEP;
      items=malloc(acnt*sizeof(void*));
      if(array->items && items)
        memcpy(items,array->items,cnt*sizeof(void*));
    }else{
      if(acnt/4>GSA_ALLOC_STEP)
	acnt+=acnt/4;
      else
	acnt+=GSA_ALLOC_STEP;
      items=realloc(array->items,acnt*sizeof(void*));
    }
    if(!items) return -1;
    array->alloc_count=acnt;
    array->items=items;
  }
  else items=array->items;
  p=items+where;
  memmove(p+1,p,(char*)(items+cnt)-(char*)p);
  array->count=cnt+1;
  *p=item;
  return 0;
}

int
gsa_cust_delete_at(gsa_array_field_t *array, unsigned indx)
{
  unsigned acnt=array->alloc_count;
  unsigned cnt=array->count;
  void **items=array->items;
  void **p;
  if(indx>=cnt) return -1;
  if(cnt && !acnt){
    p=malloc(cnt*sizeof(void*));
    if(p){
      memcpy(p,items,cnt*sizeof(void*));
      array->alloc_count=acnt=cnt;
      array->items=items=p;
    }
  }
  p=items+indx;
  array->count=--cnt;
  memmove(p,p+1,(items+cnt-p)*sizeof(void *));
  if(acnt-cnt>GSA_DEALLOC_STEP+GSA_ALLOC_STEP)
  {
    acnt-=GSA_DEALLOC_STEP;
    items=realloc(array->items,acnt*sizeof(void*));
    if(items){
      array->alloc_count=acnt;
      array->items=items;
    }
  }
  return 0;
}

void
gsa_cust_delete_all(gsa_array_field_t *array)
{
  if(array->items && array->alloc_count)
    free(array->items);
  array->items=NULL;
  array->count=0;
  array->alloc_count=0;
}
