#include <stdlib.h>
#include "priv/vkhel.h"

struct vkhel_ctx *vkhel_ctx_create() {
	struct vkhel_ctx *ctx = calloc(1, sizeof(struct vkhel_ctx));
	vulkan_ctx_init(&ctx->vk);
	return ctx;
}

void vkhel_ctx_destroy(struct vkhel_ctx *ctx) {
	vulkan_ctx_finish(&ctx->vk);
	free(ctx);
}
