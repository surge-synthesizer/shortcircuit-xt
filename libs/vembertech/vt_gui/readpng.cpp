#include "vt_gui.h"
#include <png.h>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif

// TODO det är i read_png bottlenecken som gör att editorn laddar långsamt finns
int vg_surface::read_png(string filename)
{		
	static int pngloads=0;
	pngloads++;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	FILE *fp;

	if ((fp = fopen(filename.c_str(), "rb")) == NULL)
	{
//		DebugBreak();
		return -1;
	}

	/* Create and initialize the png_struct with the desired error handler
	* functions.  If you want to use the default stderr and longjump method,
	* you can supply NULL for the last three parameters.  We also supply the
	* the compiler header file version, so that we know if the application
	* was compiled with a compatible version of the library.  REQUIRED
	*/
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);		
		return -1;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return -1;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	* the normal method of doing things with libpng).  REQUIRED unless you
	* set up your own error handlers in the png_create_read_struct() earlier.
	*/

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		/* Free all of the memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(fp);
		/* If we get here, we had a problem reading the file */
		return -1;
	}


	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* If we have already read some of the signature */
	png_set_sig_bytes(png_ptr, sig_read);

	/*
	* If you have enough memory to read in the entire image at once,
	* and you need to specify only transforms that can be controlled
	* with one of the PNG_TRANSFORM_* bits (this presently excludes
	* dithering, filling, setting background, and doing gamma
	* adjustment), then you can read the entire image (including
	* pixels) into the info structure with this call:
	*/
	
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, png_voidp_NULL);   

	/* At this point you have read the entire image */
	
	if (create(info_ptr->width,info_ptr->height))
	{
		unsigned char **row_pointers = new unsigned char*[info_ptr->height];				
		row_pointers = png_get_rows(png_ptr, info_ptr);		   
		
		int w = info_ptr->width;
		int h = info_ptr->height;

		int ch = (info_ptr->pixel_depth)>>3;

		if(ch == 4)
		{
			for(int y=0; y<h; y++)
			{
				memcpy(((unsigned char*)imgdata) + ch*y*sizeXA,row_pointers[y],ch*w);
			}		
		}
		else if(ch == 3)
		{
			for(int y=0; y<h; y++)
			{
				for(int x=0; x<w; x++)
				{
					imgdata[x + y*sizeXA] = 0xff000000 | (row_pointers[y][x*ch+2] << 16) | (row_pointers[y][x*ch + 1] << 8)| (row_pointers[y][x*ch]);
				}
			}		
		}
		else if(ch == 1)
		{
			for(int y=0; y<h; y++)
			{
				for(int x=0; x<w; x++)
				{
					char c = row_pointers[y][x];
					imgdata[x + y*sizeXA] = 0xff000000 | (c<<16) | (c<<8) | c;
				}
			}
		}
	}

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	/* close the file */
	fclose(fp);

	/* that's it */
	return 0;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif