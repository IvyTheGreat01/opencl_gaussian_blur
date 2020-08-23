// Ivan Bystrov
// 20 August 2020
//
// OpenCL kernels compiled and used by blur_gpu.c
// Read the gaussian kernel into local memory but not the image

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

/*
/ Kernel blurs all pixels in in_img and writes to out_img, this is the first pass of the blur
/ @param in_img : the original input image
/ @param out_img : the output image after the first blurring pass
/ @param gaussian_kernel : pointer to local memory where gaussian_kernel can be allocated
/ @param global_gaussian_kernel : pointer to gaussian_kernel that performs the blur (global memory)
/ @param gaussian_kernel_len : length of the gaussian kernel (private memory)
/ @param offset : the index of the gaussian kernel that corresponds to the target pixel (private memory)
/ @param img_dimension : if first pass width of image, if second pass height of image
*/
__kernel void first_pass_blur(read_only image2d_t in_img,	
						write_only image2d_t out_img, 
						__constant float *global_gaussian_kernel, 
						__local float *gaussian_kernel,
						__global float *test_out)
{
	// Copy an element from the global gaussian kernel into local memory for the work group
	uint local_id = get_local_id(0);
	if (local_id < GAUSSIAN_KERNEL_LEN) {
		gaussian_kernel[local_id] = global_gaussian_kernel[local_id];
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	// Get the coordinates of the pixel to be blurred, return early if it is out of bounds of image
	int2 coord = (int2) (get_global_id(0), get_global_id(1));
	if (coord.x >= IMG_WIDTH) { return; }
	
	// Loop over each element of the gaussian kernel and add the multiplication to sum_rgb0
	float4 sum_rgb0 = (float4) (0, 0, 0, 0);
	for (uint i = 0; i < GAUSSIAN_KERNEL_LEN; ++i) {
			int2 pxl_coord = (int2) (coord.x - OFFSET + i, coord.y);
			uint4 pxl_u = (uint4) read_imageui(in_img, sampler, pxl_coord);
			float4 pxl_f = convert_float4(pxl_u);
			sum_rgb0 += pxl_f * global_gaussian_kernel[i];
	}
	if (get_global_id(0) == 130 && get_global_id(1) == 1000) {
		*test_out = global_gaussian_kernel[local_id];
	}

	// Initialize the output vector which also includes alpha component of the original pixel
	uint4 original_pxl = (uint4) read_imageui(in_img, sampler, coord);
	uint4 out_rgba = convert_uint4_sat_rte(sum_rgb0);
	out_rgba.w = original_pxl.w;
	
	/*
	float4 original_pxlf = convert_float4(original_pxl);
	float4 out_rgbaf = (sum_rgb0.x, sum_rgb0.y, sum_rgb0.z, original_pxlf.w);
	uint4 out_rgba = convert_uint4(out_rgbaf);
	*/

	// Write the new pixel to the new image
	write_imageui(out_img, coord, out_rgba); 
}
