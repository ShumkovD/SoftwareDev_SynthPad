#include"Application.h"


// Window update Events
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Window registration
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Window class registration
	const wchar_t CLASS_NAME[] = L"LowLevel_SynthPad";
	// Window creation
	WNDCLASS wc = { };

	//Setting window instance
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"LowLevel_SynthPad",
		WS_OVERLAPPEDWINDOW,

		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) return 0;

	// Audio Initialization
	audioObject = new LLAudio();
	if (audioObject == nullptr)
	{
		return 299;
	}
	int res = audioObject->LLAudioInit();
	if (res != 0)
	{
		return res;
	}

	ShowWindow(hwnd, nCmdShow);



	// Running the message loop
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}



	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
		return 0;
	}

		case WM_KEYDOWN:
		{
			auto it = inputMaps.find(wParam);
			if (it != inputMaps.end()) 
			{
				if (audioObject->channels[it->second].status == CHS_INACTIVE)
				{
					audioObject->channels[it->second].ChannelOpen(it->second);
				}
				break;
			}
		}

		case WM_KEYUP:
		{
			auto it = inputMaps.find(wParam);
			if (it != inputMaps.end()) {
				if (audioObject->channels[it->second].status == CHS_USED)
				{
					audioObject->channels[it->second].ChannelClose();
				}
			}
			break;
		}

	



	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}