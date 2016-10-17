/*
 * Copyright (c) 2016 Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
#ifndef __DRM_FOURCC_H__
#define __DRM_FOURCC_H__

#include <linux/types.h>
#include <uapi/drm/drm_fourcc.h>

/**
 * struct drm_format_info - information about a DRM format
 * @format: 4CC format identifier (DRM_FORMAT_*)
 * @depth: Color depth (number of bits per pixel excluding padding bits),
 *	valid for a subset of RGB formats only. This is a legacy field, do not
 *	use in new code and set to 0 for new formats.
 * @num_planes: Number of color planes (1 to 3)
 * @cpp: Number of bytes per pixel (per plane)
 * @hsub: Horizontal chroma subsampling factor
 * @vsub: Vertical chroma subsampling factor
 */
struct drm_format_info {
	u32 format;
	u8 depth;
	u8 num_planes;
	u8 cpp[3];
	u8 hsub;
	u8 vsub;
};

const struct drm_format_info *drm_format_info(u32 format);
uint32_t drm_mode_legacy_fb_format(uint32_t bpp, uint32_t depth);
void drm_fb_get_bpp_depth(uint32_t format, unsigned int *depth, int *bpp);
int drm_format_num_planes(uint32_t format);
int drm_format_plane_cpp(uint32_t format, int plane);
int drm_format_horz_chroma_subsampling(uint32_t format);
int drm_format_vert_chroma_subsampling(uint32_t format);
int drm_format_plane_width(int width, uint32_t format, int plane);
int drm_format_plane_height(int height, uint32_t format, int plane);
char *drm_get_format_name(uint32_t format) __malloc;

#endif /* __DRM_FOURCC_H__ */
