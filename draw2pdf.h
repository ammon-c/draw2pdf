//--------------------------------------------------------------------
// draw2pdf.h - Class for drawing simple vector graphics (lines and
// polygons) to an Adobe PDF document file.
//
// (C) Copyright 2022 Ammon R. Campbell.
//
// I wrote this code for use in my own educational and experimental
// programs, but you may also freely use it in yours as long as you
// abide by the following terms and conditions.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials
//     provided with the distribution.
//   * The name(s) of the author(s) and contributors (if any) may not
//     be used to endorse or promote products derived from this
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.  IN OTHER WORDS, USE AT YOUR OWN RISK, NOT OURS.  
//--------------------------------------------------------------------
// LIMITATIONS:
//    * Character set:  Text with US/ANSI characters can be drawn,
//      but foreign/Unicode characters are not yet supported. 
//
//    * Different font faces are not yet supported.  All text is
//      drawn with the default font, typically Helvetica/Arial.
//
//    * Styled lines and pattern fills are not yet supported, just
//      solid lines and solid polygon fills.
//
//    * The size of the PDF drawing data is limited to available
//      memory.  Drawings that are extremely large/complex can
//      fail if memory is insufficient. 
//
//    * Images are compressed to save space but page drawing data
//      is currently written in uncompressed form.  Consider
//      compressing the page data in a future version.
//--------------------------------------------------------------------

#pragma once
#include <vector>
#include <string>
#include <exception>
#include <stdio.h>
#include <memory>

namespace draw2pdf {

//--------------------------------------------------------------------
// The draw2pdf class throws an exception of this type if an error
// occurs.
//--------------------------------------------------------------------
class PDFException : public std::exception
{
public:
   std::wstring   m_srcFile;
   size_t         m_srcLine = 0;
   std::wstring   m_errorMessage;

   PDFException() = default;
   PDFException(const std::wstring &file, size_t line, const std::wstring &message) :
      m_srcFile(file), m_srcLine(line), m_errorMessage(message) { }
};

//--------------------------------------------------------------------
// Container to describe a point coordinate.
//--------------------------------------------------------------------
struct PDFPoint
{
   double   x = 0.;
   double   y = 0.;

   PDFPoint() = default;
   PDFPoint(const PDFPoint &copy) = default;
   PDFPoint(double xx, double yy) : x(xx), y(yy) { }
};

//--------------------------------------------------------------------
// Container to describe a rectangle.
//--------------------------------------------------------------------
struct PDFBox
{
   PDFPoint m_min;
   PDFPoint m_max;

   PDFBox() = default;
   PDFBox(const PDFBox &copy) = default;
   PDFBox(const PDFPoint &emin, const PDFPoint &emax) : m_min(emin), m_max(emax) { }

   void SetToDegenerate()
      { m_min.x = m_min.y = DBL_MAX; m_max.x = m_max.y = -DBL_MAX; }

   void SetToZero()
      { m_min.x = m_min.y = m_max.x = m_max.y = 0.; }

   void ExtendBy(const PDFPoint &pt)
   {
      if (pt.x < m_min.x)     m_min.x = pt.x;
      if (pt.y < m_min.y)     m_min.y = pt.y;
      if (pt.x > m_max.x)     m_max.x = pt.x;
      if (pt.y > m_max.y)     m_max.y = pt.y;
   }

   void ExtendBy(const std::vector<PDFPoint> &pts)
   {
      for (const auto &pt : pts)
         ExtendBy(pt);
   }

   double ExtentX() const { return fabs(m_max.x - m_min.x); }
   double ExtentY() const { return fabs(m_max.y - m_min.y); }

   bool IsDegenerate() const
      { return m_min.x > m_max.y || m_min.y > m_max.y; }

   void Normalize()
   {
      if (m_min.x > m_max.x)     std::swap(m_min.x, m_max.x);
      if (m_min.y > m_max.y)     std::swap(m_min.y, m_max.y);
   }
};

//--------------------------------------------------------------------
// Container to describe a color.
// The color saturation values are zero to one inclusive.
//--------------------------------------------------------------------
struct PDFColor
{
   double   m_red = 0.;
   double   m_green = 0.;
   double   m_blue = 0.;
   double   m_alpha = 1.;

   PDFColor() = default;
   PDFColor(double r, double g, double b, double a = 1.) :
      m_red(r), m_green(g), m_blue(b), m_alpha(a) { }
};

//--------------------------------------------------------------------
// Container to describe a line style.
// For now the only attributes that can be changed are the line color
// and line width.
// TODO:  Add support for dashed line styles.
//--------------------------------------------------------------------
struct PDFLineStyle
{
   enum LinePattern { LINE_SOLID=0, LINE_NULL=1 };

   LinePattern m_pattern = LINE_SOLID; // Kind of line to draw.
   PDFColor    m_color;                // Color of line to draw.
   double      m_width = 0.;           // Line width, in points (1 point = 1/72 inch).

