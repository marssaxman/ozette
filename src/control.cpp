#include "control.h"

Control::Info Control::keys[Count] = {
	{' ', ""}, //00 NUL
	{'A', "SOH"}, //01
	{'B', "STX"}, //02
	{'C', "Copy"}, //03 ETX
	{'D', "EOT"}, //04
	{'E', "ENQ"}, //05
	{'F', "Find"}, //06 ACK
	{'G', "Go To"}, //07 BEL
	{'H', "BS"},  //08
	{'I', "HT"},  //09
	{'J', "LF"},  //0A
	{'K', "Delete"}, //0B VT
	{'L', "FF"},  //0C
	{'M', "CR"},  //0D
	{'N', "New File"}, //0E
	{'O', "Open"}, //0F SI
	{'P', "DLE"}, //10
	{'Q', "Quit"}, //11 DC1
	{'R', "Revert"}, //12 DC2
	{'S', "Save"}, //13 DC3
	{'T', "DC4"}, //14
	{'U', "NAK"}, //15
	{'V', "Paste"}, //16 SYN
	{'W', "Close"}, //17 ETB
	{'X', "Cut"}, //18 CAN
	{'Y', "EM"},  //19
	{'Z', "Undo"}, //1A SUB
	{'[', "ESC"}, //1B
	{'\\', "FS"}, //1C
	{']', "GS"},  //1D
	{'^', "RS"},  //1E
	{'_', "US"},  //1F
};
