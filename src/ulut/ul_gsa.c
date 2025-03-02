/*******************************************************************
  uLan Utilities Library - C library of basic reusable constructions

  ul_gsa.c	- generic sorted arrays

  (C) Copyright 2001-2004 by Pavel Pisa - Originator

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

#undef DEBUG

#define GSA_ALLOC_STEP 8
#define GSA_DEALLOC_STEP 32

/**
 * gsa_struct_init - Initialize GSA Structure
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key_offs:	offset to the order controlling field obtained by %UL_OFFSETOF
 * @cmp_fnc:	function defining order of items by comparing fields at offset
 *		@key_offs.
 */
void 
gsa_struct_init(gsa_array_t *array, int key_offs,
		gsa_cmp_fnc_t *cmp_fnc)
{
  array->key_offs=key_offs;
  array->cmp_fnc=cmp_fnc;
  array->count=0;
  array->alloc_count=0;
  array->items=NULL;
}

/**
 * gsa_delete_all - Delete Pointers to the All Items in the Array
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 *
 * This function releases all internally allocated memory,
 * but does not release memory of the @array structure
 */
void 
gsa_delete_all(gsa_array_t *array)
{
  if(array->items) free(array->items);
  array->items=NULL;
  array->count=0;
  array->alloc_count=0;
}


/**
 * gsa_bsearch_indx - Search for Item or Place for Item by Key
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key:	key value searched for
 * @key_offs:	offset to the order controlling field obtained by %UL_OFFSETOF
 * @cmp_fnc:	function defining order of items by comparing fields
 * @mode:	mode of the search operation
 * @indx:	pointer to place, where store value of found item array index 
 *		or index where new item should be inserted
 *
 * Core search routine for GSA arrays
 * binary searches for item with field at offset @key_off equal to @key value
 * Values are compared by function pointed by *@cmp_fnc field in the array
 * structure @array.
 * Integer @mode modifies search algorithm:
 *   %GSA_FANY   .. finds item with field value *@key,
 *   %GSA_FFIRST .. finds the first item with field value *@key,
 *   %GSA_FAFTER .. index points after last item with *@key value,
 *        reworded - index points at first item with higher 
 *        value of field or after last item
 * Return Value: Return of nonzero value indicates match found.
 */
int 
gsa_bsearch_indx(gsa_array_t *array, void *key, int key_offs,
	    gsa_cmp_fnc_t *cmp_fnc, int mode, unsigned *indx)
{
  unsigned a, b, c;
  int r;
  if(!array->items || !array->count || !cmp_fnc){
    *indx=0;
    if(mode&GSA_FAFTER)
      *indx=array->count;
    return 0;
  }
  a=0;
  b=array->count;
  while(1){
    c=(a+b)/2;
    r=cmp_fnc((char*)array->items[c]+key_offs,key);
    if(!r) break;
    if(r<0)
      a=c+1;
     else
      b=c;  
    if(a==b){
      *indx=a;
      return 0;
    }
  }
  if(mode&GSA_FFIRST){
    /* equal items can be in range a to b-1 */
    /* routine looks for first one */
    b=c;
    do{
      c=(a+b)/2;
      r=cmp_fnc((char*)array->items[c]+key_offs,key);
      if(r)
	a=c+1;
       else
	b=c;  
    }while(a!=b);
    c=b;
  } else if(mode&GSA_FAFTER) {
    /* equal items can be in range a to b-1 */
    /* return index after last one */
    a=c+1;
    while(a!=b){
      c=(a+b)/2;
      r=cmp_fnc((char*)array->items[c]+key_offs,key);
      if(r)
	b=c;
       else
	a=c+1;
    }
    c=a;
  }
  *indx=c;
  return 1;
}

/**
 * gsa_find - Find Item for Provided Key
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key:	key value searched for
 *
 * Return Value: pointer to item associated to key value or %NULL.
 */
void * 
gsa_find(gsa_array_t *array, void *key)
{
  unsigned indx;
  if(gsa_bsearch_indx(array,key,array->key_offs,
  		      array->cmp_fnc,0,&indx))
    return array->items[indx];
  else return NULL;
}

/**
 * gsa_find_first - Find the First Item for Provided Key
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key:	key value searched for
 *
 * same as above, but first matching item is found.
 * Return Value: pointer to the first item associated to key value or %NULL.
 */
void * 
gsa_find_first(gsa_array_t *array, void *key)
{
  unsigned indx;
  if(gsa_bsearch_indx(array,key,array->key_offs,
  		      array->cmp_fnc,GSA_FFIRST,&indx))
    return array->items[indx];
  else return NULL;
}

/**
 * gsa_find_indx - Find the First Item with Key Value and Return Its Index 
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key:	key value searched for
 * @indx:	pointer to place for index, at which new item should be inserted
 *
 * same as above, but additionally stores item index value.
 * Return Value: pointer to the first item associated to key value or %NULL.
 */
void * 
gsa_find_indx(gsa_array_t *array, void *key, int *indx)
{
  if(gsa_bsearch_indx(array,key,array->key_offs,
  		      array->cmp_fnc,GSA_FFIRST,indx))
    return array->items[*indx];
  else return NULL;
}

