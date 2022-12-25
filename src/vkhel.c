#include <stdlib.h>
#include <vkhel/vkhel.h>
#include "vkhel.h"

struct vkhel_ctx *vkhel_ctx_create() {
	struct vkhel_ctx *ctx = calloc(1, sizeof(struct vkhel_ctx));
	return ctx;
}

void vkhel_ctx_destroy(struct vkhel_ctx *ctx) {
	free(ctx);
}
