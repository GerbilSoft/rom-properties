/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * microtar_zstd.c: zstd reader for MicroTAR                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "microtar_zstd.h"

#include <zstd.h>

// C includes
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline size_t min_sz(size_t a, size_t b)
{
	return (a < b) ? a : b;
}

// Streaming context
typedef struct _mzstd_ctx {
	ZSTD_DCtx *dctx;
	FILE *f;

	// Data buffers
	uint8_t *buf_in;
	uint8_t *buf_out;

	size_t buf_in_size;
	size_t buf_out_size;

	// Buffer status
	// NOTE: output.pos is how much data was read into the buffer,
	// not our current output read position. For that, use output_ptr.
	ZSTD_inBuffer input;
	ZSTD_outBuffer output;
	size_t output_ptr;

	// Current decompressed position for arbitrary seek.
	size_t unz_pos;
} mzstd_ctx;

/**
 * MicroTAR read() callback
 * @param tar	[in] mtar_t
 * @param data	[out] Data buffer
 * @param size	[in] Size to read
 * @return MTAR_ESUCCESS on success; other on error.
 */
static int mtar_zstd_read(mtar_t *tar, void *data, unsigned size)
{
	// Decompress data until we read `size` bytes.
	// Reference: zstd/examples/streaming_decompression.c
	mzstd_ctx *const ctx = (mzstd_ctx*)tar->stream;
	if (!ctx)
		return MTAR_EFAILURE;

	uint8_t *data8 = (uint8_t*)data;
	while (size > 0) {
		// Check if we have any data remaining in the output buffer.
		// NOTE: ctx->output.pos indicates how much is left in the buffer.
		if (ctx->output_ptr < ctx->output.pos) {
			// We have data remaining. Determine how much to use.
			size_t toCopy = min_sz(ctx->output.pos - ctx->output_ptr, size);
			memcpy(data8, &ctx->buf_out[ctx->output_ptr], toCopy);

			data8 += toCopy;
			size -= toCopy;
			ctx->output_ptr += toCopy;
			ctx->unz_pos += toCopy;
			if (size == 0)
				break;
		}

		// Check if we need to read more data from the file.
		if (ctx->input.pos >= ctx->input.size) {
			size_t ret = fread(ctx->buf_in, 1, ctx->buf_in_size, ctx->f);
			if (ret == 0) {
				// No more data...
				return MTAR_EREADFAIL;
			}

			ctx->input.size = ret;
			ctx->input.pos = 0;
		}

		// Decompress the input buffer.
		ctx->output.size = ctx->buf_out_size;
		ctx->output.pos = 0;
		ctx->output_ptr = 0;
		size_t ret = ZSTD_decompressStream(ctx->dctx, &ctx->output, &ctx->input);
		if (ZSTD_isError(ret)) {
			// Decompression error!
			ctx->output.size = 0;
			ctx->output.pos = 0;
			ctx->output_ptr = 0;
			return MTAR_EREADFAIL;
		}

		// Output buffer copying will be done in the next loop iteration.
	}
	
	return MTAR_ESUCCESS;
}

/**
 * MicroTAR seek() callback
 * @param tar	[in] mtar_t
 * @param pos	[in] Seek position
 * @return MTAR_ESUCCESS on success; other on error.
 */
