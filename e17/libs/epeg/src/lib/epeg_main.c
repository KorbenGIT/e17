#include "Epeg.h"
#include "epeg_private.h"

static Epeg_Image   *_epeg_open_header         (Epeg_Image *im);
static void          _epeg_decode              (Epeg_Image *im);
static void          _epeg_scale               (Epeg_Image *im);
static void          _epeg_encode              (Epeg_Image *im);

static void          _epeg_fatal_error_handler (j_common_ptr cinfo);
static void          _epeg_error_handler       (j_common_ptr cinfo);
static void          _epeg_error_handler2      (j_common_ptr cinfo, int msg_level);

/**
 * Open a JPEG image by filename.
 * @param file The file path to open.
 * @return A handle to the opened JPEG file, with the header decoded.
 * 
 * This function opens the file indicated by the @p file parameter, and
 * attempts to decode it as a jpeg file. If this failes, NULL is returned.
 * Otherwise a valid handle to an open JPEG file is returned that can be used
 * by other Epeg calls.
 * 
 * The @p file must be a pointer to a valid C string, NUL (0 byte) terminated
 * thats is a relative or absolute file path. If not results are not
 * determined.
 * 
 * See also: epeg_memory_open(), epeg_close()
 */
Epeg_Image *
epeg_file_open(const char *file)
{
   Epeg_Image *im;
   
   im = calloc(1, sizeof(Epeg_Image));
   im->in.file = strdup(file);
   im->in.f = fopen(im->in.file, "rb");
   if (!im->in.f)
     {
	epeg_close(im);
	return NULL;
     }
   fstat(fileno(im->in.f), &(im->stat_info));
   im->out.quality = 75;
   return _epeg_open_header(im);
}

/**
 * Open a JPEG image stored in memory.
 * @param data A pointer to the memory containing the JPEG data.
 * @param size The size of the memory segment containing the JPEG.
 * @return  A handle to the opened JPEG, with the header decoded.
 * 
 * This function opens a JPEG file that is stored in memory pointed to by
 * @p data, and that is @p size bytes in size. If successful a valid handle
 * is returned, or on failure NULL is returned.
 * 
 * See also: epeg_file_open(), epeg_close()
 */
Epeg_Image *
epeg_memory_open(unsigned char *data, int size)
{
   Epeg_Image *im;
   
   im = calloc(1, sizeof(Epeg_Image));
   im->in.f = _epeg_memfile_read_open(data, size);
   if (!im->in.f)
     {
	epeg_close(im);
	return NULL;
     }
   im->out.quality = 75;
   return _epeg_open_header(im);
}

/**
 * Return the original JPEG pixel size.
 * @param im A handle to an opened Epeg image.
 * @param w A pointer to the width value in pixels to be filled in.
 * @param h A pointer to the height value in pixels to be filled in.
 * 
 * Returns the image size in pixels.
 * 
 */
void
epeg_size_get(Epeg_Image *im, int *w, int *h)
{
   if (w) *w = im->in.w;
   if (h) *h = im->in.h;
}

/**
 * Set the size of the image to decode in pixels.
 * @param im A handle to an opened Epeg image.
 * @param w The width of the image to decode at, in pixels.
 * @param h The height of the image to decode at, in pixels.
 * 
 * Sets the size at which to deocode the JPEG image, giving an optimised load
 * that only decodes the pixels needed.
 * 
 */
void
epeg_decode_size_set(Epeg_Image *im, int w, int h)
{
   if      (im->pixels) return;
   if      (w < 1)        w = 1;
   else if (w > im->in.w) w = im->in.w;
   if      (h < 1)        h = 1;
   else if (h > im->in.h) h = im->in.h;
   im->out.w = w;
   im->out.h = h;
}

/**
 * Set the colorspace in which to decode the image.
 * @param im A handle to an opened Epeg image.
 * @param colorspace The colorspace to decode the image in.
 * 
 * This sets the colorspace to decode the image in. The default is EPEG_YUV8,
 * as this is normally the native colorspace of a JPEG file, avoiding any
 * colorspace conversions for a faster load and/or save.
 */
