#ifndef PRIV_VKHEL_H
#define PRIV_VKHEL_H

#include <vkhel.h>
#include "priv/vector.h"
#include "priv/vulkan.h"

#define DIV_CEIL(a, b) ((a + b - 1) / b)

struct vkhel_ctx {
	struct vulkan_ctx vk;
};

#endif
