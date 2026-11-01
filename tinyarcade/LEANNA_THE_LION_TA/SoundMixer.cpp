#include <string.h>
#include "SoundMixer.hpp"

SoundMixer::SoundMixer(void)
{
	Clear();
}

SoundMixer::~SoundMixer(void)
{
}

void SoundMixer::Append(int channel, const unsigned char* buffer, int size, bool loop)
{
	this->buffer[channel] = buffer;
	this->size[channel] = size;
	this->sample[channel] = 0;
	this->loop[channel] = loop;
}

void SoundMixer::Delete(int channel)
{
	this->buffer[channel] = NULL;
	this->size[channel] = 0;
	this->sample[channel] = 0;
	this->loop[channel] = false;
}

void SoundMixer::Read(unsigned char* buffer, int size)
{
	memset(buffer, 0x80, size);
	for(int channel = 0; channel < CHANNEL_COUNT; ++ channel)
	{
		if(this->buffer[channel] == NULL)
		{
			continue;
		}
		int mix_size = size;
		int left_size = this->size[channel] - this->sample[channel];
		if(left_size < size)
		{
			mix_size = left_size;
		}
		if(mix_size > 0)
		{
			MixBuffer(buffer, &this->buffer[channel][this->sample[channel]], mix_size);
			this->sample[channel] += mix_size;
			if(this->loop[channel] == true)
			{
				if(size > left_size)
				{
					int left_mix_size = size - left_size;
					if(left_mix_size > 0)
					{
						this->sample[channel] = 0;
						MixBuffer(&buffer[mix_size], &this->buffer[channel][this->sample[channel]], left_mix_size);
						this->sample[channel] += left_mix_size;
					}
				}
			}
		}
		if(this->sample[channel] >= this->size[channel])
		{
			this->sample[channel] = 0;
			if(this->loop[channel] == false)
			{
				this->buffer[channel] = NULL;
				this->size[channel] = 0;
			}
		}
	}
}

void SoundMixer::Clear(void)
{
	for(int channel = 0; channel < CHANNEL_COUNT; ++ channel)
	{
		this->buffer[channel] = NULL;
		this->size[channel] = 0;
		this->sample[channel] = 0;
		this->loop[channel] = false;
	}
}

void SoundMixer::MixBuffer(unsigned char* destination_buffer, const unsigned char* source_buffer, int size)
{
	for(int sample = 0; sample < size; ++ sample)
	{
		short data = static_cast<short>(source_buffer[sample]) + static_cast<short>(destination_buffer[sample]) - 128;
		data = ((data > 0) ? ((data < 255) ? data : 255) : 0);
		destination_buffer[sample] = static_cast<unsigned char>(data);
	}
}
