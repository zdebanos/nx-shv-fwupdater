#ifndef C_CPCP_H
#define C_CPCP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	CCPCP_ChainPack = 1,
	CCPCP_Cpon,
} ccpcp_pack_format;

typedef enum
{
	CCPCP_RC_OK = 0,
	CCPCP_RC_MALLOC_ERROR,
	CCPCP_RC_BUFFER_OVERFLOW,
	CCPCP_RC_BUFFER_UNDERFLOW,
	CCPCP_RC_MALFORMED_INPUT,
	CCPCP_RC_LOGICAL_ERROR,
	CCPCP_RC_CONTAINER_STACK_OVERFLOW,
	CCPCP_RC_CONTAINER_STACK_UNDERFLOW,
} ccpcp_error_codes;

const char * ccpcp_error_string(int err_no);

//=========================== PACK ============================

typedef struct {
	const char *indent;
	uint8_t json_output: 1;
} ccpcp_cpon_pack_options;

struct ccpcp_pack_context;

typedef void (*ccpcp_pack_overflow_handler)(struct ccpcp_pack_context*, size_t size_hint);

typedef struct ccpcp_pack_context {
	char* start;
	char* current;
	char* end;
	int nest_count;
	int err_no; /* handlers can save error here */
	ccpcp_pack_overflow_handler handle_pack_overflow;
	void *custom_context;
	ccpcp_cpon_pack_options cpon_options;
} ccpcp_pack_context;

void ccpcp_pack_context_init(ccpcp_pack_context* pack_context, void *data, size_t length, ccpcp_pack_overflow_handler hpo);

// try to make size_hint bytes space in pack_context
// returns number of bytes available in pack_context buffer, can be < size_hint, but always > 0
// returns 0 if fails
size_t ccpcp_pack_make_space(ccpcp_pack_context* pack_context, size_t size_hint);
char *ccpcp_pack_reserve_space(ccpcp_pack_context* pack_context, size_t more);
void ccpcp_pack_copy_byte (ccpcp_pack_context* pack_context, uint8_t b);
void ccpcp_pack_copy_bytes (ccpcp_pack_context* pack_context, const void *str, size_t len);
//void ccpcp_pack_copy_bytes_cpon_string_escaped (ccpcp_pack_context* pack_context, const void *str, size_t len);

//=========================== UNPACK ============================

struct ccpcp_unpack_context;

typedef enum
{
	CCPCP_ITEM_INVALID = 0,
	CCPCP_ITEM_NULL,
	CCPCP_ITEM_BOOLEAN,
	CCPCP_ITEM_INT,
	CCPCP_ITEM_UINT,
	CCPCP_ITEM_DOUBLE,
	CCPCP_ITEM_DECIMAL,
	CCPCP_ITEM_BLOB,
	CCPCP_ITEM_STRING,
	CCPCP_ITEM_DATE_TIME,

	CCPCP_ITEM_LIST,
	CCPCP_ITEM_MAP,
	CCPCP_ITEM_IMAP,
	CCPCP_ITEM_META,
	CCPCP_ITEM_CONTAINER_END,
} ccpcp_item_types;

const char* ccpcp_item_type_to_string(ccpcp_item_types t);

#ifndef CCPCP_MAX_STRING_KEY_LEN
#define CCPCP_STRING_CHUNK_BUFF_LEN 256
#endif

typedef struct {
	long string_size;
	long size_to_load;
	char* chunk_start;
	size_t chunk_size;
	size_t chunk_buff_len;
	unsigned chunk_cnt;
	uint8_t last_chunk: 1, blob_hex: 1;
} ccpcp_string;

void ccpcp_string_init(ccpcp_string *str_it, struct ccpcp_unpack_context *unpack_context);

//#define CCPCP_INVALID_DATETIME_MIN_FROM_UTC (-64 * 15)
typedef struct {
	int64_t msecs_since_epoch;
	int minutes_from_utc;
} ccpcp_date_time;

typedef struct {
	int64_t mantisa;
	int exponent;
} ccpcp_exponentional;

