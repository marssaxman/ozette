#include "control.h"

Control::Info Control::keys[Count] = {
	{'@', "NUL"}, //00
	{'A', "SOH"}, //01
	{'B', "STX"}, //02
	{'C', "Copy"}, //03 ETX
	{'D', "EOT"}, //04
	{'E', "ENQ"}, //05
	{'F', "ACK"}, //06
	{'G', "BEL"}, //07
	{'H', "BS"},  //08
	{'I', "HT"},  //09
	{'J', "LF"},  //0A
	{'K', "VT"},  //0B
	{'L', "FF"},  //0C
	{'M', "CR"},  //0D
	{'N', "SO"},  //0E
	{'O', "SI"},  //0F
	{'P', "DLE"}, //10
	{'Q', "DC1"}, //11
	{'R', "DC2"}, //12
	{'S', "DC3"}, //13
	{'T', "DC4"}, //14
	{'U', "NAK"}, //15
	{'V', "Paste"}, //16 SYN
	{'W', "ETB"}, //17
	{'X', "Cut"}, //18 CAN
	{'Y', "EM"},  //19
	{'Z', "SUB"}, //1A
	{'[', "ESC"}, //1B
	{'\\', "FS"}, //1C
	{']', "GS"},  //1D
	{'^', "RS"},  //1E
	{'_', "US"},  //1F
};
