#include"Audio.h"


/// Sound Data
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-
constexpr int SAMPLE_RATE = 44100;
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-

/// Buffer Data
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-
constexpr int BUFFER_AMOUNT = 4;
constexpr int BUFFER_SIZE = 1024;
constexpr int CUR_BUFFER = 0;
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-

double SYNTH_PHASE = 0;

/// LLAudio Class Functions
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-

/// <summary>
/// Constructor
/// </summary>
LLAudio::	LLAudio()
	{
		pwfx = nullptr;
		phwo = nullptr;
	}

/// <summary>
/// Destructor
/// </summary>
LLAudio::~LLAudio()
	{
	waveOutUnprepareHeader(phwo, &audioBuffer[0].pwh, sizeof(WAVEHDR));
	waveOutUnprepareHeader(phwo, &audioBuffer[1].pwh, sizeof(WAVEHDR));
	waveOutClose(phwo);
	delete[] audioBuffer;

		free(pwfx);
		pwfx = nullptr;
		phwo = nullptr;

	}

/// <summary>
/// Callback, where the bufferÅfs Ping-Pong is being done
/// </summary>
/// <param name="hwo"> Audio Out Device</param>
/// <param name="uMsg">Current state of the device</param>
/// <param name="dwInstance">Pointer to the current instance of the class</param>
/// <param name="dwParam1">Used for retrieving finished buffer</param>
void CALLBACK LLAudioCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1);


/// <summary>
/// Initialization
/// </summary>
/// <returns></returns>
int LLAudio::LLAudioInit()
{
	// Setting up the sound wave structure
	pwfx = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
	// Error check
	if (pwfx == nullptr)
	{
		std::printf("Error was encountered on allocation of memory for the sound device. Sound structure was not created");
		return 300; // Audio Error code
	}
	pwfx->wFormatTag = WAVE_FORMAT_PCM;	// PCM format
	pwfx->nChannels = 1;										// Mono / Stereo
	pwfx->nSamplesPerSec = 44100;					// Sound quality
	pwfx->wBitsPerSample = 16;
	pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
	pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
	pwfx->cbSize = 0;

	// Setting up the device
	MMRESULT res = waveOutOpen(&phwo, WAVE_MAPPER, pwfx, (DWORD_PTR)LLAudioCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);

	// Error check
	if (res != MMSYSERR_NOERROR)
	{
		std::printf("Error was encountered on creation of the sound device. Error code %u\n", res);
		return 301; // Audio Error code
	}
	
	// Initialization of buffers for sound
	LLAudioCreateAudioBuffer();

	int curBuffer = 0;
	// Filling the buffer

	std::thread(&LLAudio::MainAudioLoop, this).detach();

	return 0;
};

// Creation of buffers
int LLAudio::LLAudioCreateAudioBuffer()
{
	// Initialization of 2 buffers
	for (int i = 0; i < 4; i++)
	{
		audioBuffer[i].audioBuffer = new short[BUFFER_SIZE];
		audioBuffer[i].HP_audioBuffer = new double[BUFFER_SIZE];
		audioBuffer[i].hasEnded = true;
		audioBuffer[i].hasNewPlayData = false;
	}
	return 0;
}

//// Put information about sound here
int LLAudio::FillTheBuffer(int bufferIndex)
{

	if (!audioBuffer[bufferIndex].hasEnded)
		return 0;

	std::fill(audioBuffer[bufferIndex].HP_audioBuffer,
		audioBuffer[bufferIndex].HP_audioBuffer + BUFFER_SIZE, 0.0);

		for (LLChannel& channel : channels)
		{
			if (channel.status == ChannelStatus::CHS_INACTIVE) continue;

			channel.ChannelProc(audioBuffer[bufferIndex].HP_audioBuffer);
		}



		for (int i = 0; i < BUFFER_SIZE; i++)
		{
		// Normalize (optional): reduce volume if multiple notes
		if (openChannels > 0)
			 audioBuffer[bufferIndex].HP_audioBuffer[i] /= openChannels;


		// Clamp and store as 16-bit PCM
		audioBuffer[bufferIndex].HP_audioBuffer[i] = std::clamp(audioBuffer[bufferIndex].HP_audioBuffer[i], -1.0, 1.0);
		audioBuffer[bufferIndex].audioBuffer[i] = static_cast<short>(audioBuffer[bufferIndex].HP_audioBuffer[i] * 32767);
	}

	audioBuffer[bufferIndex].hasNewPlayData = true;

	return 0;
}
// Preparing the buffers
int LLAudio::LLAudioPrepareAudioBuffer(int bufferIndex)
{
	// Giving information from the buffer to the WAVEHDR
	audioBuffer[bufferIndex].pwh.lpData = reinterpret_cast<LPSTR>(audioBuffer[bufferIndex].audioBuffer);
	audioBuffer[bufferIndex].pwh.dwBufferLength = BUFFER_SIZE * sizeof(short);
	audioBuffer[bufferIndex].pwh.dwFlags = 0;
	audioBuffer[bufferIndex].pwh.dwLoops = 0;

	// Preparing the sounds
	MMRESULT res = waveOutPrepareHeader(phwo, &audioBuffer[bufferIndex].pwh, sizeof(WAVEHDR));
	if (res != MMSYSERR_NOERROR)
	{
		std::printf("Error was encountered on creation of the sound header. Error code %u\n", res);
		return 302; // Audio Error code
	}
	return 0;
}

