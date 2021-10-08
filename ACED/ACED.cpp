#include "ACED.h"

#define KeyDown(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

#define MC_KEY 144
#define MC_PADEL 176
#define MC_PITCHWHELL 224

#define TITLEBARHIGHT 30

//初学者，码风不成熟，不习惯写注释，第一次用RtMidi库可能有些错误
//为了降低延迟不惜一切代价，代码有些诡异还请见谅
//更改下方KEYS来更改音阶，使之与 ACE Virtual Singer 中的音阶相同（或许可以通过更改为不同音阶来实现转调，它只是一个MIDI信号->按键位置的映射）
//KEYS中数字为MIDI messsage中代表按键的数字，参见MIDI相关文档，从低音so到高音la依次为55到81，每一个“半音”数字加1
//更改POSITIONS来更改在手机上各个音按键对应的Y坐标除以手机分辨率Y坐标所得的值（也或许不需要更改）
//
//例如下方的KEYS为D大调音阶
//下方POSITION为在纵横比为2/1的手机上所得的Y相对坐标
//
//使用时请提前在手机端选择好音阶，并开始录制
//
//Using scrcpy to implement low-latency screen clicks to an android phone.
//Using RtMidi Library for midi connection.

const int KEYS[16] = {
	55,57,59,61,62,64,66,67,69,71,73,74,76,78,79,81
};
const double POSITIONS[16] = {
	0.073,0.128,0.182,0.235,0.290,0.347,0.398,0.450,0.510,0.562,0.617,0.668,0.728,0.782,0.833,0.892
};
const double KEYHIGHT = 0.0546;


struct KEYMAP {
	int k;
	int p;
	int s;
};

KEYMAP keyMap[16];

bool keyState[16];

unsigned int numPortIn;
long long selPortIn;

RECT scrcpyRect;
HWND scrcpyHwnd;
double desktopYFactor,desktopXFactor;

unsigned int nByte;
unsigned int type;

int channel, key, dynamic;

std::string namePort;

int valuePitchWheel = 64;

int tmp;
bool flag;

/*

void MLMoveDown(int x, int y,HWND hwnd) {

	//std::cout << hwnd << std::endl;
	std::cout << "Mouse Down: " << SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y)) << "\n";

	return;
}

void MLUp(HWND hwnd) {
	//std::cout << hwnd << std::endl;
	std::cout << "Mouse Up: " << (hwnd, WM_LBUTTONUP, NULL, MAKELPARAM(0,0)) << "\n";
	
	
	return;
}

void MMove(int x, int y, HWND hwnd) {
	//std::cout << hwnd << std::endl;
	std::cout << "Mouse Move: " << SendMessage(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x, y)) << "\n";

	return;
}
*/


int NumInput(int min, int max) {
	int i;
	scanf("%d", &i);
	if (i >= min && i <= max) {
		return i;
	}
	else {
		std::cout << "Invalied value.Please type another value.\n";
		return NumInput(min, max);
	}
}

int SelMidiInPort(RtMidiIn *mi) {
	try {
		numPortIn = mi->getPortCount();
	}
	catch (RtMidiError &error) {
		error.printMessage();
	}

	if (numPortIn > 0) {
		std::cout << numPortIn << "MIDI input ports available.\n";
		for (int i = 0; i < numPortIn; ++i) {
			namePort = mi->getPortName(i);
			std::cout << "#" << i + 1 << "	" << namePort << "\n";
		}
		std::cout << "Type the sequence to select a MIDI input device.\n";
		int j = NumInput(1, numPortIn);
		j--;
		return j;
	}
	else {
		std::cout << "No Midi Input Port Available.\nPress ESC to quit.Press Enter to search again.\n";
		while (1) {

			if (KeyDown(VK_ESCAPE)) {
				return -1;
			}

			if (KeyDown(VK_RETURN)) {
				return SelMidiInPort(mi);
			}

			Sleep(1);
		}

	}


}

void HandleMidiMessage(std::vector<unsigned char> *message) {
	for (int j = 0; j < (message->size())/3; ++j) {
		channel = (int)message->at(j * 3);
		key = (int)message->at(j * 3 + 1);
		dynamic = (int)message->at(j * 3 + 2);
		if (key <= KEYS[15] && key >= KEYS[0]) {
			switch (channel) {
			case MC_KEY:
				if (dynamic == 0) {
					flag = false;
					for (int i = 0; i < 16; ++i) {
						if (key == keyMap[i].k) {
							keyState[i] = false;
							
						}
						if (keyState[i] == true) {
							flag = true;
						}
					}
					//To prevent undesirable mouse left up
					if (flag == false) {
						mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
						//MLUp(scrcpyHwnd);
					}
					else {
						std::cout << "There are still keys pressed.\n";
					}

					
				}
				else {
					for (int i = 0; i < 16; ++i) {
						if (key == keyMap[i].k) {
							tmp = 15 - i;
							keyState[i] = true;
							std::cout << "value = " << tmp << "\n";
							break;
						}
					}
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN, 
						(int)(((double)(scrcpyRect.right - scrcpyRect.left) / 3 * 2)*desktopXFactor), 
						(int)(((double)keyMap[tmp].p-(((double)valuePitchWheel-64)/64*KEYHIGHT*(scrcpyRect.bottom - scrcpyRect.top))+TITLEBARHIGHT)*desktopYFactor), 
						0, 0);
					//MLUp(scrcpyHwnd);
					//MLMoveDown((scrcpyRect.right - scrcpyRect.left) / 2, keyMap[tmp].p, scrcpyHwnd);
				}
				break;
			case MC_PITCHWHELL:
				mouse_event(MOUSEEVENTF_MOVE, 0, ((double)(valuePitchWheel - dynamic)) / (double)64 * (KEYHIGHT*(scrcpyRect.bottom - scrcpyRect.top)), 0, 0);
				valuePitchWheel = dynamic;
				std::cout << "Pitch Wheel= " << valuePitchWheel << "\n";
				break;
				// It should work but it works not so smoothly when the pitch wheel goes up (when the value is in the range[64,128]).
				// It reacts a little bit slow and need optimization
			default:
				break;
			}
			
		}

	}
}

