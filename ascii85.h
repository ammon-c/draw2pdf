//--------------------------------------------------------------------
// ascii85.h
// C++ code to encode binary data into ASCII-85 format.
// This encoding format is typically used inside Adobe PDF files.
//
// (C) Copyright 1996-2019 Ammon R. Campbell.
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
//
// References:
//    https://en.wikipedia.org/wiki/Ascii85
//
//--------------------------------------------------------------------

#pragma once
#include <stdlib.h>
#include <vector>

namespace {

const size_t lineWidth = 72;

//--------------------------------------------------------------------
// Class to help encode data into ASCII-85 format.
//--------------------------------------------------------------------
class Ascii85Encoder
{
public:
   std::vector<unsigned char> m_output;   // Storage for the encoded data.
   size_t m_column = 0;                   // Current column number in the output.
   size_t m_tuple = 0;                    // Temporary composite value.
   size_t m_count = 0;                    // Current position in tuple.

   Ascii85Encoder() = default;
   Ascii85Encoder(const Ascii85Encoder &copy) = delete;
   ~Ascii85Encoder() = default;

   //--------------------------------------------------------------------
   // Encodes the given data into ASCII-85 format.
   // The encoded data is returned as a vector of bytes.
   //--------------------------------------------------------------------
   const std::vector<unsigned char> & EncodeToAscii85(const void *data, size_t numBytes)
   {
      m_output.clear();
      m_column = 0;
      m_tuple = 0;
      m_count = 0;
      if (data == nullptr || numBytes < 1)
         return m_output;    // Nothing to encode.

      for (size_t offset = 0; offset < numBytes; ++offset)
         EncodeByte((reinterpret_cast<const unsigned char *>(data))[offset]);

      // Encode any partial remaining data.
      if (m_count > 0)
         EncodeTuple(static_cast<unsigned int>(m_tuple), static_cast<int>(m_count));

      // Terminate the last line.
      if (m_column + 2 > lineWidth)
      {
         m_output.push_back('\r');
         m_output.push_back('\n');
      }

      // Terminate the encoded data with "~>" and a newline.
      m_output.push_back('~');
      m_output.push_back('>');
      m_output.push_back('\r');
      m_output.push_back('\n');

      return m_output;
   }

private:
   //--------------------------------------------------------------------
   // Encodes the given tuple, appending the encoded data to m_output.
   //--------------------------------------------------------------------
   void EncodeTuple(
      unsigned int tuple,  // in:  The four byte data value to be encoded.
      int count            // in:  Number of bytes used in tuple (1..4).
      )
   {
      // Expand the tuple to 5 values of base-85 data.
      char buf[5] = {0};
      for (int index = 0; index < 5; ++index)
      {
         buf[index] = tuple % 85;
         tuple /= 85;
      }

      // Add the encoded bytes to the output buffer.
      for (int index = 0; index <= count; ++index)
      {
         char c = (buf[4 - index] + '!');
         m_output.push_back(static_cast<unsigned char>(c));
         if (m_column++ >= lineWidth)
         {
            m_column = 0;
            m_output.push_back('\r');
            m_output.push_back('\n');
         }
      }
   }
   
   //--------------------------------------------------------------------
   // Encodes the given byte, appending the encoded data to m_output.
   //--------------------------------------------------------------------
   void EncodeByte(unsigned char c)
   {
      switch (m_count++)
      {
         case 0:  m_tuple |= (static_cast<unsigned int>(c) << 24);   break;
         case 1:  m_tuple |= (static_cast<unsigned int>(c) << 16);   break;
         case 2:  m_tuple |= (static_cast<unsigned int>(c) << 8);    break;
         case 3:
            m_tuple |= c;
            if (m_tuple == 0)
            {
               m_output.push_back('z');
               if (m_column++ >= lineWidth)
               {
                  m_column = 0;
                  m_output.push_back('\r');
                  m_output.push_back('\n');
               }
            }
            else
            {
               EncodeTuple(static_cast<unsigned int>(m_tuple), static_cast<int>(m_count));
            }
            m_tuple = 0;
            m_count = 0;
            break;
      }
   }
};

} // End namespace
