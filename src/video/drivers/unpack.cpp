/* This file is part of the Pangolin Project.
 * http://github.com/stevenlovegrove/Pangolin
 *
 * Copyright (c) 2014 Steven Lovegrove
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <pangolin/video/drivers/unpack.h>

namespace pangolin
{

UnpackVideo::UnpackVideo(VideoInterface* src, VideoPixelFormat out_fmt)
    : size_bytes(0), buffer(0)
{
    if(!src) {
        throw VideoException("UnpackVideo: VideoInterface in must not be null");
    }

    if(out_fmt.bpp != 16 || out_fmt.channels != 1) {
        throw VideoException("UnpackVideo: Only supports 16bit output.");
    }

    videoin.push_back(src);

    for(size_t s=0; s< src->Streams().size(); ++s) {
        const size_t w = src->Streams()[s].Width();
        const size_t h = src->Streams()[s].Height();

        // Check compatibility of formats
        const VideoPixelFormat in_fmt = src->Streams()[s].PixFormat();
        if(in_fmt.channels > 1 || in_fmt.bpp <9 || in_fmt.bpp > 16) {
            throw VideoException("UnpackVideo: currently only supports one channel input.");
        }

        const size_t pitch = (w*out_fmt.bpp)/ 8;
        streams.push_back(pangolin::StreamInfo( out_fmt, w, h, pitch, (unsigned char*)0 + size_bytes ));
        size_bytes += h*pitch;
    }

    buffer = new unsigned char[src->SizeBytes()];
}

UnpackVideo::~UnpackVideo()
{
    delete[] buffer;
    delete videoin[0];
}

//! Implement VideoInput::Start()
void UnpackVideo::Start()
{
    videoin[0]->Start();
}

//! Implement VideoInput::Stop()
void UnpackVideo::Stop()
{
    videoin[0]->Stop();
}

//! Implement VideoInput::SizeBytes()
size_t UnpackVideo::SizeBytes() const
{
    return size_bytes;
}

//! Implement VideoInput::Streams()
const std::vector<StreamInfo>& UnpackVideo::Streams() const
{
    return streams;
}

void Convert10to16(
    Image<unsigned char>& out,
    const Image<unsigned char>& in
) {
    for(size_t r=0; r<out.h; ++r) {
        uint16_t* pout = (uint16_t*)(out.ptr + r*out.pitch);
        uint8_t* pin = in.ptr + r*in.pitch;
        const uint8_t* pin_end = in.ptr + (r+1)*in.pitch;
        while(pin != pin_end) {
            uint64_t val = *(pin++);
            val |= uint64_t(*(pin++)) << 8;
            val |= uint64_t(*(pin++)) << 16;
            val |= uint64_t(*(pin++)) << 24;
            val |= uint64_t(*(pin++)) << 32;
            *(pout++) = (val & 0x00000003FF);
            *(pout++) = (val & 0x00000FFC00) >> 10;
            *(pout++) = (val & 0x003FF00000) >> 20;
            *(pout++) = (val & 0xFFC0000000) >> 30;
        }
    }
}

void Convert12to16(
    Image<unsigned char>& out,
    const Image<unsigned char>& in
) {
    for(size_t r=0; r<out.h; ++r) {
        uint16_t* pout = (uint16_t*)(out.ptr + r*out.pitch);
        uint8_t* pin = in.ptr + r*in.pitch;
        const uint8_t* pin_end = in.ptr + (r+1)*in.pitch;
        while(pin != pin_end) {
            uint32_t val = *(pin++);
            val |= uint32_t(*(pin++)) << 8;
            val |= uint32_t(*(pin++)) << 16;
            *(pout++) = (val & 0x000FFF);
            *(pout++) = (val & 0xFFF000) >> 12;
        }
    }
}

//! Implement VideoInput::GrabNext()
bool UnpackVideo::GrabNext( unsigned char* image, bool wait )
{    
    if(videoin[0]->GrabNext(buffer,wait)) {
        for(size_t s=0; s<streams.size(); ++s) {
            Image<unsigned char> img_in  = videoin[0]->Streams()[s].StreamImage(buffer);
            Image<unsigned char> img_out = Streams()[s].StreamImage(image);

            const int bits = videoin[0]->Streams()[s].PixFormat().bpp;
            if( bits == 10) {
                Convert10to16(img_out, img_in);
            }else if( bits == 12){
                Convert12to16(img_out, img_in);
            }else{
                throw pangolin::VideoException("Incorrect image bit depth: " + bits);
            }
        }
        return true;
    }else{
        return false;
    }
}

//! Implement VideoInput::GrabNewest()
bool UnpackVideo::GrabNewest( unsigned char* image, bool wait )
{
    return GrabNext(image,wait);
}

std::vector<VideoInterface*>& UnpackVideo::InputStreams()
{
    return videoin;
}


}
