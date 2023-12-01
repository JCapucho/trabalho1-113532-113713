/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 113532 Name: Igor Coelho
// NMec: 113713 Name: João Capucho
//
//
// Date: 23rd November, 2023
//

#include "image8bit.h"

#include "instrumentation.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8 *pixel; // pixel data (a raster scan)
};

// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char *errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char *ImageErrMsg() { ///
  return errCause;
}

// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success =
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
//
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
//
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
//
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)

// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char *failmsg) {
  errCause = (char *)(condition ? "" : failmsg);
  return condition;
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  InstrName[1] = "greycmp"; // InstrCount[1] will count grey value comparations
  InstrName[2] = "divisions"; // InstrCount[2] will count divisions
                              // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
#define GREYCMP InstrCount[1]
#define DIVISIONS InstrCount[2]
// Add more macros here...

// Helper macros for iteration over the coordinates of an image

// Macro to iterate over a rect of WIDTH by HEIGHT, X and Y are the variable
// names where the coordinates for the current iteration will be stored.
//
// The body must be specified exactly as in a for loop, either a single
// statement after the macro invocation or a block delimited by braces.
//
// The order of iteration here is important, since the image is laid out such
// that pixels on the same line are adjacent, by iterating over each line
// (instead of each column) memory accesses will be more likely to hit cache
// since modern processors caches store wide lanes of contiguous memory [^1].
//
// Additionally since, once again, the memory is contiguous it's possible for
// vectorization of the code by using SIMD (Single Instruction/Multiple Data)
// instructions, this could be done manually (or potentially automatically by
// the compiler [^2]).
//
// [1]: http://igoro.com/archive/gallery-of-processor-cache-effects
// [2]: https://en.wikipedia.org/wiki/Automatic_vectorization
#define FOR_COORDINATES_SIZED(X, Y, WIDTH, HEIGHT)                             \
  for (int Y = 0; Y < HEIGHT; Y++)                                             \
    for (int X = 0; X < WIDTH; X++)

// This macro is very similar to FOR_COORDINATES_SIZED but instead of receiving
// the width and the height as arguments they are sourced directly from an
// Image.
//
// Same considerations as in FOR_COORDINATES_SIZED apply.
#define FOR_COORDINATES(IMG, X, Y)                                             \
  FOR_COORDINATES_SIZED(X, Y, IMG->width, IMG->height)

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { ///
  assert(width >= 0);
  assert(height >= 0);
  assert(0 < maxval && maxval <= PixMax);

  // Allocate the image struct backing memory
  const Image image = (Image)malloc(sizeof(struct image));
  if (check(image == NULL, "Failed to allocate image"))
    return NULL;

  // Allocate the pixel data buffer
  uint8 *pixel = (uint8 *)calloc(width * height, sizeof(uint8));
  if (check(pixel == NULL, "Failed to allocate pixel data")) {
    // The image still was allocated so it needs to be freed
    free(image);
    return NULL;
  }

  image->width = width;
  image->height = height;
  image->maxval = maxval;
  image->pixel = pixel;

  return image;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image *imgp) { ///
  assert(imgp != NULL);

  if (*imgp == NULL)
    return;

  free((*imgp)->pixel);
  free(*imgp);
  *imgp = NULL;
}