/**
 * gsa_insert_at - Insert Existing Item to the Specified Array Index 
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @item:	pointer to inserted Item
 * @where:	at which index should be @item inserted
 *
 * Return Value: positive or zero value informs about success
 */
int
gsa_insert_at(gsa_array_t *array, void *item, unsigned where)
{
  unsigned acnt=array->alloc_count;
  unsigned cnt=array->count;
  void **items, **p;
  if(where>cnt) where=cnt;
  if((cnt+1>=acnt)||!array->items)
  {
    acnt+=GSA_ALLOC_STEP;
    if(!array->items)
      items=malloc(acnt*sizeof(void*));
     else
      items=realloc(array->items,acnt*sizeof(void*));
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

/**
 * gsa_insert - Insert Existing into Ordered Item Array
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @item:	pointer to inserted Item
 * @mode:	if mode is %GSA_FAFTER, multiple items with same key can
 *		be stored into array, else strict ordering is required
 *
 * Return Value: positive or zero value informs about success
 */
int
gsa_insert(gsa_array_t *array, void *item, int mode)
{
  unsigned indx;
  int res;
  res=gsa_bsearch_indx(array,(char*)item+array->key_offs,
  		array->key_offs,array->cmp_fnc,mode,&indx);
  if(res){
    if(!mode) return -1;
  }
  if(gsa_insert_at(array,item,indx)<0)
    return -1;
  return res;
}

/**
 * gsa_delete_at - Delete Item from the Specified Array Index 
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @indx:	index of deleted item
 *
 * Return Value: positive or zero value informs about success
 */
int
gsa_delete_at(gsa_array_t *array, unsigned indx)
{
  unsigned acnt=array->alloc_count;
  unsigned cnt=array->count;
  void **items=array->items;
  void **p;
  if(indx>=cnt) return -1;
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

/**
 * gsa_delete - Delete Item from the Array
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @item:	pointer to deleted Item
 *
 * Return Value: positive or zero value informs about success
 */
int
gsa_delete(gsa_array_t *array, void *item)
{
  unsigned indx;
  int key_offs=array->key_offs;
  gsa_cmp_fnc_t *cmp_fnc=array->cmp_fnc;
  if(!gsa_bsearch_indx(array,(char*)item+key_offs,
  	key_offs,cmp_fnc,GSA_FFIRST,&indx))
    return -1;

  while(array->items[indx]!=item){
    if(++indx>=array->count) return -1;
    if(cmp_fnc){
      if(cmp_fnc((char*)(array->items[indx])+key_offs,
               (char*)item+key_offs))
        return -1;
    }
  }
  return gsa_delete_at(array,indx);
}

/**
 * gsa_resort_buble - Sort Again Array If Sorting Criteria Are Changed
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key_offs:	offset to the order controlling field obtained by %UL_OFFSETOF
 * @cmp_fnc:	function defining order of items by comparing fields
 *
 * Return Value: non-zero value informs, that resorting changed order
 */
int
gsa_resort_buble(gsa_array_t *array, int key_offs,
	       gsa_cmp_fnc_t *cmp_fnc)
{
  char **a, **b, **p;
  char *k1, *k2;
  int m, m1;
  if(array->count<2) return 0;
  a=(char**)array->items; m=0;
  b=(char**)&array->items[array->count-1];
  do{
    /* upward run */
    p=a; m1=0;
    k1=*p+key_offs;
    do{
      k2=*(p+1)+key_offs;
      if(cmp_fnc(k1,k2)>0) {
        k2=*p;
        *p=*(p+1);
        *(p+1)=k2;
        m1=1;
      } else k1=k2;
    } while(++p!=b);
    m|=m1;
    if((a==--b)||!m1) return m;
    /* downward run */
    p=b; m1=0;
    k1=*p+key_offs;
    do{
      k2=*(p-1)+key_offs;
      if(cmp_fnc(k1,k2)<0) {
        k2=*p;
        *p=*(p-1);
        *(p-1)=k2;
        m1=1;
      } else k1=k2;
    } while(--p!=a);
    m|=m1;
    if((++a==b)||!m1) return m;
  }while(1);
}

/**
 * gsa_setsort - Change Array Sorting Criterion
 * @array:	pointer to the array structure declared through %GSA_ARRAY_FOR
 * @key_offs:	new value of offset to the order controlling field 
 * @cmp_fnc:	new function defining order of items by comparing fields at
 *              offset @key_offs
 *
 * Return Value: non-zero value informs, that resorting changed order
 */
int
gsa_setsort(gsa_array_t *array, int key_offs,
	       gsa_cmp_fnc_t *cmp_fnc)
{
  if(key_offs>=0) array->key_offs=key_offs;
  if(cmp_fnc!=NULL) array->cmp_fnc=cmp_fnc;
  return gsa_resort_buble(array,array->key_offs,array->cmp_fnc);
}

int gsa_cmp_int(const void *a, const void *b) UL_ATTR_REENTRANT
{
  return *(int*)a-*(int*)b;
}

int gsa_cmp_ulong(const void *a, const void *b) UL_ATTR_REENTRANT
{
  return *(unsigned long*)a-*(unsigned long*)b;
}