   PDFLineStyle() = default;
   PDFLineStyle(const PDFColor &color, double width=0.) :
      m_pattern(LINE_SOLID), m_color(color), m_width(width) { }
   PDFLineStyle(LinePattern pattern, const PDFColor &color = PDFColor(), double width=0.) :
      m_pattern(pattern), m_color(color), m_width(width) { }
};

//--------------------------------------------------------------------
// Container to describe a fill style.
// For now the only attribute that can be changed is the solid fill
// color.
// TODO:  Add support for non-solid fill patterns.
//--------------------------------------------------------------------
struct PDFFillStyle
{
   enum FillPattern { FILL_SOLID=0, FILL_NULL=1 };

   FillPattern m_pattern = FILL_SOLID; // Kind of fill to draw.
   PDFColor    m_color;                // Color of fill to draw.

   PDFFillStyle() = default;
   explicit PDFFillStyle(const PDFColor &color) :
      m_pattern(FILL_SOLID), m_color(color) { }
   PDFFillStyle(FillPattern pattern, const PDFColor &color = PDFColor()) :
      m_pattern(pattern), m_color(color) { }
};

//--------------------------------------------------------------------
// Container to describe a text style.
// For now the only attributes that can be changed are the text color
// and height.
// TODO:  Add support for different fonts.
//--------------------------------------------------------------------
struct PDFTextStyle
{
   double m_height = 10.;  // Text height in points.
   PDFColor m_color;       // Text drawing color.

   PDFTextStyle() = default;
   PDFTextStyle(double height, const PDFColor &color) :
      m_height(height), m_color(color) { }
};

//--------------------------------------------------------------------
// Container to describe one cross reference in the PDF file.
// A list of these is used for generating the cross references table
// at the end of the PDF file.
//--------------------------------------------------------------------
struct PDFCrossRef
{
   size_t m_objnum = 0;    // The object number of the object to which this refers.
   size_t m_offset = 0;    // The offset of the object in the PDF file.

   PDFCrossRef() = default;
   PDFCrossRef(size_t objnum, size_t offset) : m_objnum(objnum), m_offset(offset) { }
};

//--------------------------------------------------------------------
// Container to describe one raster bitmap image to be written to the
// PDF file.
//--------------------------------------------------------------------
struct PDFImage
{
   size_t   m_numX = 0;    // Width of image, in pixels.
   size_t   m_numY = 0;    // Height of image, in pixels.
   size_t   m_bpp = 0;     // Number of bits per pixel in image data.
                           // Must be 8, 24, or 32.  8 is assumed to be grayscale.
   size_t   m_stride = 0;  // Number of bytes between the start of a given scanline
                           // and the next scanline in the image data.
   size_t   m_objNum = 0;  // The PDF object number of the image.  Used internally.

   std::vector<unsigned char> m_pixels;  // Image's pixel data, in the format described above.

   PDFImage() = default;
};

//--------------------------------------------------------------------
// Class to manage accumulating text or binary data into a buffer
// for later writing.  Currently the data is stored in memory.
// TODO:  Add an option to write the temporary stream data to disk
// instead of memory for extremely large PDF files.
//--------------------------------------------------------------------
class PDFStreamAccumulator
{
public:
   PDFStreamAccumulator() = default;
   PDFStreamAccumulator(const PDFStreamAccumulator &copy) = delete;
   ~PDFStreamAccumulator() = default;

   // Adds bytes of binary data to the stream.
   void AddData(const void *data, size_t numBytes);
   void AddData(const std::vector<unsigned char> &data) { AddData(data.data(), data.size()); }

   // Adds a text string to the stream.
   void AddData(const std::string &data) { AddData(data.c_str(), data.size()); }

   // Adds formatted text to the stream.
   // Formatting is the same as printf in the runtime library.
   void Printf(const char *format, ...);

   // Returns the number of bytes in the stream so far.
   size_t size() const { return m_data.size(); }

   // Returns a pointer to the stream data accumulated so far.
   const unsigned char *data() const { return m_data.data(); }

   // Discards any accumulated data.
   void clear() { m_data.clear(); }


   // Storage for the stream's accumulated data.
   std::vector<unsigned char> m_data;
};

//--------------------------------------------------------------------
// Class to draw simple vector graphics (lines and polygons) to an
// Adobe PDF file.
//--------------------------------------------------------------------
class Draw2pdf
{
public:
   Draw2pdf() = default;
   Draw2pdf(const Draw2pdf &copy) = delete;
   ~Draw2pdf();

   //---------------------------------------------------------------
   // Opens a new PDF file for writing.  Errors throw.
   // The dimensions of the page(s) in the PDF file should be given
   // in units of typesetting points (1 point = 1/72 inch).
   //---------------------------------------------------------------
   void Open(const std::wstring &filename,
            const PDFPoint &pageMinimumPoints,
            const PDFPoint &pageMaximumPoints);

   //---------------------------------------------------------------
   // Finishes writing the currently open PDF file.  Errors throw.
   //---------------------------------------------------------------
   void Close();

