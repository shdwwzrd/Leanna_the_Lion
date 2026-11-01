#include <stdlib.h>
#include "SoundStream.hpp"
#include "SoundPlayer.hpp"

SoundPlayer* SoundPlayer::sound_player;

SoundPlayer::SoundPlayer(void)
:sound_mixer()
{
	for(int i = 0; i < APPEND_MAX; ++ i)
	{
		Delete(i);
	}
}

SoundPlayer::~SoundPlayer(void)
{
}

void SoundPlayer::Initialize(int frequency)
{
	SoundStream::Initialize(frequency);
	SoundPlayer::sound_player = new SoundPlayer();
}

void SoundPlayer::Finalize(void)
{
	if(SoundPlayer::sound_player != NULL)
	{
		delete SoundPlayer::sound_player;
		SoundPlayer::sound_player = NULL;
	}
	SoundStream::Finalize();
}

SoundPlayer& SoundPlayer::GetInstance()
{
	return *SoundPlayer::sound_player;
}

int SoundPlayer::Append(int channel, const unsigned char* buffer, int size, bool loop)
{
	int handle = GetHandle();
	if(handle > -1)
	{
		this->channel[handle] = channel;
		this->buffer[handle] = buffer;
		this->size[handle] = size;
		this->loop[handle] = loop;
	}
	return handle;
}

void SoundPlayer::Delete(int handle)
{
	if(handle == -1)
	{
		return;
	}
	this->channel[handle] = 0;
	this->buffer[handle] = NULL;
	this->size[handle] = 0;
	this->loop[handle] = false;
}

void SoundPlayer::Play(int handle)
{
	if(handle == -1)
	{
		return;
	}
	sound_mixer.Append(this->channel[handle], this->buffer[handle], this->size[handle], this->loop[handle]);
}

void SoundPlayer::Stop(int handle)
{
	if(handle == -1)
	{
		return;
	}
	sound_mixer.Delete(this->channel[handle]);
}

void SoundPlayer::StopAll(void)
{
	for(int i = 0; i < APPEND_MAX; ++ i)
	{
		if(this->buffer[i] != NULL)
		{
			Stop(i);
			break;
		}
	}
}

void SoundPlayer::Update(void)
{
	SoundStream& sound_stream = SoundStream::GetInstance();
	sound_stream.Update();
	int writable_size = sound_stream.GetWritableSize();
	if(writable_size > 0)
	{
		unsigned char* temp = new unsigned char[writable_size];
		this->sound_mixer.Read(temp, writable_size);
		sound_stream.Write(temp);
		delete [] temp;
	}
}

int SoundPlayer::GetHandle(void)
{
	int handle = -1;
	for(int i = 0; i < APPEND_MAX; ++ i)
	{
		if(this->buffer[i] == NULL)
		{
			handle = i;
			break;
		}
	}
	return handle;
}
