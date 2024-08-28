//--------------------------------------------------------------------
// pdftest.cpp
// A simple program to test the draw2pdf module.  When executed, this
// program writes a selection of simple line and polygon graphics to
// a PDF file named "test.pdf" in the current working directory. 
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

#include "draw2pdf.h"

using namespace draw2pdf;

namespace {

const wchar_t *outFilename = L"test.pdf";
const double pageWidth = 612.;
const double pageHeight = 792.;

void DrawCaption(Draw2pdf &writer, double x, double y, const wchar_t *text)
{
   // Draw a caption string in small black text.

   PDFTextStyle textStyle(10., PDFColor(0., 0., 0.));
   writer.SetTextStyle(textStyle);

   writer.DrawTextString(PDFPoint(x, y), text);
}

void DrawCornerMarks(Draw2pdf &writer)
{
   // Draw four short diagonal gray lines in the corners of the page.

   PDFLineStyle lineStyle(PDFColor(0.2, 0.2, 0.2));
   writer.SetLineStyle(lineStyle);

   writer.DrawLine(PDFPoint(0., pageHeight), PDFPoint(50., pageHeight - 50.));
   writer.DrawLine(PDFPoint(pageWidth, pageHeight), PDFPoint(pageWidth - 50., pageHeight - 50.));
   writer.DrawLine(PDFPoint(0., 0.), PDFPoint(50., 50.));
   writer.DrawLine(PDFPoint(pageWidth, 0.), PDFPoint(pageWidth - 50., 50.));
}

void TestDrawingLines(Draw2pdf &writer)
{
   // Draw several straight lines of increasing width in different colors.

   for (int i = 0; i < 10; i++)
   {
      PDFLineStyle lineStyle(PDFColor(1. - (i * 0.1), 0., (i * 0.1)), (i * 0.5));
      writer.SetLineStyle(lineStyle);

      writer.DrawLine(PDFPoint(100., 450. - (i * 10.)), PDFPoint(200. + (i * 5.), 470. - (i * 10.)));
   }

   DrawCaption(writer, 100., 470., L"Lines");
}

void TestDrawingPolyline(Draw2pdf &writer)
{
   // Draw a light blue polyline.

   PDFLineStyle lineStyle2(PDFColor(0., 0., 0.8), 2.);
   writer.SetLineStyle(lineStyle2);

   std::vector<PDFPoint> points;
   points.push_back(PDFPoint(250., 450.));
   points.push_back(PDFPoint(350., 450.));
   points.push_back(PDFPoint(280., 440.));
   points.push_back(PDFPoint(300., 375.));
   points.push_back(PDFPoint(260., 440.));
   writer.DrawPolyline(points);

   DrawCaption(writer, 250., 470., L"Polyline");
}

void TestDrawingPolygon(Draw2pdf &writer)
{
   // Draw a polygon with a fat red outline and green fill.

   PDFLineStyle lineStyle3(PDFColor(0.8, 0., 0.), 4.);
   writer.SetLineStyle(lineStyle3);
   PDFFillStyle fillStyle1(PDFColor(0., 0.8, 0.));
   writer.SetFillStyle(fillStyle1);

   std::vector<PDFPoint> points;
   points.push_back(PDFPoint(350. + 50., 450.));
   points.push_back(PDFPoint(450. + 50., 450.));
   points.push_back(PDFPoint(380. + 50., 440.));
   points.push_back(PDFPoint(400. + 50., 375.));
   writer.DrawPolygon(points);

   DrawCaption(writer, 400., 470., L"Polygon");
}

void TestDrawingImage_8Bit(Draw2pdf &writer)
{
   // 4 x 6 pixels, 8-bits per pixel.
   const unsigned char pix[] =
   {
      0x40, 0x40, 0x40, 0x40,
      0x50, 0x00, 0x00, 0x50, 
      0x60, 0x77, 0x77, 0x60, 
      0x60, 0xCC, 0xCC, 0x60, 
      0x70, 0x00, 0x00, 0x70, 
      0x80, 0x80, 0x80, 0x80
   };

   writer.DrawImage(pix, 4, 6, 8, 4, 250., 150., 100., 150.);

   DrawCaption(writer, 250., 310., L"8-bit Image (4 x 6 px)");
}

void TestDrawingImage_24Bit(Draw2pdf &writer)
{
   // 4 x 6 pixels, 24-bits per pixel.
   const unsigned char pix[] =
   {
      0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x00, 
      0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00,  
      0x60, 0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x77, 0x60, 0x00, 0x00,  
      0x60, 0x00, 0x00, 0x00, 0x00, 0xCC, 0x00, 0x00, 0xCC, 0x60, 0x00, 0x00,  
      0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00,  
      0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00
   };

   writer.DrawImage(pix, 4, 6, 24, 12, 400., 150., 100., 150.);

   DrawCaption(writer, 400., 310., L"24-bit Image (4 x 6 px)");
}

void TestDrawingText(Draw2pdf &writer)
{
   // Draw several lines of text in increasing sizes and different colors.

   for (int i = 0; i < 10; i++)
   {
      PDFTextStyle textStyle(10. + i, PDFColor(0., 1. - (i * 0.1), (i * 0.1)));
      writer.SetTextStyle(textStyle);

      writer.DrawTextString(PDFPoint(100., 290. - (i * 15.)), L"Testing 123.");
   }

   DrawCaption(writer, 100., 310., L"Text");
}

void TestDrawingBigText(Draw2pdf &writer)
{
   // Draw large text near top of page, twice to simulate a shadow.

   PDFTextStyle textStyleHuge(75., PDFColor(0.3, 0.3, 0.3));
   writer.SetTextStyle(textStyleHuge);
   writer.DrawTextString(PDFPoint(100., pageHeight - 200.), L"* draw2pdf *");

   PDFTextStyle textStyleHuge2(75., PDFColor(0.3, 0.7, 0.3));
   writer.SetTextStyle(textStyleHuge2);
   writer.DrawTextString(PDFPoint(103, pageHeight - 203.), L"* draw2pdf *");
}

void TestDrawingPage2Text(Draw2pdf &writer)
{
   PDFTextStyle textStyle(30., PDFColor(0.8, 0.3, 0.6));
   writer.SetTextStyle(textStyle);
   writer.DrawTextString(PDFPoint(150., 500.), L"This is the second page.");
}

void TestDrawingRectangles(Draw2pdf &writer)
{
   // Draw several rectangles of different sizes and colors.
   for (int i = 0; i < 10; i++)
   {
      PDFLineStyle lineStyle(PDFColor(1. - (i * 0.1), 0., (i * 0.1)), (i * 0.5));
      writer.SetLineStyle(lineStyle);
      PDFFillStyle fillStyle(PDFColor(0.4, 1. - (i * 0.1), (i * 0.1)));
      writer.SetFillStyle(fillStyle);

      writer.DrawRectangle(PDFBox(
         PDFPoint(175. + (i * 10.),  200. + (i * 10.)),
         PDFPoint(475. + (-i * 10.), 300. + (i * 15.))
      ));
   }

   DrawCaption(writer, 300., 450., L"Rectangles");
}

} // End anon namespace

