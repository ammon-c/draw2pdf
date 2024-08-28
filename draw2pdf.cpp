//--------------------------------------------------------------------
// draw2pdf.cpp - Class for drawing simple vector graphics (lines and
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
// IMPLEMENTATION NOTES:
//    * PDF file format don't recognize exponential notation, so "%lf"
//      is preferred to "%lg" when writing floating-point numbers
//      with printf style formatters.
//--------------------------------------------------------------------

#include "draw2pdf.h"
#include "ascii85.h"
#include "Zlib.h"
#include <time.h>
#include <algorithm>
#include <cstdarg>

namespace {

//---------------------------------------------------------------
// Compresses the given data with ZLIB's deflate compression.
// The compressed data is returned.  No data is returned if
// ZLIB is unable to compress the given data.
//---------------------------------------------------------------
std::vector<unsigned char> DeflateData(const void *data, size_t numBytes)
{
   uLongf numCompressedBytes = static_cast<uLongf>(numBytes + 8192);
   std::vector<unsigned char> output(numCompressedBytes);

   int errcode = compress(output.data(), &numCompressedBytes,
                          reinterpret_cast<const Bytef *>(data),
                          static_cast<uLong>(numBytes));
   if (errcode != Z_OK)
      numCompressedBytes = 0;

   output.resize(numCompressedBytes);

   return output;
}

//--------------------------------------------------------------------
// Convert a wide string to a narrow string by simple casting.
// Note this is only compatible with 8-bit US/ANSI characters.
// Does not work with wide/Unicode characters.
//--------------------------------------------------------------------
std::string WideToNarrow(const std::wstring &w)
{
   std::string n;
   for (const auto chr : w)
      n += static_cast<char>(chr);
   return n;
}

} // End anon namespace

