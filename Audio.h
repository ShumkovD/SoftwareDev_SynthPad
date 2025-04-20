#pragma once
#include <Windows.h>
#include <mmeapi.h>
#include <iostream>
#include <numbers>
#include <thread>
#include<map>
#include<unordered_set>
#include<algorithm>

#pragma comment(lib, "winmm.lib")

double const static noteFrequencies[12] = {
	261.6256 , // C4
	277.1826 , // C#4
	293.6648 , // D4
	311.1270, //D#4
	329.6276, // E4
	349.2282, // F4
	369.9944, // F#4
	391.9954, //G4
	415.3047, //G#4
	440.0000, //A4
	466.1638, // A#4
	493.8833, //B4
};

double static synthPhases[12] = {
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
	0.0 ,
};

std::map<char, int> const inputMaps = {
	{'Z', 0 },
	{'S', 1 },
	{'X', 2 },
	{'D', 3 },
	{'C', 4 },
	{'V', 5 },
	{'G', 6 },
	{'B', 7 },
	{'H', 8 },
	{'N', 9 },
	{'J', 10 },
	{'M', 11 },
};

class LLAudio
{
	/// Variables
	// Output Device
	HWAVEOUT phwo;
	// Information about the playback
	WAVEFORMATEX *pwfx;
	
	std::thread audioThread;

	// Structure which holds all the data about AudioBuffer
	struct AudioBuffer {
		WAVEHDR pwh;
		short* audioBuffer;
		bool hasNewPlayData;
		bool hasEnded;
	};


	void MainAudioLoop();

public:

	LLAudio();
	~LLAudio();
	std::unordered_set<int> pressedNotes;
	AudioBuffer audioBuffer[4];
	// Creation of the device
	int LLAudioInit();
	// Creation of buffers, which are used to store information
	int LLAudioCreateAudioBuffer();
	// Put information about sound here
	int FillTheBuffer(int bufferIndex);
	// The preparation of the buffer for the playback
	int LLAudioPrepareAudioBuffer(int bufferIndex);
	// Playback
	int LLAudioPlayBuffers(int bufferIndex);
	int FindBufferIndex(WAVEHDR* header);
};