void RtmCallBack(double timeStamp, std::vector<unsigned char> *message, void *userData) {

	std::cout << "Time Stamp = " << timeStamp << "\n";
	nByte = message->size();
	
	for (unsigned int i = 0; i < nByte; ++i) {
		type = i % 3;
		switch (type) {
		case 0:
			std::cout << "Channel = ";
			break;

		case 1:
			std::cout << "Key = ";
			break;

		case 2:
			std::cout << "Dynamic = ";
			break;

		default:
			std::cout << "Unknown value = ";
		}
		std::cout << (int)message->at(i) << "  ";
	}
	std::cout << "\nUser Data = " << userData << "\n\n";

	HandleMidiMessage(message);
	
	return;
}

RECT LaunchScrcpy() {

	
	

	HWND desktopHandle = GetDesktopWindow();
	RECT desktopRect,scrcpyRect,adjustRect;
	GetWindowRect(desktopHandle, &desktopRect);
	//ShellExecute(NULL,L"open",L"scrcpy\\scrcpy-noconsole.exe",NULL,NULL,SW_SHOWNORMAL);

	SHELLEXECUTEINFO lpExecInfo = { 0 };
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.lpVerb = L"Open";
	lpExecInfo.hwnd = NULL;
	lpExecInfo.lpFile = L"scrcpy-noconsole.exe";
	lpExecInfo.lpDirectory = L"\\scrcpy";
	lpExecInfo.nShow = SW_SHOWNORMAL;
	lpExecInfo.lpParameters = NULL;
	lpExecInfo.hInstApp = NULL;
	ShellExecuteEx(&lpExecInfo);

	std::cout << "excute scrcpy.\n";
	
	Sleep(5000);
	//This is to deal with the situation that scrcpy starts too slowly due to the adb service not running

	scrcpyHwnd = GetForegroundWindow();
	GetWindowRect(scrcpyHwnd, &scrcpyRect);
	int sl = scrcpyRect.bottom - scrcpyRect.top;
	int sw = scrcpyRect.right - scrcpyRect.left;

	double aspectRatio = (double)sw / sl;

	desktopYFactor = ((double)65533) / (desktopRect.bottom - desktopRect.top);
	desktopXFactor = ((double)65533) / (desktopRect.right - desktopRect.left);
	

	MoveWindow(scrcpyHwnd, desktopRect.left, desktopRect.top, (int)(((double)desktopRect.bottom - (double)desktopRect.top)*aspectRatio), desktopRect.bottom - desktopRect.top, true);

	adjustRect.left = desktopRect.left;
	adjustRect.right = desktopRect.left + (int)(((double)desktopRect.bottom - (double)desktopRect.top)*aspectRatio);
	adjustRect.top = desktopRect.top + TITLEBARHIGHT;
	adjustRect.bottom = desktopRect.bottom;

	std::cout << "Aspect Ratio = " << aspectRatio << "\n";

	std::cout << adjustRect.top << "\n" << adjustRect.left << "	" << adjustRect.right << "\n" << adjustRect.bottom << "\n";


	return adjustRect;
}

int main() {


	

		


	RtMidiIn *midiIn = new RtMidiIn();

	selPortIn = SelMidiInPort(midiIn);
	
	if (selPortIn < 0){
		std::cout << "Quiting" << std::endl;
		goto cleanup;
	}

	scrcpyRect = LaunchScrcpy();

	for (int i = 0; i < 16; ++i) {
		keyMap[i].k = KEYS[i];
		keyMap[i].p = (int)((double)(scrcpyRect.bottom - scrcpyRect.top)*POSITIONS[i]);
		keyMap[i].s = i;
		std::cout << keyMap[i].p << "	";
	}
	std::cout << std::endl;
	
	try {
		midiIn->openPort(selPortIn);
	}
	catch (RtMidiError &error) {
		error.printMessage();
		goto cleanup;
	}

	midiIn->setCallback(&RtmCallBack);
	//midiIn->ignoreTypes(true, true, true);

	std::cout << "Linked to the MIDI input port successfully. \nReading messages...\n\nPress ESC to quit.\n";

	while (1) {
		if (KeyDown(VK_ESCAPE)) {
			std::cout << "Quiting" << std::endl;
			break;
		}
		Sleep(1);
	}

cleanup:
	delete midiIn;

	return 0;
}