namespace draw2pdf {

//---------------------------------------------------------------
// Adds the given bytes of binary data to the stream.
//---------------------------------------------------------------
void PDFStreamAccumulator::AddData(const void *data, size_t numBytes)
{
   const unsigned char *ucdata = reinterpret_cast<const unsigned char *>(data);
   m_data.insert(m_data.end(), ucdata, ucdata + numBytes);
}

//---------------------------------------------------------------
// Adds formatted text to the stream.
// Formatting is the same as printf in the runtime library.
//---------------------------------------------------------------
void PDFStreamAccumulator::Printf(const char *format, ...)
{
   std::va_list args;
   va_start(args, format);
   std::string buffer;
   for (size_t bufferSize = 32; bufferSize < 16384; bufferSize *= 2)
   {
      std::vector<char> tmp(bufferSize);
      if (_vsnprintf_s(tmp.data(), bufferSize, _TRUNCATE, format, args) != -1)
      {
         buffer = tmp.data();
         break;
      }
   }
   va_end(args);
   AddData(buffer.c_str(), buffer.size());
}

//---------------------------------------------------------------
Draw2pdf::~Draw2pdf()
{
   Close();
}

//---------------------------------------------------------------
// Opens a new PDF file for writing.  Errors throw.
// The dimensions of the page(s) in the PDF file should be given
// in units of typesetting points (1 point = 1/72 inch).
//---------------------------------------------------------------
void Draw2pdf::Open(const std::wstring &filename,
         const PDFPoint &pageMinimumPoints,
         const PDFPoint &pageMaximumPoints)
{
   Close();
   m_pageMinimumPoints = pageMinimumPoints;
   m_pageMaximumPoints = pageMaximumPoints;

   if (_wfopen_s(&m_file, filename.c_str(), L"wb") || m_file == nullptr)
      throw PDFException(__FILEW__, __LINE__,
               std::wstring(L"Failed opening file for writing:  ") + filename);

   // Write the PDF file signature to the beginning of the file.
   fprintf(m_file, "%%PDF-1.4\r\n");
   fprintf(m_file, "%%\xC0\xE1\xD2\xC3\xB4\r\n");
   fprintf(m_file, "%%PDF file generated by draw2pdf.lib\r\n");

   // Write the first object in the PDF file, the catalog object.
   m_catalogObjNumber = m_objNumber++;
   m_pagesObjNumber = m_objNumber++;
   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(m_catalogObjNumber, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", m_catalogObjNumber);
   fprintf(m_file, "<<\r\n");
   fprintf(m_file, "/Type /Catalog\r\n");
   fprintf(m_file, "/Pages %zu 0 R\r\n", m_pagesObjNumber);
   fprintf(m_file, ">>\r\n");
   fprintf(m_file, "endobj\r\n");

   DoBeginPage();
}

//---------------------------------------------------------------
// Finishes writing the currently open PDF file.  Errors throw.
//---------------------------------------------------------------
void Draw2pdf::Close()
{
   if (!m_file)
      return;

   DoEndPage();

   // Write the "Pages" object with a list of child pages.
   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(m_pagesObjNumber, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", m_pagesObjNumber);
   fprintf(m_file, "<<\r\n");
   fprintf(m_file, "/Type /Pages /Kids [");
   for (const size_t objnum : m_pageObjectNumbers)
      fprintf(m_file, "%zu 0 R ", objnum);
   fprintf(m_file, "]\r\n");
   fprintf(m_file, "/Count %zu\r\n", m_pageObjectNumbers.size());
   fprintf(m_file, ">>\r\n");
   fprintf(m_file, "endobj\r\n");

   // The entries in the cross reference table must be written in
   // object-number order, so sort the table by object number before
   // we write it.
   std::sort(m_crossRefs.begin(), m_crossRefs.end(),
      [](PDFCrossRef a, PDFCrossRef b){ return a.m_objnum < b.m_objnum; });

   // Write the cross reference table.
   fprintf(m_file, "\r\n");
   long xrefTableOffset = ftell(m_file);
   fprintf(m_file, "xref\r\n");
   fprintf(m_file, "0 %zu\r\n", m_crossRefs.size() + 1); // First line indicates count of entries in table.
   fprintf(m_file, "0000000000 65535 f\r\n");            // Required dummy first entry.
   for (const auto &xref : m_crossRefs)
      fprintf(m_file, "%010zu 00000 n\r\n", xref.m_offset);

   // Write the trailer section, which indicates the xref table size and root
   // object number in the file.  Assumes the root object is object #1.
   fprintf(m_file, "trailer\r\n");
   fprintf(m_file, "<< \r\n");
   time_t tt = {0};
   int id = static_cast<int>(time(&tt)) + rand();
   fprintf(m_file, "/ID[<%032d><%032d>]\r\n", id, id);
   fprintf(m_file, "/Size %zu /Root 1 0 R >>\r\n", m_crossRefs.size() + 1);

   // Write the "startxref" keyword followed by the offset of the cross reference
   // table in the PDF file.  PDF reader applications use this to find the cross
   // reference table.
   fprintf(m_file, "startxref\r\n");
   fprintf(m_file, "%ld\r\n", xrefTableOffset);

   // Lastly, write the PDF's EOF marker.
   fprintf(m_file, "%%%%EOF\r\n");

   // We're done with the file now.
   fclose(m_file);
   m_file = nullptr;

   // Reset members to default state for next PDF file.
   m_lineStyle = PDFLineStyle();
   m_fillStyle = PDFFillStyle();
   m_textStyle = PDFTextStyle();
   m_crossRefs.clear();
   m_objNumber = 1;
   m_catalogObjNumber = 0;
   m_pagesObjNumber = 0;
   m_pageObjectNumbers.clear();
   m_contentStream.clear();
   m_images.clear();
}

//---------------------------------------------------------------
// Sets the line style to be used for drawing any subsequent
// graphics.
//---------------------------------------------------------------
void Draw2pdf::SetLineStyle(const PDFLineStyle &style)
{
   m_lineStyle = style;

   // Set the line color.
   m_contentStream.Printf("%lf %lf %lf RG\r\n",
      m_lineStyle.m_color.m_red,
      m_lineStyle.m_color.m_green,
      m_lineStyle.m_color.m_blue);

   // Set the line width.
   m_contentStream.Printf("%lf w\r\n", m_lineStyle.m_width);
}

//---------------------------------------------------------------
// Sets the fill style to be used for drawing any subsequent
// graphics.
//---------------------------------------------------------------
void Draw2pdf::SetFillStyle(const PDFFillStyle &style)
{
   m_fillStyle = style;

   // Set the fill color.
   m_contentStream.Printf("%lf %lf %lf rg\r\n",
      m_fillStyle.m_color.m_red,
      m_fillStyle.m_color.m_green,
      m_fillStyle.m_color.m_blue);
}

//---------------------------------------------------------------
// Sets the text style to be used for drawing any subsequent
// text strings.
//---------------------------------------------------------------
void Draw2pdf::SetTextStyle(const PDFTextStyle &style)
{
   m_textStyle = style;
}

//---------------------------------------------------------------
// Draws a line between two points using the current line style.
// The point coordinates are given in units of points.
//---------------------------------------------------------------
void Draw2pdf::DrawLine(const PDFPoint &pt1, const PDFPoint &pt2)
{
   // Draw the single line as a polyline.
   std::vector<PDFPoint> points;
   points.push_back(pt1);
   points.push_back(pt2);
   DrawPolyline(points);
}

//---------------------------------------------------------------
// Draws a polyline using the current line style.
// The point coordinates are given in units of points.
//---------------------------------------------------------------
void Draw2pdf::DrawPolyline(const std::vector<PDFPoint> &points)
{
   if (m_lineStyle.m_pattern == PDFLineStyle::LINE_NULL)
      return;

   // Output the polyline as a moveto (m) followed by a sequence
   // of lineto (l) operations.
   bool first = true;
   for (const auto &point : points)
   {
      m_contentStream.Printf("%lf %lf %c\r\n", point.x, point.y, first ? 'm' : 'l');
      first = false;
   }

   // Stroke the polyline.
   m_contentStream.Printf("S\r\n");
}

//---------------------------------------------------------------
// Draws a (non-compound) polygon using the current line and
// fill styles.
// The point coordinates are given in units of points.
//---------------------------------------------------------------
void Draw2pdf::DrawPolygon(const std::vector<PDFPoint> &points)
{
   if (m_lineStyle.m_pattern == PDFLineStyle::LINE_NULL &&
       m_fillStyle.m_pattern == PDFFillStyle::FILL_NULL)
   {
      return;
   }

   // Output the polygon as a moveto (m) followed by a sequence
   // of lineto (l) operations.
   bool first = true;
   for (const auto &point : points)
   {
      m_contentStream.Printf("%lf %lf %c\r\n", point.x, point.y, first ? 'm' : 'l');
      first = false;
   }

   // Close the polygon's path.
   m_contentStream.Printf("h\r\n");

   // Stroke and/or fill the polygon.
   if (m_lineStyle.m_pattern == PDFLineStyle::LINE_SOLID &&
       m_fillStyle.m_pattern == PDFFillStyle::FILL_SOLID)
   {
      // Stroke and fill the path.
      // Note that removing the '*' would change the polygon filling rule
      // from even-odd fill to winding fill.
      m_contentStream.Printf("B*\r\n");
   }
   else if (m_fillStyle.m_pattern == PDFFillStyle::FILL_SOLID)
   {
      // Fill the path without stroking.
      m_contentStream.Printf("f*\r\n");
   }
   else if (m_lineStyle.m_pattern == PDFLineStyle::LINE_SOLID)
   {
      // Stroke the path without filling.
      m_contentStream.Printf("S\r\n");
   }
}

//---------------------------------------------------------------
// Draws a rectangle using the current line and fill styles.
// The point coordinates are given in units of points.
//---------------------------------------------------------------
void Draw2pdf::DrawRectangle(const PDFBox &box)
{
   std::vector<PDFPoint> points;
   points.push_back(PDFPoint(box.m_min.x, box.m_min.y));
   points.push_back(PDFPoint(box.m_max.x, box.m_min.y));
   points.push_back(PDFPoint(box.m_max.x, box.m_max.y));
   points.push_back(PDFPoint(box.m_min.x, box.m_max.y));

   DrawPolygon(points);
}

//---------------------------------------------------------------
// Draws a text string at the specified position on the page
// (in points) using the current text style.
//---------------------------------------------------------------
void Draw2pdf::DrawTextString(const PDFPoint &point, const std::wstring &text)
{
   // TODO:  Add support for Unicode characters.  Currently assumes 8-bit US/English.

   // TODO:  PDF doesn't like certain characters inside text strings.
   //        Filter/re-encode any that aren't acceptable.

   // TODO:  Doesn't currently support fonts.  Text is shown with default font.

   m_contentStream.Printf("q\r\n");                                  // Push state.
   m_contentStream.Printf("BT\r\n");
   m_contentStream.Printf("/F1 %lf Tf\r\n", m_textStyle.m_height);   // Set font size.
   m_contentStream.Printf("%lf %lf %lf rg\r\n",
      m_textStyle.m_color.m_red,
      m_textStyle.m_color.m_green,
      m_textStyle.m_color.m_blue);

   const std::string text2 = WideToNarrow(text);

   m_contentStream.Printf("%lf %lf Td\r\n", point.x, point.y);       // Set text position.
   m_contentStream.Printf("(%s) Tj\r\n", text2.c_str());             // Set text string.
   m_contentStream.Printf("ET\r\n");
   m_contentStream.Printf("Q\r\n");                                  // Pop state.
}

//---------------------------------------------------------------
// Draws a bitmap (raster) image at the specified position and
// size (in points) on the page.
//---------------------------------------------------------------
void Draw2pdf::DrawImage(
   const PDFImage &image,  // Image to be drawn.
   double destX,           // Where to draw left edge of image on page, in points.
   double destY,           // Where to draw top edge of image on page, in points.
   double destWidth,       // Width to draw image on page, in points.
   double destHeight       // Height to draw image on page, in points.
   )
{
   // Store the image data to be written later (when the XObjects are written
   // to the PDF file).
   m_images.push_back(image);

   // Reserve a PDF object number for this image.
   m_images[m_images.size() - 1].m_objNum = m_objNumber++;

   m_contentStream.Printf("q\r\n");    // Push state.

   // Set the transform matrix for the image.
   // PDF uses six coefficients, in this order:
   //    A     scaleX
   //    B     skewX
   //    C     skewY
   //    D     scaleY
   //    E     offsetX
   //    F     offsetY
   //
   // TODO:2  The image may need to be y-flipped???
   double scaleX = destWidth;
   double scaleY = destHeight;
   double offsetX = destX;
   double offsetY = destY;

   m_contentStream.Printf("%lf %lf %lf %lf %lf %lf cm\r\n",
      scaleX, 0., 0., scaleY, offsetX, offsetY);

   // Indicate which XObject will contain the data for this image.
   m_contentStream.Printf("/Im%zu Do\r\n", m_images.size() - 1);

   m_contentStream.Printf("Q\r\n");    // Pop state.
}

//---------------------------------------------------------------
// Draws a bitmap (raster) image at the specified position and
// size (in points) on the page.
//---------------------------------------------------------------
void Draw2pdf::DrawImage(
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
   )
{
   PDFImage image;
   image.m_numX   = numX;
   image.m_numY   = numY;
   image.m_bpp    = bpp;
   image.m_stride = stride;
   image.m_pixels.resize(numY * stride);
   memcpy(image.m_pixels.data(), pixels, numY * stride);
   DrawImage(image, destX, destY, destWidth, destHeight);
}

//---------------------------------------------------------------
// Writes a previously stored image to the PDF file.
// The index indicates which element of m_images[] is to be
// written.
//---------------------------------------------------------------
void Draw2pdf::DoWriteImage(size_t index, bool compress)
{
   const PDFImage &image = m_images[index];

   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(image.m_objNum, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", image.m_objNum);
   fprintf(m_file, "<<\r\n");
   fprintf(m_file, "/Type /XObject\r\n");
   fprintf(m_file, "/Subtype /Image\r\n");
   fprintf(m_file, "/Name /Im%zu\r\n", index);
   fprintf(m_file, "/Width %zu\r\n", image.m_numX);
   fprintf(m_file, "/Height %zu\r\n", image.m_numY);
   fprintf(m_file, "/BitsPerComponent 8\r\n");
   if (image.m_bpp == 8)
      fprintf(m_file, "/ColorSpace /DeviceGray\r\n");
   else
      fprintf(m_file, "/ColorSpace /DeviceRGB\r\n");

   // Pack the image pixel data so there's no padding between scanlines.
   // If image is 32 bits, the alpha byte of each pixel must also be removed.
   std::vector<unsigned char> rawData(image.m_numY * image.m_numX * image.m_bpp / 8);
   size_t outChannels = (image.m_bpp == 8 ? 1 : 3);
   for (size_t y = 0; y < image.m_numY; ++y)
   {
      const unsigned char *inpixel = &image.m_pixels[y * image.m_stride];
      unsigned char *outpixel = &rawData[y * image.m_numX * image.m_bpp / 8];
      for (size_t x = 0; x < image.m_numX; ++x)
      {
         for (size_t channel = 0; channel < outChannels; ++channel)
            *outpixel++ = *inpixel++;
         if (image.m_bpp == 32)
            ++inpixel;  // Skip the alpha byte.
      }
   }

   // Encode the image data.
   std::vector<unsigned char> encodedData;
   if (compress)
   {
      encodedData = DeflateData(rawData.data(), rawData.size());
      fprintf(m_file, "/Filter /FlateDecode\r\n");
      fprintf(m_file, "/Length %zu\r\n", encodedData.size());
   }
   else
   {
      Ascii85Encoder a85;
      encodedData = a85.EncodeToAscii85(rawData.data(), rawData.size());
      fprintf(m_file, "/Filter /ASCII85Decode\r\n");
      fprintf(m_file, "/Length %zu\r\n", encodedData.size());
   }
   fprintf(m_file, ">>\r\n");

   fprintf(m_file, "stream\r\n");
   fwrite(encodedData.data(), 1, encodedData.size(), m_file);
   fprintf(m_file, "\r\n");
   fprintf(m_file, "endstream\r\n");
   fprintf(m_file, "endobj\r\n");
}

//---------------------------------------------------------------
// Finishes the current page of the currently open PDF file and
// prepares to start writing to the next page.  Errors throw.
//---------------------------------------------------------------
void Draw2pdf::NextPage()
{
   DoEndPage();
   DoBeginPage();
}

//---------------------------------------------------------------
// Performs any actions that need to be done once at the start
// of each page of the PDF file.
//---------------------------------------------------------------
void Draw2pdf::DoBeginPage()
{
   size_t pageObjNumber = m_objNumber++;
   m_pageObjectNumbers.push_back(pageObjNumber);
   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(pageObjNumber, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", pageObjNumber);
   fprintf(m_file, "<<\r\n");
   fprintf(m_file, "/Type /Page\r\n");
   fprintf(m_file, "/Parent %zu 0 R\r\n", m_pagesObjNumber);

   fprintf(m_file, "/MediaBox [ %lf %lf %lf %lf ]\r\n",
      m_pageMinimumPoints.x, m_pageMinimumPoints.y,
      m_pageMaximumPoints.x, m_pageMaximumPoints.y);

   m_contentsObjNumber = m_objNumber++;
   fprintf(m_file, "/Contents %zu 0 R\r\n", m_contentsObjNumber);

   fprintf(m_file, "/Resources\r\n");
   fprintf(m_file, "<<\r\n");
   fprintf(m_file, "/ProcSet [ /PDF /Text /ImageB /ImageC /ImageI ]\r\n");
   m_xobjectObjNumber = m_objNumber++;
   fprintf(m_file, "/XObject %zu 0 R\r\n", m_xobjectObjNumber);
   fprintf(m_file, ">>\r\n");

   fprintf(m_file, ">>\r\n");
   fprintf(m_file, "endobj\r\n");
}

//---------------------------------------------------------------
// Performs any actions that need to be done once at the end of
// each page of the PDF file.
//---------------------------------------------------------------
void Draw2pdf::DoEndPage()
{
   // Write the graphics content stream.
   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(m_contentsObjNumber, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", m_contentsObjNumber);
   fprintf(m_file, "<<\r\n");

   if (!m_compressContent)
   {
      fprintf(m_file, "/Length %zu\r\n", m_contentStream.size());
      fprintf(m_file, ">>\r\n");
      fprintf(m_file, "stream\r\n");
      fwrite(m_contentStream.data(), 1, m_contentStream.size(), m_file);
      fprintf(m_file, "\r\n");
      fprintf(m_file, "endstream\r\n");
      fprintf(m_file, "endobj\r\n");
   }
   else
   {
      std::vector<unsigned char> encodedData;
      encodedData = DeflateData(m_contentStream.data(), m_contentStream.size());
      fprintf(m_file, "/Filter /FlateDecode\r\n");
      fprintf(m_file, "/Length %zu\r\n", encodedData.size());
      fprintf(m_file, ">>\r\n");

      fprintf(m_file, "stream\r\n");
      fwrite(encodedData.data(), 1, encodedData.size(), m_file);
      fprintf(m_file, "\r\n");
      fprintf(m_file, "endstream\r\n");
      fprintf(m_file, "endobj\r\n");
   }

   // Write the object containing the XObjects table.
   fprintf(m_file, "\r\n");
   m_crossRefs.push_back(PDFCrossRef(m_xobjectObjNumber, static_cast<size_t>(ftell(m_file))));
   fprintf(m_file, "%zu 0 obj\r\n", m_xobjectObjNumber);
   fprintf(m_file, "<<\r\n");
   for (size_t index = 0; index < m_images.size(); ++index)
      fprintf(m_file, "/Im%zu %zu 0 R\r\n", index, m_images[index].m_objNum);
   fprintf(m_file, ">>\r\n");
   fprintf(m_file, "endobj\r\n");

   // Write the objects that contain the image pixel data.
   for (size_t index = 0; index < m_images.size(); ++index)
      DoWriteImage(index, m_compressImages);

   // Prepare for next page, if any.
   m_contentStream.clear();
   m_images.clear();
}

} // End namespace draw2pdf
