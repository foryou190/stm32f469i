/*
 * dma2d.c -- Utility functions for DMA2D peripheral
 *
 * Copyright (c) 2016-2017 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * <license>
 *
 * These are some helper functions I've written for the
 * DMA2D device, see the dma2d.h file for the API.
 */

#include <stdint.h>
#include <stdio.h>
#include <libopencm3/stm32/dma2d.h>
#include "dma2d.h"

/*
 * dma2d_draw_4bpp -- Write nyblets into the framebuffer
 */
void
dma2d_draw_4bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	uint8_t *p = ((uint8_t *) fb->buf) + y * fb->stride + (x >> 1);
	if ((x & 1) == 0) {
		*p = (*p & 0xf0) | (pixel & 0xf);
	} else {
		*p = (*p & 0xf) | ((pixel & 0xf) << 4);
	}
}

/*
 * dma2d_draw_8bpp -- Write bytes into the framebuffer
 */
void
dma2d_draw_8bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->buf) + y * fb->stride + x) = pixel & 0xff;
}

/*
 * dma2d_draw_16bpp -- Write shorts into the framebuffer
 */
void
dma2d_draw_16bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->buf) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
}

/*
 * dma2d_draw_24bpp -- Write tri-bytes into the framebuffer
 */
void
dma2d_draw_24bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->buf) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 2) = (pixel >> 16) & 0xff;
}

/*
 * dma2d_draw_32bpp -- Write longs into the framebuffer
 */
void
dma2d_draw_32bpp(DMA2D_BITMAP *fb, int x, int y, uint32_t pixel)
{
	*((uint8_t *)(fb->buf) + y * fb->stride + x) = pixel & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 1) = (pixel >> 8) & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 2) = (pixel >> 16) & 0xff;
	*((uint8_t *)(fb->buf) + y * fb->stride + x + 3) = (pixel >> 24) & 0xff;
}

/*
 * Clear the bitmap to a single color value.
 */
void
dma2d_clear(DMA2D_BITMAP *bm, DMA2D_COLOR color)
{
    DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_R2M);
    DMA2D_OPFCCR = bm->mode;
    DMA2D_OCOLR = color.raw;
    DMA2D_OOR = 0;
    DMA2D_NLR = DMA2D_SET(NLR, PL, bm->w) | bm->h; 
    DMA2D_OMAR = (uint32_t) bm->buf;

    /* kick it off */
    DMA2D_CR |= DMA2D_CR_START;
}

/* lookup table relates Color Mode to Bits per Pixel */
static const uint8_t mode2bpp[] = {
/*   0   1   2   3  4   5  6   7  8  9 10 11 12 13 14 15 */
	32, 24, 16, 16, 16, 8, 8, 16, 4, 8, 4, 1, 1, 1, 1, 1
};

/*
 * dma2d_render( ... )
 *
 * This code uses memory - to - memory - with - PFC to copy
 * data from one DMA2D_BITMAP into another DMA2D_BITMAP. I am
 * assuming that *most* of the time the destination will be the
 * screen but I could imagine that it is some sort of complext
 * bitmap building scheme.
 * 
 * It also uses the size of src bitmap to cover the transfer
 * so you get the whole bitmap. You can do character glyphs
 * (for example) by having one bit bitmap of glyphs but creating
 * a bunch of bitmap structures that point at each glyph. (I think)
 */
void
dma2d_render(DMA2D_BITMAP *src, DMA2D_BITMAP *dst, int x, int y)
{
	int		i;
	
	/* We derive the DMA2D settings from the fields of
 	 * the bitmap structure. Note that on 'dst' index 
 	 * color modes are not supported and I'm not using
	 * DMA to load the color table. Instead doing it
	 * manually if required.
	 */
	DMA2D_CR = DMA2D_SET(CR, MODE, DMA2D_CR_MODE_M2MWB);

	/* pixel data */
	DMA2D_FGMAR = (uint32_t) src->buf;
	DMA2D_FGOR = 0; /* just add stride each time */
	DMA2D_FGCOLR = src->fg.raw;
	/* Set the mode, alpha will come from src image */
	DMA2D_FGPFCCR = src->mode;
	/* If Max Color is non-zero, load the color table */
	if (src->maxc > 0) {
		DMA2D_FGPFCCR |= (src->maxc) << DMA2D_FGPFCCR_CS_SHIFT;
		for (i = 0; i < src->maxc; i++) {
			*(DMA2D_FG_CLUT + i) = *(src->clut + i);
		}
	}

	/* pixel data */
	i = mode2bpp[dst->mode] / 8; /* bytes per pixel */

	DMA2D_BGMAR = (uint32_t) (((uint8_t *)dst->buf) + (dst->w * i * y) + (x * i));

	DMA2D_BGOR = dst->w - src->w;
	DMA2D_OMAR = (uint32_t) (((uint8_t *)dst->buf) + (dst->w * i * y) + (x * i));

	DMA2D_OOR = dst->w - src->w;
	DMA2D_BGCOLR = src->bg.raw;
	DMA2D_BGPFCCR = dst->mode;
	DMA2D_NLR = DMA2D_SET(NLR, PL, src->w) | src->h; /* all of the src */

	/* kick it off */
	DMA2D_CR |= DMA2D_CR_START;
	while ((DMA2D_CR & DMA2D_CR_START));
}