// Writing from buffer
int LLAudio::LLAudioPlayBuffers(int bufferIndex)
{
	MMRESULT res;

	res = waveOutWrite(phwo, &audioBuffer[bufferIndex].pwh, sizeof(WAVEHDR));
	if (res != MMSYSERR_NOERROR) {
		std::printf("Error was encountered on writing audio buffer. Error code %u\n", res);
		return 303;
	}

	return 0;
}


void  LLAudio::MainAudioLoop()
{
	while (true)
	{
		for (int i = 0; i < 4; i++)
		{
			if (openChannels == 0) continue;

			// Try to fill if it's empty and ready
			if (audioBuffer[i].hasEnded && !audioBuffer[i].hasNewPlayData)
			{
				FillTheBuffer(i);
			}

			// If filled, prepare and play it
			if (audioBuffer[i].hasEnded && audioBuffer[i].hasNewPlayData)
			{
				LLAudioPrepareAudioBuffer(i);
				LLAudioPlayBuffers(i);
				for (LLChannel& channel : channels)
				{
					if (channel.status == CHS_CLOSING)
					{
						LLAudio::openChannels--;
						channel.status = CHS_INACTIVE;
					}
				}
				audioBuffer[i].hasEnded = false;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int LLAudio::FindBufferIndex(WAVEHDR* header)
{
	for (int i = 0; i < 4; ++i)
	{
		if (header == &audioBuffer[i].pwh)
			return i;
	}
	return -1; // Not found
}

void CALLBACK LLAudioCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1)
{
	if (uMsg == WOM_DONE)
	{
		LLAudio* self = reinterpret_cast<LLAudio*>(dwInstance);

		// Find which buffer just finished playing
		WAVEHDR* doneHeader = (WAVEHDR*)dwParam1;
		int doneBufferIndex = self->FindBufferIndex(doneHeader);
		self->audioBuffer[doneBufferIndex].hasNewPlayData = false;
		self->audioBuffer[doneBufferIndex].hasEnded = true;
	}
}



LLChannel::LLChannel()
{

}

LLChannel::~LLChannel()
{

}

int LLChannel::ChannelOpen(int id) {
	LLAudio::openChannels++;
	channelFrequency = noteFrequencies[id];
	channelPhase = 0.0;
	status = ChannelStatus::CHS_OPENING;
	return 0;
}
int LLChannel::ChannelClose() {

	status = ChannelStatus::CHS_CLOSING;
	return 0;
}
int LLChannel::ChannelPlay() {
	return 0;
}

int LLChannel::ChannelProc(double* Buffer) {

	switch (status)
	{
		case ChannelStatus::CHS_OPENING:
		{
			FillChannelBuffer(Buffer, fadeInEff);
			status = ChannelStatus::CHS_USED;
			break;
		}
		case ChannelStatus::CHS_USED:
		{
			FillChannelBuffer(Buffer, nullptr);
			break;
		}
		case ChannelStatus::CHS_CLOSING:
		{
			FillChannelBuffer(Buffer, fadeOutEff);
			break;
		}
		case ChannelStatus::CHS_INACTIVE:
		{
			break;
		}
	}
	return 0;
}

int LLChannel::FillChannelBuffer(double* Buffer, void(*funcprt)(LLChannel, double&, int))
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		double curAmount = std::sin(channelPhase);

		if (funcprt != nullptr)
			funcprt(*this, curAmount, i);

		Buffer[i] += curAmount;
		channelPhase += 2.0 * std::numbers::pi * channelFrequency / 44100.0;

		if (channelPhase > 2.0 * std::numbers::pi)
			channelPhase -= 2.0 * std::numbers::pi;

	}

	return 0;
}

void  LLChannel::fadeInEff(LLChannel channel ,double& amount, int curPos)
{
	if (curPos < channel.fadeInAmount && channel.fadeInAmount != 0)
		amount *= static_cast<double>(curPos) / channel.fadeInAmount;
}

void  LLChannel::fadeOutEff(LLChannel channel, double& amount, int curPos)
{
	if (curPos > BUFFER_SIZE - channel.fadeOutAmount && channel.fadeOutAmount != 0)
		amount *=  (1.0 - (static_cast<double>(BUFFER_SIZE - curPos) / BUFFER_SIZE));
}