   //---------------------------------------------------------------
   // Sets the line style to be used for drawing any subsequent
   // graphics.
   //---------------------------------------------------------------
   void SetLineStyle(const PDFLineStyle &style);

   //---------------------------------------------------------------
   // Sets the fill style to be used for drawing any subsequent
   // graphics.
   //---------------------------------------------------------------
   void SetFillStyle(const PDFFillStyle &style);

   //---------------------------------------------------------------
   // Sets the text style to be used for drawing any subsequent
   // text strings.
   //---------------------------------------------------------------
   void SetTextStyle(const PDFTextStyle &style);

   //---------------------------------------------------------------
   // Draws a line between two points using the current line style.
   // The point coordinates are given in units of points.
   //---------------------------------------------------------------
   void DrawLine(const PDFPoint &pt1, const PDFPoint &pt2);

   //---------------------------------------------------------------
   // Draws a polyline using the current line style.
   // The point coordinates are given in units of points.
   //---------------------------------------------------------------
   void DrawPolyline(const std::vector<PDFPoint> &points);

   //---------------------------------------------------------------
   // Draws a (non-compound) polygon using the current line and
   // fill styles.
   // The point coordinates are given in units of points.
   //---------------------------------------------------------------
   void DrawPolygon(const std::vector<PDFPoint> &points);

   //---------------------------------------------------------------
   // Draws a rectangle using the current line and fill styles.
   // The point coordinates are given in units of points.
   //---------------------------------------------------------------
   void DrawRectangle(const PDFBox &box);

   //---------------------------------------------------------------
   // Draws a text string at the specified position on the page
   // (in points) using the current text style.
   //---------------------------------------------------------------
   void DrawTextString(const PDFPoint &point, const std::wstring &text);

   //---------------------------------------------------------------
   // Draws a bitmap (raster) image at the specified position and
   // size (in points) on the page.
   //---------------------------------------------------------------
   void DrawImage(
      const PDFImage &image,  // Image to be drawn.
      double destX,           // Where to draw left edge of image on page, in points.
      double destY,           // Where to draw top edge of image on page, in points.
      double destWidth,       // Width to draw image on page, in points.
      double destHeight       // Height to draw image on page, in points.
      );
   void DrawImage(
      const void *pixels,     // Pointer to the pixel data for the image.
      size_t      numX,       // Width of image, in pixels.
      size_t      numY,       // Height of image, in pixels.
      size_t      bpp,        // Number of bits per pixel in image data.
                              // Must be 8, 24, or 32.  8 is assumed to be grayscale.
      size_t      stride,     // Number of bytes from the start of one scanline to
                              // the next in the pixel data array.
      double      destX,      // Where to draw left edge of image on page, in points.
      double      destY,      // Where to draw top edge of image on page, in points.
      double      destWidth,  // Width to draw image on page, in points.
      double      destHeight  // Height to draw image on page, in points.
      );

   //---------------------------------------------------------------
   // Finishes the current page of the currently open PDF file and
   // prepares to start writing to the next page.  Errors throw.
   //---------------------------------------------------------------
   void NextPage();

   //---------------------------------------------------------------
   // Enable or disable compression of images in subsequent PDF
   // files.
   //---------------------------------------------------------------
   void EnableImageCompression(bool enable) { m_compressImages = enable; }

   //---------------------------------------------------------------
   // Enable or disable compression of page content stream data
   // in subsequent pages.
   //---------------------------------------------------------------
   void EnableContentCompression(bool enable) { m_compressContent = enable; }

private:
   void DoBeginPage();
   void DoEndPage();
   void DoWriteImage(size_t index, bool compress);

   // File stream to the PDF file currently being written.
   FILE * m_file = nullptr;

   // Extents of the page, in points.
   PDFPoint m_pageMinimumPoints;
   PDFPoint m_pageMaximumPoints;

   // The current drawing attributes are stored here.
   PDFLineStyle   m_lineStyle;
   PDFFillStyle   m_fillStyle;
   PDFTextStyle   m_textStyle;

   // List of cross reference information for the objects in the PDF file.
   // This is used to generate the cross reference table at the end of the PDF file.
   std::vector<PDFCrossRef> m_crossRefs;

   // Next available object number in the current PDF file.
   size_t m_objNumber = 1;

   // Object numbers reserved for certain objects in the current PDF file.
   size_t m_catalogObjNumber = 0;
   size_t m_pagesObjNumber = 0;
   size_t m_contentsObjNumber = 0;
   size_t m_xobjectObjNumber = 0;

   // List of PDF object numbers of each of the "Page" objects in the PDF file.
   std::vector<size_t> m_pageObjectNumbers;

   // Storage for the page's graphic content stream.
   PDFStreamAccumulator m_contentStream;

   // Storage for the data of any images that need to be written to the PDF file.
   std::vector<PDFImage> m_images;

   // True if images are compressed in the PDF file.
   bool m_compressImages = false;

   // True if page content streams are compressed in the PDF file.
   bool m_compressContent = false;
};

} // End namespace draw2pdf
