#include "control.h"

Control::Info Control::keys[Count] = {
	{' ', ""}, //00 NUL   @ `
		{'A', "SOH"}, //01 SOH ! A a
		{'B', "STX"}, //02 STX " B b
	{'C', "Copy"}, //03 ETX # C c
	{'D', "Directory"}, //04 EOT $ D d
		{'E', "ENQ"}, //05 ENQ % E e
	{'F', "Find"}, //06 ACK & F f
	{'G', "Go To"}, //07 BEL ' G g
		{'H', "BS"}, //08 BS ( H h
	{'I', "Tab"},  //09 HT ) I i - Tab key
	{'J', "Enter"},  //0A LF * J j - Enter key
		{'K', "VT"}, //0B VT + K k
		{'L', "FF"},  //0C FF , L l
	{'M', "Return"},  //0D CR - M m - Return key
	{'N', "New File"}, //0E SO . N n
	{'O', "Open"}, //0F SI / O o
		{'P', "DLE"},//10 DLE 0 P p
	{'Q', "Quit"}, //11 DC1 1 Q q
		{'R', "DC2"}, //12 DC2 2 R r
	{'S', "Save"}, //13 DC3 3 S s
		{'T', "DC4"}, //14 DC4 4 T t
		{'U', "NAK"}, //15 NAK 5 U u
	{'V', "Paste"}, //16 SYN 6 V v
	{'W', "Close"}, //17 ETB 7 W w
	{'X', "Cut"}, //18 CAN 8 X x
	{'Y', "Redo"}, //19 EM 9 Y y
	{'Z', "Undo"}, //1A SUB : Z z
	{'[', "Escape"}, //1B ESC ; [ {
		{'\\', "FS"}, //1C FS < \ |
		{']', "GS"},  //1D GS = ] }
		{'^', "RS"},  //1E RS > ^ ~
	{'?', "Help"},  //1F US ? _ DEL
};