void
epeg_decode_colorspace_set(Epeg_Image *im, Epeg_Colorspace colorspace)
{
   if (im->pixels) return;
   if ((colorspace < EPEG_GRAY8) || (colorspace > EPEG_ARGB32)) return;
   im->color_space = colorspace;
}

/**
 * Get a segment of decoded pixels from an image.
 * @param im A handle to an opened Epeg image.
 * @param x Rectangle X.
 * @param y Rectangle Y.
 * @param w Rectangle width.
 * @param h Rectangle height.
 * @return Pointer to the top left of the requested pixel block.
 * 
 * Return image pixels in the decoded format from the specified location
 * rectangle bounded with the box @p x, @p y @p w X @p y. The pixel block is
 * packed with no row padding, and it organsied from top-left to bottom right,
 * row by row. You must free the pixel block using epeg_pixels_free() before
 * you close the image handle, and assume the pixels to be read-only memory.
 * 
 * On success the pointer is returned, on failure, NULL is returned. Failure
 * may be because the rectangle is out of the bounds of the image, memory
 * allocations failed or the image data cannot be decoded.
 * 
 */
const void *
epeg_pixels_get(Epeg_Image *im, int x, int y,  int w, int h)
{
   int xx, yy, ww, hh, bpp, ox, oy, ow, oh, iw, ih;
   
   if (!im->pixels) _epeg_decode(im);
   if (!im->pixels) return NULL;
   bpp = im->in.jinfo.output_components;
   iw = im->out.w;
   ih = im->out.h;
   ow = w;
   oh = h;
   ox = 0;
   oy = 0;
   if ((x + ow) > iw) ow = iw - x;
   if ((y + oh) > ih) oh = ih - y;
   if (ow < 1) return NULL;
   if (oh < 1) return NULL;
   if (x < 0)
     {
	ow += x;
	ox = -x;
     }
   if (y < 0)
     {
	oh += y;
	oy = -y;
     }
   if (ow < 1) return NULL;
   if (oh < 1) return NULL;

   ww = x + ox + ow;
   hh = y + oy + oh;

   if (im->color_space == EPEG_GRAY8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 1);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox));
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = s[0];
		  p++;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_YUV8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 3);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox) * 3);
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = s[0];
		  p[1] = s[1];
		  p[2] = s[2];
		  p += 3;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_RGB8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 3);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox) * 3);
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = s[0];
		  p[1] = s[1];
		  p[2] = s[2];
		  p += 3;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_BGR8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 3);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox) * 3);
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = s[2];
		  p[1] = s[1];
		  p[2] = s[0];
		  p += 3;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_RGBA8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 4);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox) * 4);
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = s[0];
		  p[1] = s[1];
		  p[2] = s[2];
		  p[3] = 0xff;
		  p += 4;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_BGRA8)
     {
	unsigned char *pix, *p;
	
	pix = malloc(w * h * 4);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox) * 4);
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = 0xff;
		  p[1] = s[2];
		  p[2] = s[1];
		  p[3] = s[0];
		  p += 4;
		  s += bpp;
	       }
	  }
	return pix;
     }
   else if (im->color_space == EPEG_ARGB32)
     {
	unsigned int *pix, *p;
	
	pix = malloc(w * h * 4);
	if (!pix) return NULL;
	for (yy = y + oy; yy < hh; yy++)
	  {
	     unsigned char *s;
	     
	     s = im->lines[yy] + ((x + ox) * bpp);
	     p = pix + ((((yy - y) * w) + ox));
	     for (xx = x + ox; xx < ww; xx++)
	       {
		  p[0] = 0xff000000 | (s[0] << 16) | (s[1] << 8) | (s[2]);
		  p++;
		  s += bpp;
	       }
	  }
	return pix;
     }
   return NULL;
}

/**
 * Free requested pixel block from an image.
 * @param im A handle to an opened Epeg image.
 * @param data The pointer to the image pixels.
 * 
 * This frees the data for a block of pixels requested from image @p im.
 * @p data must be a valid (non NULL) pointer to a pixel block taken from the
 * image @p im by epeg_pixels_get() and mustbe called before the image is
 * closed by epeg_close().
 */
void
epeg_pixels_free(Epeg_Image *im, const void *data)
{
   free((void *)data);
}

