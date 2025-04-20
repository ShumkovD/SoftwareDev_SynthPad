#include"Audio.h"

/// Buffer Data
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-
const int BUFFER_SIZE = 1024;
int CUR_BUFFER = 0;
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-

/// Synth Data
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-
static float SYNTH_PHASE = 0;
//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-//+-+-



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
		audioBuffer[i].hasEnded = true;
		audioBuffer[i].hasNewPlayData = false;
	}
	return 0;
}

// Put information about sound here
int LLAudio::FillTheBuffer(int bufferIndex)
{

	if (!audioBuffer[bufferIndex].hasEnded)
		return 0;

	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		double mixedSample = 0.0;
		audioBuffer[bufferIndex].audioBuffer[i] = 0;
		for (int note : pressedNotes)
		{
			double freq = noteFrequencies[note];
			double& phase = synthPhases[note]; // Reference to retain between calls

			mixedSample += std::sin(phase);
			phase += 2.0 * std::numbers::pi * freq / 44100.0;

			if (phase > 2.0 * std::numbers::pi)
				phase -= 2.0 * std::numbers::pi;
		}


		// Normalize (optional): reduce volume if multiple notes
		if (!pressedNotes.empty())
			mixedSample /= pressedNotes.size();

		// Clamp and store as 16-bit PCM
		mixedSample = std::clamp(mixedSample, -1.0, 1.0);
		audioBuffer[bufferIndex].audioBuffer[i] = static_cast<short>(mixedSample * 32767);

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
			if (pressedNotes.size() == 0)
			{
				continue;
			}
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
				audioBuffer[i].hasEnded = false;
			}
		}


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

		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			// Clear buffer
			self->audioBuffer[doneBufferIndex].audioBuffer[i] = 0;
		}
	}
}

