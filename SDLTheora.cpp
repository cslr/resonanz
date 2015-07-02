/*
 * SDLTheora.cpp
 *
 *  Created on: 30.6.2015
 *      Author: Tomas
 */

#include "SDLTheora.h"
#include <string.h>

#include <ogg/ogg.h>
#include <math.h>

#include <chrono>
#include <thread>

#include "Log.h"


namespace whiteice {
namespace resonanz {

SDLTheora::SDLTheora(float q) :
		FPS(5), MSECS_PER_FRAME(1000/5) // currently only saves at 5 frames per second
{
	if(q >= 0.0f && q <= 1.0f)
		quality = q;
	else
		quality = 0.5f;

	running = false;
	encoder_thread = nullptr;
	handle = NULL;
	outputFile = NULL;
}

SDLTheora::~SDLTheora()
{
	std::lock_guard<std::mutex> lock1(incoming_mutex);
	for(auto& i : incoming){
		delete[] i->buffer[0].data;
		delete[] i->buffer[1].data;
		delete[] i->buffer[2].data;
		delete i;
	}

	incoming.clear();

	std::lock_guard<std::mutex> lock2(start_lock);
	running = false;
	if(encoder_thread){
		encoder_thread->detach();
		delete encoder_thread; // shutdown using force
	}

	if(outputFile)
		fclose(outputFile);

	if(handle)
		th_encode_free(handle);
}


// setups encoding structure
bool SDLTheora::startEncoding(const std::string& filename, unsigned int width, unsigned int height)
{
	std::lock_guard<std::mutex> lock(start_lock);

	if(width <= 0 || height <= 0)
		return false;

	if(running)
		return false;

	error_flag = false;

	th_info format;

	memset(&format, 0, sizeof(format));

	frameHeight = ((height + 15)/16)*16;
	frameWidth  = ((width + 15)/16)*16;

	format.frame_width = frameWidth;
	format.frame_height = frameHeight;
	format.pic_width = width;
	format.pic_height = height;
	format.pic_x = 0;
	format.pic_y = 0;
	format.quality = (int)(63*quality);
	format.target_bitrate = 0;
	format.pixel_fmt = TH_PF_444;
	format.colorspace = TH_CS_UNSPECIFIED;
	format.fps_numerator = FPS; // frames per second!
	format.fps_denominator = 1;

	handle = th_encode_alloc(&format);

	if(handle == NULL)
		return false;

	// sets encoding speed to the maximum
	{
		int splevel = 100;
		th_encode_ctl(handle, TH_ENCCTL_GET_SPLEVEL_MAX, &splevel, sizeof(int));
		int result = th_encode_ctl(handle, TH_ENCCTL_SET_SPLEVEL, &splevel, sizeof(int));

		if(result == TH_EFAULT || result == TH_EINVAL || result == TH_EIMPL)
			logging.warn("couldn't get theora to maximum encoding speed");
	}

	outputFile = fopen(filename.c_str(), "wb");
	if(outputFile == NULL){
		th_encode_free(handle);
		return false;
	}

	if(ferror(outputFile)){
		fclose(outputFile);
		th_encode_free(handle);
		return false;
	}


	try{
		latest_frame_encoded = -1;
		encoder_thread = new std::thread(encoder_loop, this);

		if(encoder_thread == nullptr){
			running = false;
			fclose(outputFile);
			th_encode_free(handle);
			outputFile = NULL;
			handle = nullptr;
			return false;
		}
	}
	catch(std::exception& e){
		running = false;
		fclose(outputFile);
		th_encode_free(handle);
		outputFile = NULL;
		handle = nullptr;
		return false;
	}

	return true;
}


// inserts SDL_Surface picture frame into video at msecs
// onwards since the start of the encoding (msecs = 0 is the first frame)
bool SDLTheora::insertFrame(unsigned int msecs, SDL_Surface* surface)
{
	// very quick skipping of frames [without conversion] when picture for the current frame has been already inserted
	const int frame = msecs/MSECS_PER_FRAME;
	if(frame <= latest_frame_encoded)
		return false;

	if(running){
		if(__insert_frame(msecs, surface, false)){
			latest_frame_encoded = frame;
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

// inserts last frame and stops encoding (and saves and closes file when encoding has stopped)
bool SDLTheora::stopEncoding(unsigned int msecs, SDL_Surface* surface)
{
	if(running){
		if(__insert_frame(msecs, surface, true) == false){
			logging.fatal("sdl-theora: inserting LAST frame failed");
		}
	}
	else{
		return false;
	}

	std::lock_guard<std::mutex> lock(start_lock);

	encoder_thread->join(); // bug: may block forever... [should do timed wait instead?]
	delete encoder_thread;

	encoder_thread = nullptr;
	handle = NULL;
	outputFile = NULL;

	return (running == false); // everything went correctly
}


bool SDLTheora::__insert_frame(unsigned int msecs, SDL_Surface* surface, bool last)
{
	// converts SDL into YUV format [each plane separatedly and have full width and height]
	// before sending it to the encoder thread

	SDL_Surface* frame = SDL_CreateRGBSurface(0, frameWidth, frameHeight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);

	if(frame == NULL){
		logging.error("sdl-theora::__insert_frame failed [1]");
		return false;
	}

	if(surface != NULL){
		SDL_BlitSurface(surface, NULL, frame, NULL);
	}
	else{ // just fills the frame with black
		SDL_FillRect(frame, NULL, SDL_MapRGB(frame->format, 0, 0, 0));
	}

#if 0
	// NOT supported by SDL2: needs to convert to YUV format by hand

	SDL_Surface* yuv = SDL_ConvertSurfaceFormat(frame, SDL_PIXELFORMAT_IYUV, 0);

	if(yuv == NULL){
		logging.error("sdl-theora::__insert_frame failed [2]");
		logging.error(SDL_GetError());
		SDL_FreeSurface(frame);
		return false;
	}
#endif

	// assumes yuv pixels format is full plane for each component:
	// Y plane (one byte per pixel), U plane (one byte per pixel), V plane (one byte per pixel)

	SDLTheora::frame* f = new SDLTheora::frame;

	f->msecs = msecs;

	f->buffer[0].width  = frameWidth;
	f->buffer[0].height = frameHeight;
	f->buffer[0].stride = frameWidth;
	f->buffer[1].width  = frameWidth;
	f->buffer[1].height = frameHeight;
	f->buffer[1].stride = frameWidth;
	f->buffer[2].width  = frameWidth;
	f->buffer[2].height = frameHeight;
	f->buffer[2].stride = frameWidth;

	f->buffer[0].data = new unsigned char[frameHeight*frameWidth];
	f->buffer[1].data = new unsigned char[frameHeight*frameWidth];
	f->buffer[2].data = new unsigned char[frameHeight*frameWidth];

	unsigned int index = 0;
	for(unsigned int y=0;y<(unsigned)frameHeight;y++){
		for(unsigned int x=0;x<(unsigned)frameWidth;x++, index++){
			unsigned int* source = (unsigned int*)(frame->pixels);

			const unsigned int r = (source[index] & 0x00FF0000)>>16;
			const unsigned int g = (source[index] & 0x0000FF00)>> 8;
			const unsigned int b = (source[index] & 0x000000FF)>> 0;

			auto y  =  (0.257*r) + (0.504*g) + (0.098*b) + 16.0;
			auto Cr =  (0.439*r) - (0.368*g) - (0.071*b) + 128.0;
			auto Cb = -(0.148*r) - (0.291*g) + (0.439*b) + 128.0;

			if(y < 0.0) y = 0.0;
			else if(y > 255.0) y = 255.0;

			if(Cr < 0.0) Cr = 0.0;
			else if(Cr > 255.0) Cr = 255.0;

			if(Cb < 0.0) Cb = 0.0;
			else if(Cb > 255.0) Cb = 255.0;

			f->buffer[0].data[index] = (unsigned char)round(y);
			f->buffer[1].data[index] = (unsigned char)round(Cb);
			f->buffer[2].data[index] = (unsigned char)round(Cr);


		}
	}

	// const unsigned char* source = (unsigned char*)yuv->pixels;
	// memcpy(f->buffer[0].data, source + 0, (yuv->w)*(yuv->h)); // Y plane
	// memcpy(f->buffer[1].data, source + 1*(yuv->w)*(yuv->h), (yuv->w)*(yuv->h)); // U plane
	// memcpy(f->buffer[2].data, source + 2*(yuv->w)*(yuv->h), (yuv->w)*(yuv->h)); // V plane

	f->last = last; // IMPORTANT!

	{
		std::lock_guard<std::mutex> lock1(start_lock);
		std::lock_guard<std::mutex> lock2(incoming_mutex);

		// always processes special LAST frames
		if((running == false || incoming.size() >= MAX_QUEUE_LENGTH) && f->last != true){
			logging.error("sdl-theora::__insert_frame failed [3]");

			delete[] f->buffer[0].data;
			delete[] f->buffer[1].data;
			delete[] f->buffer[2].data;
			delete f;

			SDL_FreeSurface(frame);
			return false;
		}
		else
			incoming.push_back(f);
	}

	SDL_FreeSurface(frame);

	return true;
}


// thread to do all encoding communication between theora and
// writing resulting frames into disk
void SDLTheora::encoder_loop()
{
	running = true;

	logging.info("sdl-theora: encoder thread started..");

	prev = nullptr;

	ogg_stream_state ogg_stream;
	ogg_packet packet;
	ogg_page page;

	memset(&ogg_stream, 0, sizeof(ogg_stream));
	memset(&packet, 0, sizeof(packet));
	memset(&page, 0, sizeof(page));

	if(ogg_stream_init(&ogg_stream, (int)0xC0DEC0DE) != 0){
		logging.error("sdl-theora: cannot initialize ogg-stream => abort");
		error_flag = true;
		running = false;
		return;
	}

	const char* vendorName = "CSLR WHiTEiCE NEUROMANCER NEUROSTIM <nop@iki.fi>";
	char buffer[256];
	strcpy(buffer, vendorName);

	th_comment comment;
	comment.user_comments = NULL;
	comment.comment_lengths = NULL;
	comment.comments = 0;
	comment.vendor = buffer;

	int result = 0;

	// first encode headers
	while((result = th_encode_flushheader(handle, &comment, &packet)) != 0){
		if(result < 0){
			error_flag = true;
			logging.error("sdl-theora: encoding headers failed (1) => abort");
			running = false;
			ogg_stream_destroy(&ogg_stream);
			return; // something went wrong => abort
		}

		if(ogg_stream_packetin(&ogg_stream, &packet) != 0){
			error_flag = true;
			logging.error("sdl-theora: encoding headers failed (2) => abort");
			running = false;
			ogg_stream_destroy(&ogg_stream);
			return;
		}

		if(ogg_stream_pageout(&ogg_stream, &page) != 0){
			// write page to file..
			fwrite(page.header, 1, page.header_len, outputFile);
			fwrite(page.body, 1, page.body_len, outputFile);

			if(ferror(outputFile)){
				error_flag = true;
				logging.error("sdl-theora: encoding headers failed (3) => abort");
				running = false;
				ogg_stream_destroy(&ogg_stream);
				return;
			}
		}
	}


	logging.info("sdl-theora: theora video headers written..");


	// keeps encoding incoming frames
	SDLTheora::frame* f = nullptr;
	int latest_frame_generated = -1;

	while(1)
	{
		{
			char buffer[80];
			sprintf(buffer, "sdl-theora: incoming frame buffer size: %d", (int)incoming.size());

			logging.info(buffer);
		}

		{
			incoming_mutex.lock();
			if(incoming.size() > 0){ // has incoming picture data
				f = incoming.front();
				incoming.pop_front();
				incoming_mutex.unlock();
			}
			else{
				incoming_mutex.unlock();

				// sleep here ~10ms [time between frames 40ms]
				std::this_thread::sleep_for(std::chrono::milliseconds(MSECS_PER_FRAME/4));

				continue;
			}
		}

		// converts milliseconds field to frame number
		int f_frame = (f->msecs / MSECS_PER_FRAME);

		// if there has been no frames between:
		// last_frame_generated .. f_frame
		// fills them with latest_frame_generated (prev)

		if(latest_frame_generated == -1 && f_frame >= 0){
			// writes f frame
			latest_frame_generated = 0;

			logging.info("sdl-theora: writing initial f-frames");

			for(int i=latest_frame_generated;i<f_frame;i++){
				if(encode_frame(f->buffer, &ogg_stream, &packet, &page) == false)
					logging.error("sdl-theora: encoding frame failed");
			}
		}
		else if((latest_frame_generated+1) < f_frame){
			// writes prev frames

			logging.info("sdl-theora: writing prev-frames");

			for(int i=(latest_frame_generated+1);i<f_frame;i++){
				if(encode_frame(prev->buffer, &ogg_stream, &packet, &page) == false)
					logging.error("sdl-theora: encoding frame failed");
			}
		}

		// writes f-frame once (f_frame) if it is a new frame for this msec
		// OR if it is a last frame [stream close frame]
		if(latest_frame_generated < f_frame || f->last)
		{
			logging.info("sdl-theora: writing current frame");

			if(encode_frame(f->buffer, &ogg_stream, &packet, &page, f->last) == false)
				logging.error("sdl-theora: encoding frame failed");
		}

		latest_frame_generated = f_frame;

		if(prev != nullptr){
			delete[] prev->buffer[0].data;
			delete[] prev->buffer[1].data;
			delete[] prev->buffer[2].data;
			delete prev;
			prev = nullptr;
		}

		prev = f;

		if(f->last == true){
			logging.info("sdl-theora: special last frame seen => exit");
			break;
		}
	}

	// writes yet unhandled packets into a page
	{
		if(ogg_stream_flush(&ogg_stream, &page) != 0){
			// write page to file..
			fwrite(page.header, 1, page.header_len, outputFile);
			fwrite(page.body, 1, page.body_len, outputFile);

			if(ferror(outputFile)){
				error_flag = true;
			}
		}
	}


	logging.info("sdl-theora: theora encoder thread shutdown sequence..");

	logging.info("sdl-theora: encoder thread shutdown: ogg_stream_destroy");
	ogg_stream_destroy(&ogg_stream);
	logging.info("sdl-theora: encoder thread shutdown: ogg_stream_destroy.. done");


	// all frames has been written
	if(prev != nullptr){
		delete[] prev->buffer[0].data;
		delete[] prev->buffer[1].data;
		delete[] prev->buffer[2].data;
		delete prev;
		prev = nullptr;
	}

	logging.info("sdl-theora: encoder thread shutdown: incoming buffer clear");

	{
		std::lock_guard<std::mutex> lock1(incoming_mutex);
		for(auto i : incoming){
			delete[] i->buffer[0].data;
			delete[] i->buffer[1].data;
			delete[] i->buffer[2].data;
			delete i;
		}

		incoming.clear();
	}

	{
		logging.info("sdl-theora: encoder thread halt. running = false");
		running = false;
	}
}


bool SDLTheora::encode_frame(th_ycbcr_buffer buffer,
		ogg_stream_state* ogg_stream,
		ogg_packet* packet,
		ogg_page* page,
		bool last)
{
	if(th_encode_ycbcr_in(handle, buffer) != 0)
		return false;

	while(th_encode_packetout(handle, last, packet) != 0){
		if(ogg_stream_packetin(ogg_stream, packet) != 0){
			error_flag = true;
			return false;
		}

		if(ogg_stream_pageout(ogg_stream, page) != 0){
			// write page to file..
			fwrite(page->header, 1, page->header_len, outputFile);
			fwrite(page->body, 1, page->body_len, outputFile);

			if(ferror(outputFile)){
				error_flag = true;
				return false;
			}
		}
	}

	return true;
}



}
} /* namespace whiteice */