/**
 * Get the image comment field as a string.
 * @param im A handle to an opened Epeg image.
 * @return A pointer to the loaded image comments.
 * 
 * This function returns the comment field as a string (NUL byte terminated)
 * of the loaded image @p im, if there is a comment, or NULL if no comment is
 * saved with the image. Consider the string returned to be read-only.
 * 
 */
const char *
epeg_comment_get(Epeg_Image *im)
{
   return im->in.comment;
}

/**
 * Get thumbnail comments of loaded image.
 * @param im A handle to an opened Epeg image.
 * @param info Pointer to a thumbnail info struct to be filled in.
 * 
 * This function retrieves thumbnail comments written by Epeg to any saved
 * JPEG files. If no thumbnail comments were saved, the fields will be 0 in
 * the @p info struct on return.
 * 
 */
void
epeg_thumbnail_comments_get(Epeg_Image *im, Epeg_Thumbnail_Info *info)
{
   if (!info) return;
   info->uri      = im->in.thumb_info.uri;
   info->mtime    = im->in.thumb_info.mtime;
   info->w        = im->in.thumb_info.w;
   info->h        = im->in.thumb_info.h;
   info->mimetype = im->in.thumb_info.mime;
}

/**
 * Set the comment field of the image for saving.
 * @param im A handle to an opened Epeg image.
 * @param comment The comment to set.
 * 
 * Set the comment for the image file for when it gets saved. This is a NUL
 * byte terminated C string. If @p comment is NULL the output file will have
 * no comment field.
 * 
 * The default comment will be any comment loaded from the input file.
 * 
 */
void
epeg_comment_set(Epeg_Image *im, const char *comment)
{
   if (im->out.comment) free(im->out.comment);
   if (!comment) im->out.comment = NULL;
   else im->out.comment = strdup(comment);
}

/**
 * Set the encoding quality of the saved image.
 * @param im A handle to an opened Epeg image.
 * @param quality The quality of encoding from 0 to 100.
 * 
 * Set the quality of the output encoded image. Values from 0 to 100
 * inclusive are valid, with 100 being the maximum quality, and 0 being the
 * minimum. If the quality is set equal to or above 90%, the output U and V
 * color planes are encoded at 1:1 with the Y plane.
 * 
 * The default quality is 75.
 * 
 */
void
epeg_quality_set(Epeg_Image *im, int quality)
{
   if      (quality < 0)   quality = 0;
   else if (quality > 100) quality = 100;
   im->out.quality = quality;
}

/**
 * Enable thumbnail comments in saved image.
 * @param im A handle to an opened Epeg image.
 * @param onoff A boolean on and off enabling flag.
 * 
 * if @p onoff is 1, the output file will have thumbnail comments added to
 * it, and if it is 0, it will not. The default is 0.
 * 
 */
void
epeg_thumbnail_comments_enable(Epeg_Image *im, int onoff)
{
   im->out.thumbnail_info = onoff;
}

/**
 * Set the output file path for the image when saved.
 * @param im A handle to an opened Epeg image.
 * @param file The path to the output file.
 * 
 * This sets the output file path name (either a full or relative path name)
 * to where the file will be written when saved. @p file must be a NUL
 * terminated C string conatining the path to the file to be saved to. If it is
 * NULL, the image will not be saved to a file when calling epeg_encode().
 */
void
epeg_file_output_set(Epeg_Image *im, const char *file)
{
   if (im->out.file) free(im->out.file);
   if (!file) im->out.file = NULL;
   else im->out.file = strdup(file);
}

/**
 * Set the output file to be a block of allocated memory.
 * @param im A handle to an opened Epeg image.
 * @param data A pointer to a pointer to a memory block.
 * @param size A pointer to a counter of the size of the memory block.
 * 
 * This sets the output encoding of the image when saved to be allocated
 * memory. After epeg_close() is called the pointer pointed to by @p data
 * and the integer pointed to by @p size will contain the pointer to the
 * memory block and its size in bytes, respecitvely. The memory block can be
 * freed with the free() function call. If the save fails the pointer to the
 * memory block will be unaffected, as will the size.
 * 
 */
void
epeg_memory_output_set(Epeg_Image *im, unsigned char **data, int *size)
{
   im->out.mem.data = data;
   im->out.mem.size = size;
}

