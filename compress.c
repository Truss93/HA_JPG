#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dctquant.h"
#include "bitmap.h"

//Output path can be NULL to suppress the dumping of the grayscale bitmap.
static bitmap_pixel_hsv_t* create_grayscale_bitmap(const char* input_path, const char* output_path, int* blocks_x, int* blocks_y)
{
	//Read the input bitmap:
	bitmap_pixel_hsv_t* pixels;
	int width_px, height_px;

	bitmap_error_t error = bitmapReadPixels(input_path, (bitmap_pixel_t**)&pixels, &width_px, &height_px, BITMAP_COLOR_SPACE_HSV);

	if (error != BITMAP_ERROR_SUCCESS)
	{
		printf("Failed to read bitmap, does it exist?\n");
		return NULL;
	}

	//Make sure the bitmap has a multiple of 8 pixels in both dimensions:
	if (((width_px % 8) != 0) || ((height_px % 8) != 0))
	{
		printf("Width and height must be a multiple of 8 pixels!\n");
		free(pixels);

		return NULL;
	}

	//Assign the block size:
	*blocks_x = width_px / 8;
	*blocks_y = height_px / 8;

	//Convert the bitmap to grayscale:
	for (int i = 0; i < (width_px * height_px); i++)
	{
		pixels[i].s = 0;
	}

	//Dump the bitmap:
	bitmap_parameters_t params =
	{
		.bottomUp = BITMAP_BOOL_TRUE,
		.widthPx = width_px,
		.heightPx = height_px,
		.colorDepth = BITMAP_COLOR_DEPTH_24,
		.compression = BITMAP_COMPRESSION_NONE,
		.dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
		.colorSpace = BITMAP_COLOR_SPACE_HSV,
	};

	error = bitmapWritePixels(output_path, BITMAP_BOOL_TRUE, &params, (bitmap_pixel_t*)pixels);

	if (error != BITMAP_ERROR_SUCCESS)
	{
		printf("Failed to write bitmap!\n");
		free(pixels);

		return NULL;
	}

	return pixels;
}

//Read the block ("index_x", "index_y") from the given pixel buffer and copy it to "block".
//The dimensions of the image (in blocks) is given by "blocks_x" x "blocks_y".
static void read_block(const bitmap_pixel_hsv_t* pixels, int index_x, int index_y, int blocks_x, int blocks_y, float* block)
{
	int bytes_per_row = blocks_x * 8;
	int base_offset = index_y * 8 * bytes_per_row;
	int col_offset = index_x * 8;

	for (int curr_y = 0; curr_y < 8; curr_y++)
	{
		int row_offset = curr_y * bytes_per_row;

		for (int curr_x = 0; curr_x < 8; curr_x++)
		{
			block[(8 * curr_y) + curr_x] = pixels[base_offset + row_offset + col_offset + curr_x].v - 128.0f;
		}
	}
}

static float dct_sum(const float* block, int p, int q)
{
	float sum = 0;

	for (int m = 0; m < 8; m++)
	{
		for (int n = 0; n < 8; n++)
		{
			sum += *block++ * cosf((M_PI * (m + 0.5f) * p) / 8.0f) * cosf((M_PI * (n + 0.5f) * q) / 8.0f);
		}
	}

	return sum;
}

//Perform the DCT on the given block of input data.
static void perform_dct(const float* input_block, float* output_block)
{
	for (int p = 0; p < 8; p++)
	{
		for (int q = 0; q < 8; q++)
		{
			*output_block++ = alpha(p) * alpha(q) * dct_sum(input_block, p, q);
		}
	}
}

//Divide by the components of the quantization matrix and round.
static void quantize(float* input_block, const int* quant_matrix, int8_t* output_block)
{
	for (int i = 0; i < 64; i++)
	{
		output_block[i] = (int8_t)roundf(input_block[i] / quant_matrix[i]);
	}
}

//Perform zig-zag encoding.
static void zig_zag(int8_t* input_block, int8_t* output_block)
{
	for (int i = 0; i < 64; i++)
	{
		output_block[zig_zag_index_matrix[i]] = input_block[i];
	}
}

int compress(const char* file_path, const int* quant_matrix, const char* grayscale_path, const char* output_path)
{
	//Load the bitmap in grayscale:
	int blocks_x, blocks_y;
	bitmap_pixel_hsv_t* pixels = create_grayscale_bitmap(file_path, grayscale_path, &blocks_x, &blocks_y);

	if (!pixels)
	{
		return -1;
	}

	//Open the output file (.dct):
	FILE* file = fopen(output_path, "wb");

	if (!file)
	{
		printf("Failed to create output file!\n");
		return -1;
	}

	//FIXME: Honor endianness!
	if (fwrite(&blocks_x, sizeof(blocks_x), 1, file) != 1)
	{
		printf("Failed to write number of blocks in x direction!\n”");
		fclose(file);

		return -1;
	}

	if (fwrite(&blocks_y, sizeof(blocks_y), 1, file) != 1)
	{
		printf("Failed to write number of blocks in y direction!\n”");
		fclose(file);

		return -1;
	}

	for (int index_y = 0; index_y < blocks_y; index_y++)
	{
		for (int index_x = 0; index_x < blocks_x; index_x++)
		{
			float input_block[64];
			float dct_block[64];
			int8_t quantized_block[64];
			int8_t zig_zagged_block[64];

			//Read the next block:
			read_block(pixels, index_x, index_y, blocks_x, blocks_y, input_block);

			//Execute the actual DCT:
			perform_dct(input_block, dct_block);

			//Quantize:
			quantize(dct_block, quant_matrix, quantized_block);

			//ZigZag:
			zig_zag(quantized_block, zig_zagged_block);

			//Write the zig-zagged block into the output file (.dct):
			if (fwrite(zig_zagged_block, sizeof(zig_zagged_block), 1, file) != 1)
			{
				printf("Failed to write block!\n");
				fclose(file);

				return -1;
			}
		}
	}

	//Close the output file:
	fclose(file);

	//Free the pixels:
	free(pixels);

	//Everything is fine :)
	return 0;
}