/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE *f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char *filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE *f = NULL;
  Image img = NULL;

  int success =
      check((f = fopen(filename, "rb")) != NULL, "Open failed") &&
      // Parse PGM header
      check(fscanf(f, "P%c ", &c) == 1 && c == '5', "Invalid file format") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &h) == 1 && h >= 0, "Invalid height") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d", &maxval) == 1 && 0 < maxval &&
                maxval <= (int)PixMax,
            "Invalid maxval") &&
      check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected") &&
      // Allocate image
      (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
      // Read pixels
      check(fread(img->pixel, sizeof(uint8), w * h, f) == w * h,
            "Reading pixels");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL)
    fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char *filename) { ///
  assert(img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE *f = NULL;

  int success = check((f = fopen(filename, "wb")) != NULL, "Open failed") &&
                check(fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0,
                      "Writing header failed") &&
                check(fwrite(img->pixel, sizeof(uint8), w * h, f) == w * h,
                      "Writing pixels failed");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (f != NULL)
    fclose(f);
  return success;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert(img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert(img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert(img != NULL);
  return img->maxval;
}

/// Calculates the area of the image
static inline int ImageArea(Image img) { return img->width * img->height; }

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8 *min, uint8 *max) { ///
  assert(img != NULL);

  int num_pixels = ImageArea(img);
  *min = *max = ImageGetPixel(img, 0, 0);

  for (int i = 1; i < num_pixels; i++) {
    PIXMEM++; // count one pixel access (read)
    const uint8 level = img->pixel[i];

    if (level > *max) {
      *max = level;
    } else if (level < *min) {
      *min = level;
    }
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert(img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert(img != NULL);
  assert(x >= 0 && y >= 0);
  assert(w >= 0 && h >= 0);

  // Subtract the dimensions instead of adding the dimension and the offset,
  // this prevents the value from potentially overflowing, since the dimensions
  // are always positive their subtraction will never underflow.
  return img->width - w >= x && img->height - h >= y;
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel.
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  const int index = y * img->width + x;
  assert(0 <= index && index < img->width * img->height);
  return index;
}

/// Clamps the value to be between min and max
/// This is an internal function.
static int clamp(int val, int min, int max) {
  const int t = val < min ? min : val;
  return t > max ? max : t;
}

/// Calculates the integer division between the numerator
/// and denominator respectively, rounding the result.
static inline int round_div(int num, int denom) {
  DIVISIONS++;
  return (int)((double)num / (double)denom + 0.5);
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (read)
  return img->pixel[G(img, x, y)];
}

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
}

/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.

/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
  assert(img != NULL);
  const size_t num_pixels = ImageArea(img);
  for (size_t i = 0; i < num_pixels; i++) {
    PIXMEM += 2; // count two pixels accesses (1 read and 1 write)
    img->pixel[i] = img->maxval - img->pixel[i];
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert(img != NULL);
  const size_t num_pixels = ImageArea(img);
  for (size_t i = 0; i < num_pixels; i++) {
    PIXMEM += 2; // count two pixels accesses (1 read and 1 write)
    const int current_value = img->pixel[i];
    img->pixel[i] = current_value >= thr ? img->maxval : 0;
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert(img != NULL);
  assert(factor >= 0.0);
  const size_t num_pixels = ImageArea(img);
  for (size_t i = 0; i < num_pixels; i++) {
    PIXMEM += 2; // count two pixels accesses (1 read and 1 write)
    const int current_value = img->pixel[i];
    // Add +0.5 for rounding
    const int updated_value = (int)((double)current_value * factor + 0.5);
    img->pixel[i] = clamp(updated_value, 0, img->maxval);
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint:
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
  assert(img != NULL);

  // The width and height will be swapped since the image is rotated
  const Image new_img = ImageCreate(img->height, img->width, img->maxval);
  // The errno and errCause from ImageCreate will be propagated
  if (new_img == NULL)
    return NULL;

  // If we rotate an image 90 degrees anti-clockwise we can define a function
  // f that will take the current coordinates of a given pixel and map it to
  // a new pixel. This function is defined as
  //
  // f(x, y) := (y, H - x - 1)
  //
  // Where the H is the image height.

  FOR_COORDINATES(img, x, y) {
    const int new_x = y;
    const int new_y = new_img->height - x - 1;
    const uint8 level = ImageGetPixel(img, x, y);
    ImageSetPixel(new_img, new_x, new_y, level);
  }

  return new_img;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) {
  assert(img != NULL);

  const Image new_img = ImageCreate(img->width, img->height, img->maxval);
  // The errno and errCause from ImageCreate will be propagated
  if (new_img == NULL)
    return NULL;

  FOR_COORDINATES(img, x, y) {
    const uint8 level = ImageGetPixel(img, x, y);
    ImageSetPixel(new_img, img->width - x - 1, y, level);
  }

  return new_img;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
  assert(img != NULL);
  assert(ImageValidRect(img, x, y, w, h));

  const Image new_img = ImageCreate(w, h, img->maxval);
  // The errno and errCause from ImageCreate will be propagated
  if (new_img == NULL)
    return NULL;

  FOR_COORDINATES_SIZED(new_x, new_y, w, h) {
    const uint8 level = ImageGetPixel(img, new_x + x, new_y + y);
    ImageSetPixel(new_img, new_x, new_y, level);
  }

  return new_img;
}

/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));

  FOR_COORDINATES(img2, new_x, new_y) {
    const uint8 level = ImageGetPixel(img2, new_x, new_y);
    ImageSetPixel(img1, new_x + x, new_y + y, level);
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));

  FOR_COORDINATES(img2, new_x, new_y) {
    const int old_x = new_x + x;
    const int old_y = new_y + y;

    const int img2_value = ImageGetPixel(img1, old_x, old_y);
    const int img1_value = ImageGetPixel(img2, new_x, new_y);

    const int updated_value = (int)((double)img1_value * alpha +
                                    (double)img2_value * (1 - alpha) + 0.5);
    const int clamped_value = clamp(updated_value, 0, img1->maxval);

    ImageSetPixel(img1, old_x, old_y, clamped_value);
  }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidPos(img1, x, y));

  if (!ImageValidRect(img1, x, y, img2->width, img2->height))
    return 0;

  FOR_COORDINATES(img2, new_x, new_y) {
    const int img1_value = ImageGetPixel(img1, new_x + x, new_y + y);
    const int img2_value = ImageGetPixel(img2, new_x, new_y);

    GREYCMP++;
    if (img1_value != img2_value)
      return 0;
  }

  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px,
/// *py). If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int *px, int *py, Image img2) { ///
  assert(img1 != NULL);
  assert(img2 != NULL);

  if (img2->width > img1->width || img2->height > img1->height)
    return 0;

  // Since the image needs to fit in order to be a subimage, it doesn't make
  // sense to check the last pixels on the end of a line (or the bottom of an
  // image) that wouldn't have enough space to fit the subimage.
  const int check_width = img1->width - img2->width;
  const int check_height = img1->height - img2->height;

  FOR_COORDINATES_SIZED(x, y, check_width + 1, check_height + 1) {
    if (ImageMatchSubImage(img1, x, y, img2)) {
      *px = x;
      *py = y;
      return 1;
    }
  }

  return 0;
}