/**
 * This saved the image to its specified destination.
 * @param im A handle to an opened Epeg image.
 * 
 * This saves the image @p im to its destination specified by
 * epeg_file_output_set() or epeg_memory_output_set(). The image will be
 * encoded at the deoded pixel size, using the quality, comment and thumbnail
 * comment settings set on the image.
 */
void
epeg_encode(Epeg_Image *im)
{
   _epeg_decode(im);
   _epeg_scale(im);
   _epeg_encode(im);
}

/**
 * Close an image handle.
 * @param im A handle to an opened Epeg image.
 * 
 * This closes an opened image handle and frees all memory associated with it.
 * It does not free encoded data generated by epeg_memory_output_set() followed
 * by epeg_encode() nor does it guarantee to free any data recieved by
 * epeg_pixels_get(). Once an image handle is closed consider it invalid.
 */
void
epeg_close(Epeg_Image *im)
{
   if (im->pixels)             free(im->pixels);
   if (im->lines)              free(im->lines);
   if (im->in.file)            free(im->in.file);
   if (im->in.f)               jpeg_destroy_decompress(&(im->in.jinfo));
   if (im->in.f)               fclose(im->in.f);
   if (im->in.comment)         free(im->in.comment);
   if (im->in.thumb_info.uri)  free(im->in.thumb_info.uri);
   if (im->in.thumb_info.mime) free(im->in.thumb_info.mime);
   if (im->out.file)           free(im->out.file);
   if (im->out.f)              jpeg_destroy_compress(&(im->out.jinfo));
   if (im->out.f)              fclose(im->out.f);
   if (im->out.comment)        free(im->out.comment);
   free(im);
}















static Epeg_Image *
_epeg_open_header(Epeg_Image *im)
{
   struct jpeg_marker_struct *m;
   
   im->jerr.pub.error_exit     = _epeg_fatal_error_handler;
   im->jerr.pub.emit_message   = _epeg_error_handler2;
   im->jerr.pub.output_message = _epeg_error_handler;

   im->in.jinfo.err  = jpeg_std_error(&(im->jerr.pub));
   
   if (setjmp(im->jerr.setjmp_buffer))
     {
	error:
	epeg_close(im);
	return NULL;
     }

   jpeg_create_decompress(&(im->in.jinfo));
   jpeg_save_markers(&(im->in.jinfo), JPEG_APP0 + 7, 1024);
   jpeg_save_markers(&(im->in.jinfo), JPEG_COM,      65535);
   jpeg_stdio_src(&(im->in.jinfo), im->in.f);
   jpeg_read_header(&(im->in.jinfo), TRUE);
   im->in.w = im->in.jinfo.image_width;
   im->in.h = im->in.jinfo.image_height;
   if (im->in.w <= 1) goto error;
   if (im->in.h <= 1) goto error;
   
   im->out.w = im->in.w;
   im->out.h = im->in.h;
   
   im->color_space = ((im->in.color_space = im->in.jinfo.out_color_space) == JCS_GRAYSCALE)?EPEG_GRAY8:EPEG_YUV8;

   for (m = im->in.jinfo.marker_list; m; m = m->next)
     {
	if (m->marker == JPEG_COM)
	  {
	     if (im->in.comment) free(im->in.comment);
	     im->in.comment = malloc(m->data_length + 1);
	     if (im->in.comment)
	       {
		  memcpy(im->in.comment, m->data, m->data_length);
		  im->in.comment[m->data_length] = 0;
	       }
	  }
	else if (m->marker == (JPEG_APP0 + 7))
	  {
	     if ((m->data_length > 7) &&
		 (!strncmp((char *)m->data, "Thumb::", 7)))
	       {
		  char *p, *p2;
		  
		  p = malloc(m->data_length + 1);
		  if (p)
		    {
		       memcpy(p, m->data, m->data_length);
		       p[m->data_length] = 0;
		       p2 = strchr(p, '\n');
		       if (p2)
			 {
			    p2[0] = 0;
			    if (!strcmp(p, "Thumb::URI"))

			      im->in.thumb_info.uri = strdup(p2 + 1);
			    else if (!strcmp(p, "Thumb::MTime"))
			      sscanf(p2 + 1, "%llu", &(im->in.thumb_info.mtime));
			    else if (!strcmp(p, "Thumb::Image::Width"))
			      im->in.thumb_info.w = atoi(p2 + 1);
			    else if (!strcmp(p, "Thumb::Image::Height"))
			      im->in.thumb_info.h = atoi(p2 + 1);
			    else if (!strcmp(p, "Thumb::Mimetype"))
			      im->in.thumb_info.mime = strdup(p2 + 1);
			 }
		       free(p);
		    }
	       }
	  }
     }
   return im;
}

