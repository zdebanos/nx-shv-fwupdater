/*******************************************************************
  uLan Utilities Library - C library of basic reusable constructions

  ul_gsa.h	- generic sorted arrays

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

#ifndef _UL_GSA_H
#define _UL_GSA_H

#include "ul_utdefs.h"
#include "ul_itbase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSA_FANY 0
#define GSA_FFIRST 1
#define GSA_FAFTER 2

/* function to compare fields of two array items */
typedef int gsa_cmp_fnc_t(const void *a, const void *b) UL_ATTR_REENTRANT;

/* compare two integer fields */
int gsa_cmp_int(const void *a, const void *b) UL_ATTR_REENTRANT;

/* compare two unsigned long fields */
int gsa_cmp_ulong(const void *a, const void *b) UL_ATTR_REENTRANT;

/* define structure representing head of array */
#define GSA_ARRAY_FOR(_type) \
  struct { \
    _type **items; \
    unsigned count; \
    unsigned alloc_count; \
    int key_offs; \
    gsa_cmp_fnc_t *cmp_fnc; \
  }

/* array type for functions independent on stored type */
typedef GSA_ARRAY_FOR(void) gsa_array_t;

/* initialize array head structure */
void
gsa_struct_init(gsa_array_t *array, int key_offs,
		gsa_cmp_fnc_t *cmp_fnc);

/* delete all pointers from array */
void 
gsa_delete_all(gsa_array_t *array);

/* Core binary search routine for GSA arrays
   searches in "array" for index "indx" of item
   with value of item field at offset "key_offs" 
   equal to "*key". Values are compared by function
   "*cmp_fnc".
   Integer "mode" modifies search algorithm
     0 .. finds index of any item with field value "*key"
     1 .. finds index of first item with "*key"
     2 .. index points after last item with "*key" value 
          reworded - index points at first item with higher 
          value of field or after last item
   Return of nonzerro value indicates match found.
 */
int 
gsa_bsearch_indx(gsa_array_t *array, void *key, int key_offs,
	    gsa_cmp_fnc_t *cmp_fnc, int mode, unsigned *indx);

/* returns item with key field value equal to "*key" or NULL */
void * 
gsa_find(gsa_array_t *array, void *key);

/* same as above, but first matching item is found */
void * 
gsa_find_first(gsa_array_t *array, void *key);

/* find index "indx" of the first matching item */
void * 
gsa_find_indx(gsa_array_t *array, void *key, int *indx);

/* insert new item at index "where" */
int
gsa_insert_at(gsa_array_t *array, void *item, unsigned where);

/* insert new item at the right index, 
   "mode" has same meaning as in "gsa_bsearch_indx"
   if mode==0 then strict sorting is required
   and violation result in ignore of new item
   and return value <0
 */
int
gsa_insert(gsa_array_t *array, void *item, int mode);

/* delete item at index */
int
gsa_delete_at(gsa_array_t *array, unsigned indx);

/* delete item from array */
int
gsa_delete(gsa_array_t *array, void *item);

/* set new sorting field and function
   returns 0 if no change needed */
int
gsa_setsort(gsa_array_t *array, int key_offs,
	       gsa_cmp_fnc_t *cmp_fnc);

/*===========================================================*/
/* Macrodefinitions to prepare custom GSA arrays with */

/**
 * struct gsa_array_field_t - Structure Representing Anchor of custom GSA Array
 * @items:	pointer to array of pointers to individual items
 * @count:	number of items in the sorted array
 * @alloc_count: allocated pointer array capacity
 */

typedef struct gsa_array_field_t{
  void **items;
  unsigned count;
  unsigned alloc_count;
} gsa_array_field_t;

typedef struct gsa_static_array_field_t{
  void * const *items;
  unsigned count;
} gsa_static_array_field_t;

void gsa_cust_init_array_field(gsa_array_field_t *array);
int gsa_cust_insert_at(gsa_array_field_t *array, void *item, unsigned where);
int gsa_cust_delete_at(gsa_array_field_t *array, unsigned indx);
void gsa_cust_delete_all(gsa_array_field_t *array);


/* User must provide his/her own compare routine with 
    int cust_cmp_fnc(cust_key_t *a, cust_key_t *b) */