/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy) {
  assert(img != NULL);
  assert(dx >= 0 && dy >= 0);

  // The blurred pixels will be written to a separate array that will be swapped
  // at the end because the original pixels values will be needed at all times.
  const size_t num_pixels = sizeof(uint8) * ImageArea(img);
  uint8 *blurred_pixels = (uint8 *)malloc(num_pixels);

  if (check(blurred_pixels == NULL, "Failed to allocate memory"))
    return;

  // The algorithm implemented here is based on the ideas of the
  // FMF/FMFT (Fast Mean Filter).
  //
  // The three basic ideas behind these algorithms/techniques are:
  // 1. Since all pixels have equal weight instead of dividing each one and
  //    adding them to obtain the blurred value, the raw values are added and
  //    then divided (this works because of the distributive property).
  // 2. The filter is separable this means that instead of calculating the
  //    entire filter at once, we can instead apply a 1D filter in each axis and
  //    then obtain the full filter by calculating the product of both axis.
  // 3. Since the pixels that contribute in the filter for a given pixel are
  //    almost identical for the adjacent pixels except for two (in case of a 1D
  //    filter) instead of recalculating the sum, the previous sum is used and
  //    the value of the pixel that no longer belongs to the filter window is
  //    removed and the pixel that entered the window has is value added to the
  //    sum.
  //
  // All of these together allows us to design a filter that is essentially
  // independent of the window size, except for a small initialization step.

  // Array of the sums used for the 1D filter spanning the y axis with radius
  // dy.
  int line_sum[img->width];

  // The last valid index in each axis
  int last_x = img->width - 1;
  int last_y = img->height - 1;

  // The effective radius to consider when fetching pixels from memory for the
  // filter sum.
  int radius_x = img->width > dx ? dx : last_x;
  int radius_y = img->height > dy ? dy : last_y;

  // The filter window sizes and areas.
  int win_width = 2 * dx + 1;
  int win_height = 2 * dy + 1;
  int win_area = win_width * win_height;

  // The size of the filter window that exceeds the image size plus 1 for each
  // axis.
  int spill_x = dx >= img->width ? dx - radius_x + 1 : 1;
  int spill_y = dy >= img->height ? dy - radius_y + 1 : 1;

  // Initialization phase
  //
  // This phase is responsible for initializing the sum vector that will be
  // used throughout the algorithm. In here the sum of 1D filter in the y axis
  // will be calculated for all pixels in the first line.
  //
  // The principle of accumulation can't be used here since we don't have a
  // previous value so each pixel in the window will read and it's value added
  // to the sum on the corresponding position.
  for (int x = 0; x < img->width; x++) {
    // The first pixel value will appear once in it's position but also y
    // radius of the filter window times, because we are considering a border
    // clamp sampling of the pixels, this means that all out of bounds pixel
    // accesses will be mapped to the nearest pixel.
    line_sum[x] = (dy + 1) * ImageGetPixel(img, x, 0);

    // Each of the pixels in the effective radius will be added to the sum we
    // are calculating minus the last.
    for (int half_win_y = 1; half_win_y < radius_y; half_win_y++) {
      line_sum[x] += ImageGetPixel(img, x, half_win_y);
    }

    // The last pixel will, like the first pixel, not only appear once but also
    // as many times as the filter window exceeds the image size.
    line_sum[x] += spill_y * ImageGetPixel(img, x, radius_y);
  }

  // From this point on each line will be treated individually to calculate it's
  // blurred values.
  for (int y = 0; y < img->height; y++) {
    // NOTE: This part could be made more efficient (in run time, not number of
    // operations), by performing all other operations except this if for the
    // first line, having this loop start at 1 and then removing the if.
    //
    // This would remove a conditional from the hot code path, but increase the
    // code size of the function.
    if (y != 0) {
      // Update phase
      //
      // For all lines, except the first, the sum vector will need to be updated
      // with the new pixels value. But as we already have a previous sum for
      // the last line, we don't need to consider all pixels instead we only
      // subtract the first pixel of the previous window and add the last pixel
      // of the current window.

      // The read coordinates need to be clamped to the image size.
      const int prev_y = clamp(y - dy - 1, 0, last_y);
      const int next_y = clamp(y + dy, 0, last_y);

      for (int x = 0; x < img->width; x++) {
        line_sum[x] +=
            ImageGetPixel(img, x, next_y) - ImageGetPixel(img, x, prev_y);
      }
    }

    // Blur phase
    //
    // Finally the blurred pixel value will be calculated, this is done by first
    // calculating the sum for the first pixel (by summing the values of
    // vertical filter inside the horizontal filter window), this will be used
    // not only for the blurred value of the first pixel but also for
    // accumulation on subsequent pixels.

    // Here, as when calculating the initial values for the sum vector, we
    // multiply the first and last pixel values for the part of the filter
    // window that is out of bounds instead of making multiple clamped reads.
    // All pixels in the effective memory region are read normally and their
    // value added to the sum.
    int sum = (dx + 1) * line_sum[0];
    for (int half_win_x = 1; half_win_x < radius_x; half_win_x++) {
      sum += line_sum[half_win_x];
    }
    sum += spill_x * line_sum[radius_x];

    // Calculate the blurred value by dividing the sum by the window area and
    // store it in the blurred pixels memory.
    PIXMEM++; // count one pixel access (write)
    blurred_pixels[G(img, 0, y)] = round_div(sum, win_area);

    // For all remaining pixels in the line update the sum by removing the first
    // pixel in the previous filter window and adding the new pixel and. Then
    // the blurred pixel value is calculated and updated as done previously.
    for (int x = 1; x < img->width; x++) {
      const int prev_x = clamp(x - dx - 1, 0, last_x);
      const int next_x = clamp(x + dx, 0, last_x);

      sum += line_sum[next_x] - line_sum[prev_x];
      PIXMEM++; // count one pixel access (write)
      blurred_pixels[G(img, x, y)] = round_div(sum, win_area);
    }
  }

  // At this point blurred_pixels contains the new values and the old pixels
  // memory is no longer useful so the pointers are swapped and the old pointer
  // is deallocated.
  uint8 *temp = img->pixel;
  img->pixel = blurred_pixels;
  free(temp);
}