static void
_epeg_decode(Epeg_Image *im)
{
   int scale, scalew, scaleh, y;
   
   if (im->pixels) return;
   
   scalew = im->in.w / im->out.w;
   scaleh = im->in.h / im->out.h;
   
   scale = scalew;   
   if (scaleh < scalew) scale = scaleh;

   if      (scale > 8) scale = 8;
   else if (scale < 1) scale = 1;
   
   im->in.jinfo.scale_num           = 1;
   im->in.jinfo.scale_denom         = scale;
   im->in.jinfo.do_fancy_upsampling = FALSE;
   im->in.jinfo.do_block_smoothing  = FALSE;
   im->in.jinfo.dct_method          = JDCT_IFAST;

   switch (im->color_space)
     {
      case EPEG_GRAY8:
	im->in.jinfo.out_color_space = JCS_GRAYSCALE;
	im->in.jinfo.output_components = 1;
	break;
      case EPEG_YUV8:
	im->in.jinfo.out_color_space = JCS_YCbCr;
	break;
      case EPEG_RGB8:
      case EPEG_BGR8:
      case EPEG_RGBA8:
      case EPEG_BGRA8:
      case EPEG_ARGB32:
	im->in.jinfo.out_color_space = JCS_RGB;
	break;
      default:
	break;
     }
   
   jpeg_calc_output_dimensions(&(im->in.jinfo));
   
   im->pixels = malloc(im->in.jinfo.output_width * im->in.jinfo.output_height *
		       im->in.jinfo.output_components);
   if (!im->pixels) return;
   im->lines = malloc(im->in.jinfo.output_height * sizeof(char *));
   if (!im->lines) return;
   jpeg_start_decompress(&(im->in.jinfo));
   
   for (y = 0; y < im->in.jinfo.output_height; y++)
     im->lines[y] = im->pixels + 
     (y * im->in.jinfo.output_components * im->in.jinfo.output_width);
   
   while (im->in.jinfo.output_scanline < im->in.jinfo.output_height)
     jpeg_read_scanlines(&(im->in.jinfo), 
			 &(im->lines[im->in.jinfo.output_scanline]), 
			 im->in.jinfo.rec_outbuf_height);
   
   jpeg_finish_decompress(&(im->in.jinfo));
}

static void
_epeg_scale(Epeg_Image *im)
{
   unsigned char *dst, *row, *src;
   int            x, y, w, h, i;
   
   if ((im->in.w == im->out.w) && (im->in.h == im->out.h)) return;
   if (im->scaled) return;
   
   im->scaled = 1;
   w = im->out.w;
   h = im->out.h;
   for (y = 0; y < h; y++)
     {
	row = im->pixels + 
	  (((y * im->in.jinfo.output_height) / h) *
	   im->in.jinfo.output_components * im->in.jinfo.output_width);
	dst = im->pixels + 
	  (y * im->in.jinfo.output_components * im->in.jinfo.output_width);
	for (x = 0; x < im->out.w; x++)
	  {
	     src = row + 
	       (((x * im->in.jinfo.output_width) / w) *
		im->in.jinfo.output_components);
	     for (i = 0; i < im->in.jinfo.output_components; i++)
	       dst[i] = src[i];
	     dst += im->in.jinfo.output_components;
	  }
     }
}