/*** Base declaration of custom GSA array  ***/
#define GSA_BASE_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
static inline cust_item_t * \
cust_prefix##_indx2item(const cust_array_t *array, unsigned indx) \
{\
  if(indx>=array->cust_array_field.count) return NULL;\
  return (cust_item_t*)array->cust_array_field.items[indx];\
}\
\
static inline cust_key_t *\
cust_prefix##_indx2key(const cust_array_t *array, unsigned indx)\
  { return &(cust_prefix##_indx2item(array, indx)->cust_item_key);}\
\
cust_scope int cust_prefix##_bsearch_indx(const cust_array_t *array, cust_key_t const *key, int mode, unsigned *indxp);\
\
static inline  cust_item_t *\
cust_prefix##_at(const cust_array_t *array, unsigned indx)\
{\
  return cust_prefix##_indx2item(array, indx);\
}\
\
static inline cust_item_t *\
cust_prefix##_find(const cust_array_t *array, cust_key_t const *key)\
{\
  unsigned indx;\
  if(!cust_prefix##_bsearch_indx(array, key, 0, &indx)) return NULL;\
  return cust_prefix##_indx2item(array, indx);\
}\
\
static inline cust_item_t *\
cust_prefix##_find_first(const cust_array_t *array, cust_key_t const *key)\
{\
  unsigned indx;\
  if(!cust_prefix##_bsearch_indx(array, key, GSA_FFIRST, &indx)) return NULL;\
  return cust_prefix##_indx2item(array, indx);\
}\
\
static inline unsigned \
cust_prefix##_find_first_indx(const cust_array_t *array, cust_key_t const *key)\
{\
  unsigned indx;\
  if(!cust_prefix##_bsearch_indx(array, key, GSA_FFIRST, &indx)) return -1;\
  return indx;\
}\
\
static inline cust_item_t *\
cust_prefix##_find_after(const cust_array_t *array, cust_key_t const *key)\
{\
  unsigned indx;\
  if(!cust_prefix##_bsearch_indx(array, key, GSA_FAFTER, &indx)) return NULL;\
  return cust_prefix##_indx2item(array, indx);\
}\
\
static inline unsigned \
cust_prefix##_find_after_indx(const cust_array_t *array, cust_key_t const *key)\
{\
  unsigned indx;\
  cust_prefix##_bsearch_indx(array, key, GSA_FAFTER, &indx);\
  return indx;\
}\
\
static inline int \
cust_prefix##_is_empty(const cust_array_t *array)\
{\
  return !array->cust_array_field.count;\
}\
\
static inline cust_item_t *\
cust_prefix##_first(const cust_array_t *array)\
{\
  return cust_prefix##_indx2item(array, 0);\
}\
\
static inline unsigned \
cust_prefix##_first_indx(const cust_array_t *array)\
{\
  (void)array;\
  return 0;\
}\
\
static inline cust_item_t *\
cust_prefix##_last(const cust_array_t *array)\
{\
  return cust_prefix##_indx2item(array, array->cust_array_field.count-1);\
}\
\
static inline unsigned \
cust_prefix##_last_indx(const cust_array_t *array)\
{\
  return array->cust_array_field.count-1;\
}

/*** Iterators for GSA arrays ***/
#define GSA_IT_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t) \
\
typedef struct {\
  cust_array_t *container;\
  unsigned indx;\
} cust_prefix##_it_t;\
\
static inline cust_item_t *\
cust_prefix##_it2item(const cust_prefix##_it_t *it)\
{\
  return cust_prefix##_indx2item(it->container,it->indx);\
}\
\
static inline void \
cust_prefix##_first_it(cust_array_t *container, cust_prefix##_it_t *it)\
{\
  it->container=container;\
  it->indx=cust_prefix##_first_indx(container);\
}\
\
static inline void \
cust_prefix##_last_it(cust_array_t *container, cust_prefix##_it_t *it)\
{\
  it->container=container;\
  it->indx=cust_prefix##_last_indx(container);\
}\
\
static inline void \
cust_prefix##_next_it(cust_prefix##_it_t *it)\
{\
  if(it->indx<=cust_prefix##_last_indx(it->container)) it->indx++;\
  else it->indx=cust_prefix##_first_indx(it->container);\
}\
\
static inline void \
cust_prefix##_prev_it(cust_prefix##_it_t *it)\
{\
  if(it->indx<=cust_prefix##_last_indx(it->container)) it->indx--;\
  else it->indx=cust_prefix##_first_indx(it->container);\
}\
\
static inline int \
cust_prefix##_is_end_it(cust_prefix##_it_t *it)\
{\
  if(!it->container) return 1;\
  return it->indx>cust_prefix##_last_indx(it->container);\
}\
\
static inline int \
cust_prefix##_find_it(cust_array_t *container, cust_key_t const *key, cust_prefix##_it_t *it)\
{\
  it->container=container;\
  return (it->indx=cust_prefix##_find_first_indx(container, key))!=(unsigned)-1;\
}\
\
static inline int \
cust_prefix##_find_first_it(cust_array_t *container, cust_key_t const *key, cust_prefix##_it_t *it)\
{\
  it->container=container;\
  return (it->indx=cust_prefix##_find_first_indx(container, key))!=(unsigned)-1;\
}\
\
static inline int \
cust_prefix##_find_after_it(cust_array_t *container, cust_key_t const *key, cust_prefix##_it_t *it)\
{\
  it->container=container;\
  return (it->indx=cust_prefix##_find_after_indx(container, key))!=(unsigned)-1;\
}

/* Declaration of new const custom array without support of runtime modifications */
#define GSA_CONST_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
GSA_BASE_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
GSA_IT_CUST_DEC(cust_prefix, const cust_array_t, cust_item_t, cust_key_t)

#define GSA_CONST_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
	GSA_CONST_CUST_DEC_SCOPE(extern, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc)

/*** Declaration of dynamic custom array with full functions ***/
#define GSA_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
GSA_BASE_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
cust_scope int cust_prefix##_insert(cust_array_t *array, cust_item_t *item);\
cust_scope int cust_prefix##_delete(cust_array_t *array, const cust_item_t *item);\
\
static inline void \
cust_prefix##_init_array_field(cust_array_t *array)\
{\
  gsa_cust_init_array_field(&array->cust_array_field);\
}\
\
static inline int \
cust_prefix##_insert_at(cust_array_t *array, cust_item_t *item, unsigned indx)\
{\
  return gsa_cust_insert_at(&array->cust_array_field, item, indx);\
}\
\
static inline int \
cust_prefix##_delete_at(cust_array_t *array, unsigned indx)\
{\
  return gsa_cust_delete_at(&array->cust_array_field, indx);\
}\
\
static inline void \
cust_prefix##_delete_all(cust_array_t *array)\
{\
  gsa_cust_delete_all(&array->cust_array_field);\
}\
\
static inline cust_item_t *\
cust_prefix##_cut_last(cust_array_t *array)\
{\
  if(cust_prefix##_is_empty(array)) return NULL;\
  return (cust_item_t *)array->cust_array_field.items\
                          [--array->cust_array_field.count];\
}\
/*** Iterators ***/\
GSA_IT_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t) \
\
static inline void \
cust_prefix##_delete_it(cust_prefix##_it_t *it)\
{\
  cust_prefix##_delete_at(it->container,it->indx);\
}

#define GSA_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
	GSA_CUST_DEC_SCOPE(extern, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \

/*** Declaration of static custom array with limited functions ***/
#define GSA_STATIC_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
GSA_BASE_CUST_DEC_SCOPE(cust_scope, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
\
cust_scope int cust_prefix##_insert(cust_array_t *array, cust_item_t *item);\
cust_scope int cust_prefix##_delete(cust_array_t *array, const cust_item_t *item);\
cust_scope void cust_prefix##_init_array_field(cust_array_t *array);\
cust_scope int cust_prefix##_insert_at(cust_array_t *array, cust_item_t *item, unsigned indx);\
cust_scope int cust_prefix##_delete_at(cust_array_t *array, unsigned indx); \
\
static inline void \
cust_prefix##_delete_all(cust_array_t *array)\
{\
  array->cust_array_field.count=0;\
}\
\
static inline cust_item_t *\
cust_prefix##_cut_last(cust_array_t *array)\
{\
  if(cust_prefix##_is_empty(array)) return NULL;\
  return (cust_item_t *)array->cust_array_field.items\
                          [--array->cust_array_field.count];\
}\
/*** Iterators ***/\
GSA_IT_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t) \
\
static inline void \
cust_prefix##_delete_it(cust_prefix##_it_t *it)\
{\
  cust_prefix##_delete_at(it->container,it->indx);\
}

#define GSA_STATIC_CUST_DEC(cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc) \
	GSA_STATIC_CUST_DEC_SCOPE(extern, cust_prefix, cust_array_t, cust_item_t, cust_key_t,\
		cust_array_field, cust_item_key, cust_cmp_fnc)

/* The next implementation of foreaach is elegant, but can not
   be used in C99 non-conformant C compiler */
#ifdef WITH_C99

#define gsa_cust_for_each(cust_prefix, array, ptr) \
	for(unsigned __fe_indx=cust_prefix##_first_indx(array);\
	    (ptr=cust_prefix##_indx2item(array,__fe_indx));\
	    __fe_indx++)

#define gsa_cust_for_each_rev(cust_prefix, array, ptr) \
	for(unsigned __fe_indx=cust_prefix##_last_indx(array);\
	    (ptr=cust_prefix##_indx2item(array,__fe_indx));\
	    __fe_indx--)

#define gsa_cust_for_each_from(cust_prefix, array, key, ptr) \
	for(unsigned __fe_indx=cust_prefix##_find_first_indx(array, key); \
	    (ptr=cust_prefix##_indx2item(array,__fe_indx)); \
	    __fe_indx++)

#define gsa_cust_for_each_after(cust_prefix, array, key, ptr) \
	for(unsigned __fe_indx=cust_prefix##_find_after_indx(array, key); \
	    (ptr=cust_prefix##_indx2item(array,__fe_indx)); \
	    __fe_indx++)

#endif /*WITH_C99*/

#define gsa_cust_for_each_cut(cust_prefix, array, ptr) \
	for(;(ptr=cust_prefix##_cut_last(array));)


#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /* _UL_GSA_H */
