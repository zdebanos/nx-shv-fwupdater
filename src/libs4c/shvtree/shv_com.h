#ifndef SHV_FUNCTIONS_H
#define SHV_FUNCTIONS_H

#include "cchainpack.h"

#define SHV_BUF_LEN  1024
#define SHV_MET_LEN  16
#define SHV_PATH_LEN 64


#define TAG_ERROR       8
#define TAG_REQUEST_ID  8
#define TAG_SHV_PATH    9
#define TAG_METHOD      10
#define TAG_CALLER_IDS  11

struct shv_node;

typedef struct shv_con_ctx {
  int stream_fd;
  int timeout;
  int rid;
  int cid_cnt;
  int cid_capacity;
  int *cid_ptr;
  struct ccpcp_pack_context pack_ctx;
  struct ccpcp_unpack_context unpack_ctx;
  char shv_data[SHV_BUF_LEN];
  char shv_rd_data[SHV_BUF_LEN];
  int shv_len;
  int shv_send;
  struct shv_node *root;
} shv_con_ctx_t;

typedef struct shv_str_list_it_t shv_str_list_it_t;

struct shv_str_list_it_t {
   const char * (*get_next_entry)(shv_str_list_it_t *it, int reset_to_first);
};

shv_con_ctx_t *shv_com_init(struct shv_node *root);
void shv_com_end(shv_con_ctx_t *ctx);
void shv_send_int(shv_con_ctx_t *shv_ctx, int rid, int num);
void shv_send_double(shv_con_ctx_t *shv_ctx, int rid, double num);
void shv_send_str(shv_con_ctx_t *shv_ctx, int rid, const char *str);
void shv_send_str_list(shv_con_ctx_t *shv_ctx, int rid, int num_str, const char **str);
void shv_send_str_list_it(shv_con_ctx_t *shv_ctx, int rid, int num_str, shv_str_list_it_t *str_it);
void shv_send_error(shv_con_ctx_t *shv_ctx, int rid, const char *msg);
void shv_send_ping(shv_con_ctx_t *shv_ctx);

void shv_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint);
void shv_pack_head_reply(shv_con_ctx_t *shv_ctx, int rid);

int shv_unpack_data(ccpcp_unpack_context * ctx, int * v, double * d);

#endif /* SHV_FUNCTIONS_H */