//--------------------------------------------------------------------
// Application entry point.
// Returns EXIT_SUCCESS if no errors.
//--------------------------------------------------------------------
int main()
{
   try
   {
      wprintf(L"Creating '%s'\n", outFilename);
      Draw2pdf writer;
/*
      writer.EnableImageCompression(true);
      writer.EnableContentCompression(true);
*/
      writer.Open(outFilename,
         PDFPoint(0., 0.),
         PDFPoint(pageWidth, pageHeight));

      // Draw some test shapes on the first page.
      DrawCornerMarks(writer);
      TestDrawingLines(writer);
      TestDrawingPolyline(writer);
      TestDrawingPolygon(writer);
      TestDrawingImage_8Bit(writer);
      TestDrawingImage_24Bit(writer);
      TestDrawingText(writer);
      TestDrawingBigText(writer);

      writer.NextPage();

      // Draw some test shapes on the second page.
      DrawCornerMarks(writer);
      TestDrawingRectangles(writer);
      TestDrawingPage2Text(writer);

      wprintf(L"Closing '%s'\n", outFilename);
      writer.Close();
   }
   catch(const PDFException &exc)
   {
      wprintf(L"Exception:  %s(%zu):  %s\n",
         exc.m_srcFile.c_str(), exc.m_srcLine, exc.m_errorMessage.c_str());
      return EXIT_FAILURE;
   }
   catch(...)
   {
      wprintf(L"Aborted by unhandled exception!\n");
      return EXIT_FAILURE;
   }

   wprintf(L"Completed.\n");
   return EXIT_SUCCESS;
}