static int mtar_zstd_seek(mtar_t *tar, unsigned pos)
{
	mzstd_ctx *const ctx = (mzstd_ctx*)tar->stream;
	if (!ctx)
		return MTAR_EFAILURE;

	if (pos == ctx->unz_pos) {
		// Useless seek to the same position...
		return MTAR_ESUCCESS;
	}

	if (pos < ctx->unz_pos) {
		// NOTE: MicroTAR seeks backwards after reading the .tar header.
		// If we can seek backwards in the input buffer, do that so we
		// don't have to re-decompress everything.
		// NOTE 2: There are definitely some edge cases that this won't handle,
		// but it fixes *most* of them, resulting in the .tar.zst tests running
		// approximately 2.8 times slower than plain .tar files.

		// Quick test in debug builds: (MegaDrive only)
		// - .tar: 1,093 ms
		// - .tar.zst: 2,805 ms
		if (pos > 0) {
			size_t bytes_to_reverse = ctx->unz_pos - pos;
			if (ctx->output.pos != 0 && ctx->output_ptr <= ctx->output.pos) {
				if (bytes_to_reverse <= ctx->output_ptr) {
					ctx->output_ptr -= bytes_to_reverse;
					ctx->unz_pos -= bytes_to_reverse;
					return MTAR_ESUCCESS;
				}
			}
		}

		// Rewind the file and reset ZSTD decompression.
		rewind(ctx->f);
		ctx->input.size = 0;
		ctx->input.pos = 0;
		ctx->output.size = 0;
		ctx->output.pos = 0;
		ctx->output_ptr = 0;
		ctx->unz_pos = 0;
		ZSTD_DCtx_reset(ctx->dctx, ZSTD_reset_session_and_parameters);
	}

	if (pos == 0) {
		// We're at the start of the file. Nothing else to do here.
		return MTAR_ESUCCESS;
	}

	// Seek to the requested position.
	uint8_t seek_buf[512];
	size_t seek_diff = pos - ctx->unz_pos;
	while (seek_diff > 0) {
		// Read up to seek_buf bytes.
		size_t toCopy = min_sz(sizeof(seek_buf), seek_diff);
		int ret = mtar_zstd_read(tar, seek_buf, toCopy);
		if (ret != MTAR_ESUCCESS)
			return ret;

		seek_diff -= toCopy;
	}

	return MTAR_ESUCCESS;
}

/**
 * MicroTAR close() callback
 * @param tar	[in] mtar_t
 * @return MTAR_ESUCCESS on success; other on error.
 */
static int mtar_zstd_close(mtar_t *tar)
{
	mzstd_ctx *const ctx = (mzstd_ctx*)tar->stream;
	if (!ctx)
		return MTAR_EFAILURE;

	if (ctx->f)
		fclose(ctx->f);
	if (ctx->dctx)
		ZSTD_freeDCtx(ctx->dctx);

	free(ctx->buf_in);
	free(ctx->buf_out);
	free(ctx);
	return MTAR_ESUCCESS;
}

/**
 * Open a .tar.zst file using zstd and MicroTAR. (read-only access)
 * @param tar		[out] mtar_t
 * @param filename	[in] Filename
 * @return MTAR_ESUCCESS on success; other on error.
 */
int mtar_zstd_open_ro(mtar_t *tar, const char *filename)
{
	// Allocate buffers.
	mzstd_ctx *const ctx = calloc(1, sizeof(mzstd_ctx));
	if (!ctx)
		return MTAR_EFAILURE;

	ctx->f = fopen(filename, "rb");
	if (!ctx->f) {
		free(ctx);
		return MTAR_EOPENFAIL;
	}

	ctx->dctx = ZSTD_createDCtx();
	if (!ctx->dctx) {
		fclose(ctx->f);
		free(ctx);
		return MTAR_EOPENFAIL;
	}

	ctx->buf_in_size = ZSTD_DStreamInSize();
	ctx->buf_out_size = ZSTD_DStreamOutSize();

	ctx->buf_in = malloc(ctx->buf_in_size);
	ctx->buf_out = malloc(ctx->buf_out_size);
	if (!ctx->buf_in || !ctx->buf_out) {
		free(ctx->buf_in);
		free(ctx->buf_out);
		ZSTD_freeDCtx(ctx->dctx);
		fclose(ctx->f);
		free(ctx);
		return MTAR_EOPENFAIL;
	}

	ctx->input.src = ctx->buf_in;
	ctx->output.dst = ctx->buf_out;

	memset(tar, 0, sizeof(*tar));
	tar->read = mtar_zstd_read;
	tar->seek = mtar_zstd_seek;
	tar->close = mtar_zstd_close;
	tar->stream = ctx;
	return MTAR_ESUCCESS;
}