static void
_epeg_encode(Epeg_Image *im)
{
   void  *data = NULL;
   size_t size = 0;
   
   if (im->out.f) return;
   
   if (im->out.file)
     im->out.f = fopen(im->out.file, "wb");
   else
     im->out.f = _epeg_memfile_write_open(&data, &size);
   if (!im->out.f)
     {
	im->error = 1;
	return;
     }
   im->out.jinfo.err = jpeg_std_error(&(im->jerr.pub));
   jpeg_create_compress(&(im->out.jinfo));
   jpeg_stdio_dest(&(im->out.jinfo), im->out.f);
   im->out.jinfo.image_width      = im->out.w;
   im->out.jinfo.image_height     = im->out.h;
   im->out.jinfo.input_components = im->in.jinfo.output_components;
   im->out.jinfo.in_color_space   = im->in.jinfo.out_color_space;
   im->out.jinfo.dct_method       = JDCT_IFAST;
   jpeg_set_defaults(&(im->out.jinfo));
   jpeg_set_quality(&(im->out.jinfo), im->out.quality, TRUE);   
   
   if (im->out.quality >= 90)
     {
	im->out.jinfo.comp_info[0].h_samp_factor = 1;
	im->out.jinfo.comp_info[0].v_samp_factor = 1;
	im->out.jinfo.comp_info[1].h_samp_factor = 1;
	im->out.jinfo.comp_info[1].v_samp_factor = 1;
	im->out.jinfo.comp_info[2].h_samp_factor = 1;
	im->out.jinfo.comp_info[2].v_samp_factor = 1;
     }
   jpeg_start_compress(&(im->out.jinfo), TRUE);

   if (im->out.comment)
     jpeg_write_marker(&(im->out.jinfo), JPEG_COM, 
		       im->out.comment, strlen(im->out.comment));

   if (im->out.thumbnail_info)
     {
	char buf[8192];
	
	if (im->in.file)
	  {
	     snprintf(buf, sizeof(buf), "Thumb::URI\nfile://%s", im->in.file);
	     jpeg_write_marker(&(im->out.jinfo), JPEG_APP0 + 7, buf, strlen(buf));
	     snprintf(buf, sizeof(buf), "Thumb::MTime\n%llu", 
		      (unsigned long long int)im->stat_info.st_mtime);
	  }
	jpeg_write_marker(&(im->out.jinfo), JPEG_APP0 + 7, buf, strlen(buf));
	snprintf(buf, sizeof(buf), "Thumb::Image::Width\n%i", im->in.w);
	jpeg_write_marker(&(im->out.jinfo), JPEG_APP0 + 7, buf, strlen(buf));
	snprintf(buf, sizeof(buf), "Thumb::Image::Height\n%i", im->in.h);
	jpeg_write_marker(&(im->out.jinfo), JPEG_APP0 + 7, buf, strlen(buf));
	snprintf(buf, sizeof(buf), "Thumb::Mimetype\nimage/jpeg");
	jpeg_write_marker(&(im->out.jinfo), JPEG_APP0 + 7, buf, strlen(buf));
     }
   
   while (im->out.jinfo.next_scanline < im->out.h)
     jpeg_write_scanlines(&(im->out.jinfo), 
			  &(im->lines[im->out.jinfo.next_scanline]), 1);
   
   jpeg_finish_compress(&(im->out.jinfo));
   
   if (im->in.f)                       jpeg_destroy_decompress(&(im->in.jinfo));
   if ((im->in.f) && (im->in.file))    fclose(im->in.f);
   if ((im->in.f) && (!im->in.file))   _epeg_memfile_read_close(im->in.f);
   if (im->out.f)                      jpeg_destroy_compress(&(im->out.jinfo));
   if ((im->out.f) && (im->out.file))  fclose(im->out.f);
   if ((im->out.f) && (!im->out.file)) _epeg_memfile_write_close(im->out.f);
   im->in.f = NULL;
   im->out.f = NULL;
   
   if (im->out.mem.data) *(im->out.mem.data) = data;
   if (im->out.mem.size) *(im->out.mem.size) = size;
}

static void 
_epeg_fatal_error_handler(j_common_ptr cinfo)
{
   emptr errmgr;
   
   errmgr = (emptr)cinfo->err;
   return;
}

static void 
_epeg_error_handler(j_common_ptr cinfo)
{
   emptr errmgr;
   
   errmgr = (emptr) cinfo->err;
   return;
}

static void
_epeg_error_handler2(j_common_ptr cinfo, int msg_level)
{
   emptr errmgr;
   
   errmgr = (emptr) cinfo->err;
   return;
   msg_level = 0;
}