double ccpcp_exponentional_to_double(const int64_t mantisa, const int exponent, const int base);
double ccpcp_decimal_to_double(const int64_t mantisa, const int exponent);
int ccpcp_decimal_to_string(char *buff, size_t buff_len, int64_t mantisa, int exponent);

typedef struct {
	union
	{
		ccpcp_string String;
		ccpcp_date_time DateTime;
		ccpcp_exponentional Decimal;
		uint64_t UInt;
		int64_t Int;
		double Double;
		bool Bool;
	} as;
	ccpcp_item_types type;
} ccpcp_item;

typedef struct {
	ccpcp_item_types container_type;
	size_t item_count;
	ccpcp_item_types current_item_type;
} ccpcp_container_state;

void ccpcp_container_state_init(ccpcp_container_state *self, ccpcp_item_types cont_type);

struct ccpcp_container_stack;

typedef int (*ccpcp_container_stack_overflow_handler)(struct ccpcp_container_stack*);

typedef struct ccpcp_container_stack {
	ccpcp_container_state *container_states;
	size_t length;
	size_t capacity;
	ccpcp_container_stack_overflow_handler overflow_handler;
} ccpcp_container_stack;

void ccpcp_container_stack_init(ccpcp_container_stack* self, ccpcp_container_state *states, size_t capacity, ccpcp_container_stack_overflow_handler hnd);

typedef size_t (*ccpcp_unpack_underflow_handler)(struct ccpcp_unpack_context*);

typedef struct ccpcp_unpack_context {
	ccpcp_item item;
	const char* start;
	const char* current;
	const char* end; /* logical end of buffer */
	int err_no; /* handlers can save error here */
	int parser_line_no; /* helps to find parser error line */
	const char *err_msg;
	ccpcp_container_stack *container_stack;
	ccpcp_unpack_underflow_handler handle_unpack_underflow;
	void *custom_context;
	char default_string_chunk_buff[CCPCP_STRING_CHUNK_BUFF_LEN];
	char *string_chunk_buff;
	size_t string_chunk_buff_len;
} ccpcp_unpack_context;

void ccpcp_unpack_context_init(ccpcp_unpack_context* self, const void* data, size_t length
							   , ccpcp_unpack_underflow_handler huu
							   , ccpcp_container_stack *stack);

ccpcp_container_state* ccpcp_unpack_context_push_container_state(ccpcp_unpack_context* self, ccpcp_item_types container_type);
ccpcp_container_state* ccpcp_unpack_context_top_container_state(ccpcp_unpack_context* self);
ccpcp_container_state* ccpcp_unpack_context_parent_container_state(ccpcp_unpack_context* self);
ccpcp_container_state* ccpcp_unpack_context_closed_container_state(ccpcp_unpack_context* self);
void ccpcp_unpack_context_pop_container_state(ccpcp_unpack_context* self);

const char *ccpcp_unpack_take_byte(ccpcp_unpack_context* unpack_context);
const char *ccpcp_unpack_peek_byte(ccpcp_unpack_context* unpack_context);
/*
bool ccpcp_item_is_string_unfinished(ccpcp_unpack_context* unpack_context);
bool ccpcp_item_is_list_item(ccpcp_unpack_context* unpack_context);
bool ccpcp_item_is_map_key(ccpcp_unpack_context* unpack_context);
bool ccpcp_item_is_map_val(ccpcp_unpack_context* unpack_context);
*/
#define UNPACK_ERROR(error_code, error_msg)                        \
{                                                       \
    unpack_context->item.type = CCPCP_ITEM_INVALID;        \
	unpack_context->err_no = error_code;           \
	unpack_context->err_msg = error_msg;           \
    return;                                             \
}

#define UNPACK_TAKE_BYTE() \
{ \
	p = ccpcp_unpack_take_byte(unpack_context);        \
	if(!p) \
		return; \
}

#define UNPACK_PEEK_BYTE() \
{ \
	p = ccpcp_unpack_peek_byte(unpack_context);        \
	if(!p) \
		return; \
}


#ifdef __cplusplus
}
#endif

#endif 
