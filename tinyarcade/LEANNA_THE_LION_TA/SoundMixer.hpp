#ifndef TINYSOUNDMIXER_HPP
#define TINYSOUNDMIXER_HPP

class SoundMixer
{
public:
	enum
	{
		CHANNEL_COUNT = 4,
	};
	SoundMixer(void);
	~SoundMixer(void);
	void Append(int channel, const unsigned char* buffer, int size, bool loop);
	void Delete(int channel);
	void Read(unsigned char* buffer, int size);
	void Clear(void);
private:
	void MixBuffer(unsigned char* destination_buffer, const unsigned char* source_buffer, int size);
	const unsigned char* buffer[CHANNEL_COUNT];
	int size[CHANNEL_COUNT];
	int sample[CHANNEL_COUNT];
	bool loop[CHANNEL_COUNT];
	SoundMixer(SoundMixer&);
	SoundMixer& operator = (SoundMixer&);
};

#endif
