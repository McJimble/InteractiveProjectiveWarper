#pragma once

#include <cfloat>

/*
 * Base struct for an RGBA (4 channel) pixel, designed to be contiguous
 * in memory. Each channel can be accessed individually.
 *
 * Also provides a static functions for creating an array
 * of pixels in the most efficient data structure possible.
 * All functions here should be static so that c++ stores this struct's data
 * in memory as plain old data (to still allow 2d array access contiguously)
 *
 */
struct PixelRGBA
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
       
    /*  Creates a 2D pixmap of PixelRGBAs in such a way
     *  that data storage is contiguous and efficient.
     *
     *  initValues: sets all pixel's values to (0, 0, 0, 255) if true.
     *  Otherwise the data isn't written to yet.
     *
     *  Returns the double pointer to the first element of the array pixels
     *  are stored in, allowing for 2D array access ( eg. pixmap[2][1] )
     */ 
	static PixelRGBA** CreatePixmap(const int& rows, const int& cols, bool initValues);
	
    /*
     *  Copy all data from "fromPixmap" into a new PixelRGBA object and returns
     *  the new object. Returned pixmap will have same dimenstions as "from"
     */
    static PixelRGBA** CopyPixmap(PixelRGBA**& fromPixmap, const int& rows, const int& cols);
    
    /*
     *  Read the contiguous array of chars that will be received when reading
     *  pixels from the screen, converting this data to a contiguous 2D array of PixelRGBAs.
     *
     *  oldPixmap: If oldPixmap is not null, it will be overridden with the new data. Regardless, a new
     *  2D pixmap will be created using CreatePixmap. Also, overwrites data in oldPixmap as it's a reference.
     *
     *  readPixmap: Assumes this is an array of unsigned chars of size [nchannels * width * weight], as
     *  this structure will be used when reading pixel data with OIIO
     */
    static void ContiguousDataToPixmap(PixelRGBA**& oldPixmap, unsigned char*& copyPixmap,
        const int& width, const int& height, const int& channels);

    /*
     *  Helper func. to clear data of a pixmap
     */
    static void DeletePixmap(PixelRGBA**& pixmap);
};