#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dctquant.h"
#include "bitmap.h"

//Perform zig-zag decoding.
static void un_zig_zag(const int8_t* input_block, int8_t* output_block)
{
	for (int i = 0; i < 64; i++)
	{
		output_block[i] = input_block[zig_zag_index_matrix[i]];
	}
}


//Multiply with the components of the quantization matrix.
static void dequantize(const int8_t* input_block, const int* quant_matrix, float* output_block)
{
	for (int i = 0; i < 64; i++)
	{
		output_block[i] = input_block[i] * quant_matrix[i];
	}
}

static float dct_sum(const float* block, int m, int n)
{
	float sum = 0;

	for (int p = 0; p < 8; p++)
	{
		for (int q = 0; q < 8; q++)
		{
			sum +=  *block++ * alpha(p) * alpha(q) * cosf((M_PI * (m + 0.5f) * p) / 8.0f) * cosf((M_PI * (n + 0.5f) * q) / 8.0f);
		}
	}

	return sum;
}

//Perform the IDCT on the given block of input data.
static void perform_inverse_dct(const float* input_block, float* output_block)
{
	for (int m = 0; m < 8; m++)
	{
		for (int n = 0; n < 8; n++)
		{
			*output_block++ = dct_sum(input_block, m, n);
		}
	}
}

//Write the "block" (index_x, index_y) to the given pixel buffer.
//The dimensions of the image (in blocks) is given by "blocks_x" x "blocks_y".
static void write_block(bitmap_pixel_rgb_t* pixels, int index_x, int index_y, int blocks_x, int blocks_y, const float* block)
{
	int bytes_per_row = blocks_x * 8;
	int base_offset = index_y * 8 * bytes_per_row;
	int col_offset = index_x * 8;

	for (int curr_y = 0; curr_y < 8; curr_y++)
	{
		int rowOffset = curr_y * bytes_per_row;

		for (int curr_x = 0; curr_x < 8; curr_x++)
		{
			int offset = base_offset + rowOffset + col_offset + curr_x;

			float component = round(block[(8 * curr_y) + curr_x]) + 128;
			bitmap_component_t clamped_component = (bitmap_component_t)MAX(MIN(255, component), 0);

			pixels[offset].r = clamped_component;
			pixels[offset].g = clamped_component;
			pixels[offset].b = clamped_component;
		}
	}
}


int decompress(const char* file_path, const int* quant_matrix, const char* output_path)
{

	int blocks_x, blocks_y;
	
	//Open the output file (.bmp):
	FILE* file_in = fopen(file_path, "rb");

	if (!file_in)
	{
		printf("Failed to create output file!\n");
		return -1;
	}

	if (fread(&blocks_x, sizeof(blocks_x), 1, file_in) != 1)
	{
		printf("Failed to read number of blocks in x direction!\n”");
		fclose(file_in);

		return -1;
	}


	if (fread(&blocks_y, sizeof(blocks_y), 1, file_in) != 1)
	{
		printf("Failed to read number of blocks in y direction!\n”");
		fclose(file_in);

		return -1;
	}

		
	bitmap_pixel_rgb_t* pixels = (bitmap_pixel_rgb_t*)malloc((blocks_x*8 * blocks_y*8)*3);

	bitmap_parameters_t parameters =
	{
		.bottomUp = BITMAP_BOOL_TRUE,
		.widthPx = blocks_x*8,
		.heightPx = blocks_y*8,
		.colorDepth = BITMAP_COLOR_DEPTH_24,
		.compression = BITMAP_COMPRESSION_NONE,
		.dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
		.colorSpace = BITMAP_COLOR_SPACE_RGB,
	};

	
	for (int index_y = 0; index_y < blocks_y; index_y++)
	{
		for (int index_x = 0; index_x < blocks_x; index_x++)
		{	
			int8_t input_block[64];
			int8_t un_zig_zagged_block[64];
			float original_block[64];
			float quantized_block[64];
			

			for(int i = 0; i < 64; i++)
			{
				if (fread(&input_block[i], sizeof(int8_t), 1, file_in) != 1)
				{
					printf("Failed to read value!\n”");
					fclose(file_in);

					return -1;
				}
			}
			
			// Unzigzag the input_block
			un_zig_zag(input_block, un_zig_zagged_block);

			// Dequantize the unzigzagged_block
			dequantize(un_zig_zagged_block, quant_matrix, quantized_block);

			// Inverse DCT
			perform_inverse_dct(quantized_block, original_block);

			// Create pixels for the .bmp
			write_block(pixels, index_x, index_y, blocks_x, blocks_y, original_block);
		}
	}

	// Write whole bmp image to disk
	bitmapWritePixels(output_path, BITMAP_BOOL_TRUE, &parameters, (bitmap_pixel_t*)pixels);

	//Close the output file:
	fclose(file_in);

	//Free the pixels:
	free(pixels);

	return -1;
}
