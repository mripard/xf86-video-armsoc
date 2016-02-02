/*
 * Copyright © 2013 ARM Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <xf86drm.h>

#include "../drmmode_driver.h"

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define ALIGN(x, a)			__ALIGN_KERNEL((x), (a))

/* This should be included from uapi headers once the driver is
 * mainlined
 */
struct drm_sun4i_gem_create {
	uint64_t	size;
	uint32_t	flags;
	uint32_t	handle;
};

#define DRM_SUN4I_GEM_CREATE            0x00

#define DRM_IOCTL_SUN4I_GEM_CREATE      DRM_IOWR(DRM_COMMAND_BASE + DRM_SUN4I_GEM_CREATE, \
						 struct drm_sun4i_gem_create)

/* Cursor dimensions
 * Technically we probably don't have any size limit.. since we
 * are just using an overlay... but xserver will always create
 * cursor images in the max size, so don't use width/height values
 * that are too big
 */
/* width */
#define CURSORW   (64)
/* height */
#define CURSORH   (64)
/* Padding added down each side of cursor image */
#define CURSORPAD (0)

static int create_custom_gem(int fd, struct armsoc_create_gem *create_gem)
{
	struct drm_sun4i_gem_create create_sun4i;
	int ret;
	unsigned int pitch;

	assert((create_gem->buf_type == ARMSOC_BO_SCANOUT) ||
	       (create_gem->buf_type == ARMSOC_BO_NON_SCANOUT));

	/* make pitch a multiple of 64 bytes for best performance */
	pitch = DIV_ROUND_UP(create_gem->width * create_gem->bpp, 8);
	pitch = ALIGN(pitch, 64);

	memset(&create_sun4i, 0, sizeof(create_sun4i));
	create_sun4i.size = create_gem->height * pitch;

	ret = drmIoctl(fd, DRM_IOCTL_SUN4I_GEM_CREATE, &create_sun4i);
	if (ret)
		return ret;

	/* Convert custom create_sun4i to generic create_gem */
	create_gem->handle = create_sun4i.handle;
	create_gem->pitch = pitch;
	create_gem->size = create_sun4i.size;

	return 0;
}

struct drmmode_interface sun4i_interface = {
	"sun4i-drm"		/* name of drm driver*/,
	1			/* use_page_flip_events */,
	1			/* use_early_display */,
	CURSORW			/* cursor width */,
	CURSORH			/* cursor_height */,
	CURSORPAD		/* cursor padding */,
	HWCURSOR_API_PLANE	/* cursor_api */,
	NULL			/* init_plane_for_cursor */,
	0			/* vblank_query_supported */,
	create_custom_gem	/* create_custom_gem */,
};
