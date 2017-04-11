/*
 * dma2d.h -- API for the DMA2D utility functions
 *
 * Copyright (c) 2016-2017, Charles McManis <cmcmanis@mcmanis.com>
 *
 * <license>
 *
 * This is the API for the DMA2D device utility functions.
 */

#pragma once
#include <stdint.h>

/*
 * Color to bits map for the DMA2D_OCOLR register
 * Important note: If you re-use a variable defined as a
 * DMA2D_COLOR type, where one use is a different 'form'
 * than the other use, previous bits will not be cleared.
 * It is unclear that the DMA2D peripheral cares about
 * stray bits in the color register that aren't applicable
 * to the current color mode but be aware.
 */
typedef union __dma2d_color {
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
		uint32_t	a:8;
	} argb8888;
	struct {
		uint32_t	b:8;
		uint32_t	g:8;
		uint32_t	r:8;
	} rgb888;
	struct {
		uint32_t	b:5;
		uint32_t	g:6;
		uint32_t	r:5;
	} rgb565;
	struct {
		uint32_t	b:5;
		uint32_t	g:5;
		uint32_t	r:5;
		uint32_t	a:1;
	} argb1555;
	struct {
		uint32_t	b:4;
		uint32_t	g:4;
		uint32_t	r:4;
		uint32_t	a:4;
	} argb4444;
	uint32_t	raw;
} DMA2D_COLOR;

/*
 * This is a 'blittable' bitmap. The important bits to
 * get right are the stride and the bits per pixel. Also
 * the type. If it is A8 or A4 you need 'color' set.
 * for L8 or L4 you need the pointer to the color map
 * set. For transparency effects make sure that the color
 * that represents transparency (typically 0) has ALPHA
 * set to 0.
 */
type struct __dma2d_bitmap  {
	void		*buf;	/* this is where the pixel data is */
	int		mode;	/* This is the bitmap 'mode' */
	int		w, h;	/* width and height */
	int		stride;	/* distance between 'lines' in the bitmap */
	DMA2D_COLOR	c;	/* Default color on A4/A8 bitmaps */
	uint32_t	*clut;	/* the color lookup table */
} DMA2D_BITMAP;

