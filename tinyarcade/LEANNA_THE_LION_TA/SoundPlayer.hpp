#ifndef TINYSOUNDPLAYER_HPP
#define TINYSOUNDPLAYER_HPP

#include "SoundMixer.hpp"

class SoundPlayer
{
public:
	enum
	{
		APPEND_MAX = 8
	};
	SoundPlayer(void);
	~SoundPlayer(void);
	static SoundPlayer* sound_player;
	static void Initialize(int frequency);
	static void Finalize(void);
	static SoundPlayer& GetInstance();
	int Append(int channel, const unsigned char* buffer, int size, bool loop);
//	int Append(const char* path, bool loop);
	void Delete(int handle);
	void Play(int handle);
	void Stop(int handle);
	void StopAll(void);
	void Update(void);
private:
	int GetHandle(void);
	int channel[APPEND_MAX];
	const unsigned char* buffer[APPEND_MAX];
	int size[APPEND_MAX];
	bool loop[APPEND_MAX];
	SoundMixer sound_mixer;
	SoundPlayer(SoundPlayer&);
	SoundPlayer& operator = (SoundPlayer&);
};

#endif
