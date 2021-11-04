#include "gfx.h"
#include "gpio.h"
#include "board.h"

/*
 * SSD1306.c
 * Author: Harris Shallcross
 * Year: 2014-ish
 *
 * SSD1306 driver library using the STM32F0 discovery board
 * STM32F0 communicates with OLED display through SPI, pinouts described below.
 *
 * This code is provided AS IS and no warranty is included!
 *
 * Character array below was provided by Adafruit and other than a few added characters,
 * will remain theirs.
 */

const char Chars[] =
{
		0x00, 0x00, 0x00, 0x00, 0x00 // 20
		,0x00, 0x00, 0x5f, 0x00, 0x00 // 21 !
		,0x00, 0x07, 0x00, 0x07, 0x00 // 22 "
		,0x14, 0x7f, 0x14, 0x7f, 0x14 // 23 #
		,0x24, 0x2a, 0x7f, 0x2a, 0x12 // 24 $
		,0x23, 0x13, 0x08, 0x64, 0x62 // 25 %
		,0x36, 0x49, 0x55, 0x22, 0x50 // 26 &
		,0x00, 0x05, 0x03, 0x00, 0x00 // 27 '
		,0x00, 0x1c, 0x22, 0x41, 0x00 // 28 (
		,0x00, 0x41, 0x22, 0x1c, 0x00 // 29 )
		,0x14, 0x08, 0x3e, 0x08, 0x14 // 2a *
		,0x08, 0x08, 0x3e, 0x08, 0x08 // 2b +
		,0x00, 0x50, 0x30, 0x00, 0x00 // 2c ,
		,0x08, 0x08, 0x08, 0x08, 0x08 // 2d -
		,0x00, 0x60, 0x60, 0x00, 0x00 // 2e .
		,0x20, 0x10, 0x08, 0x04, 0x02 // 2f /
		,0x3e, 0x51, 0x49, 0x45, 0x3e // 30 0
		,0x00, 0x42, 0x7f, 0x40, 0x00 // 31 1
		,0x42, 0x61, 0x51, 0x49, 0x46 // 32 2
		,0x21, 0x41, 0x45, 0x4b, 0x31 // 33 3
		,0x18, 0x14, 0x12, 0x7f, 0x10 // 34 4
		,0x27, 0x45, 0x45, 0x45, 0x39 // 35 5
		,0x3c, 0x4a, 0x49, 0x49, 0x30 // 36 6
		,0x01, 0x71, 0x09, 0x05, 0x03 // 37 7
		,0x36, 0x49, 0x49, 0x49, 0x36 // 38 8
		,0x06, 0x49, 0x49, 0x29, 0x1e // 39 9
		,0x00, 0x36, 0x36, 0x00, 0x00 // 3a :
		,0x00, 0x56, 0x36, 0x00, 0x00 // 3b ;
		,0x08, 0x14, 0x22, 0x41, 0x00 // 3c <
		,0x14, 0x14, 0x14, 0x14, 0x14 // 3d =
		,0x00, 0x41, 0x22, 0x14, 0x08 // 3e >
		,0x02, 0x01, 0x51, 0x09, 0x06 // 3f ?
		,0x32, 0x49, 0x79, 0x41, 0x3e // 40 @
		,0x7e, 0x11, 0x11, 0x11, 0x7e // 41 A
		,0x7f, 0x49, 0x49, 0x49, 0x36 // 42 B
		,0x3e, 0x41, 0x41, 0x41, 0x22 // 43 C
		,0x7f, 0x41, 0x41, 0x22, 0x1c // 44 D
		,0x7f, 0x49, 0x49, 0x49, 0x41 // 45 E
		,0x7f, 0x09, 0x09, 0x09, 0x01 // 46 F
		,0x3e, 0x41, 0x49, 0x49, 0x7a // 47 G
		,0x7f, 0x08, 0x08, 0x08, 0x7f // 48 H
		,0x00, 0x41, 0x7f, 0x41, 0x00 // 49 I
		,0x20, 0x40, 0x41, 0x3f, 0x01 // 4a J
		,0x7f, 0x08, 0x14, 0x22, 0x41 // 4b K
		,0x7f, 0x40, 0x40, 0x40, 0x40 // 4c L
		,0x7f, 0x02, 0x0c, 0x02, 0x7f // 4d M
		,0x7f, 0x04, 0x08, 0x10, 0x7f // 4e N
		,0x3e, 0x41, 0x41, 0x41, 0x3e // 4f O
		,0x7f, 0x09, 0x09, 0x09, 0x06 // 50 P
		,0x3e, 0x41, 0x51, 0x21, 0x5e // 51 Q
		,0x7f, 0x09, 0x19, 0x29, 0x46 // 52 R
		,0x46, 0x49, 0x49, 0x49, 0x31 // 53 S
		,0x01, 0x01, 0x7f, 0x01, 0x01 // 54 T
		,0x3f, 0x40, 0x40, 0x40, 0x3f // 55 U
		,0x1f, 0x20, 0x40, 0x20, 0x1f // 56 V
		,0x3f, 0x40, 0x38, 0x40, 0x3f // 57 W
		,0x63, 0x14, 0x08, 0x14, 0x63 // 58 X
		,0x07, 0x08, 0x70, 0x08, 0x07 // 59 Y
		,0x61, 0x51, 0x49, 0x45, 0x43 // 5a Z
		,0x00, 0x7f, 0x41, 0x41, 0x00 // 5b [
		,0x02, 0x04, 0x08, 0x10, 0x20 // 5c
		,0x00, 0x41, 0x41, 0x7f, 0x00 // 5d ]
		,0x04, 0x02, 0x01, 0x02, 0x04 // 5e ^
		,0x40, 0x40, 0x40, 0x40, 0x40 // 5f _
		,0x00, 0x01, 0x02, 0x04, 0x00 // 60 `
		,0x20, 0x54, 0x54, 0x54, 0x78 // 61 a
		,0x7f, 0x48, 0x44, 0x44, 0x38 // 62 b
		,0x38, 0x44, 0x44, 0x44, 0x20 // 63 c
		,0x38, 0x44, 0x44, 0x48, 0x7f // 64 d
		,0x38, 0x54, 0x54, 0x54, 0x18 // 65 e
		,0x08, 0x7e, 0x09, 0x01, 0x02 // 66 f
		,0x0c, 0x52, 0x52, 0x52, 0x3e // 67 g
		,0x7f, 0x08, 0x04, 0x04, 0x78 // 68 h
		,0x00, 0x44, 0x7d, 0x40, 0x00 // 69 i
		,0x20, 0x40, 0x44, 0x3d, 0x00 // 6a j
		,0x7f, 0x10, 0x28, 0x44, 0x00 // 6b k
		,0x00, 0x41, 0x7f, 0x40, 0x00 // 6c l
		,0x7c, 0x04, 0x18, 0x04, 0x78 // 6d m
		,0x7c, 0x08, 0x04, 0x04, 0x78 // 6e n
		,0x38, 0x44, 0x44, 0x44, 0x38 // 6f o
		,0x7c, 0x14, 0x14, 0x14, 0x08 // 70 p
		,0x08, 0x14, 0x14, 0x18, 0x7c // 71 q
		,0x7c, 0x08, 0x04, 0x04, 0x08 // 72 r
		,0x48, 0x54, 0x54, 0x54, 0x20 // 73 s
		,0x04, 0x3f, 0x44, 0x40, 0x20 // 74 t
		,0x3c, 0x40, 0x40, 0x20, 0x7c // 75 u
		,0x1c, 0x20, 0x40, 0x20, 0x1c // 76 v
		,0x3c, 0x40, 0x30, 0x40, 0x3c // 77 w
		,0x44, 0x28, 0x10, 0x28, 0x44 // 78 x
		,0x0c, 0x50, 0x50, 0x50, 0x3c // 79 y
		,0x44, 0x64, 0x54, 0x4c, 0x44 // 7a z
		,0x00, 0x08, 0x36, 0x41, 0x00 // 7b {
		,0x00, 0x00, 0x7f, 0x00, 0x00 // 7c |
		,0x00, 0x41, 0x36, 0x08, 0x00 // 7d }
		,0x10, 0x08, 0x08, 0x10, 0x08 // 7e
		,0x78, 0x46, 0x41, 0x46, 0x78 // 7f
		,0xFF, 0x00, 0x00, 0x00, 0x00 // Current char line
};

const int8_t ST[256] =
{
		0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34,
		37, 40, 43, 46, 49, 51, 54, 57, 60, 63, 65, 68,
		71, 73, 76, 78, 81, 83, 85, 88, 90, 92, 94, 96,
		98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
		117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126,
		126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 125,
		125, 124, 123, 122, 122, 121, 120, 118, 117, 116, 115, 113,
		112, 111, 109, 107, 106, 104, 102, 100, 98, 96, 94, 92,
		90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63,
		60, 57, 54, 51, 49, 46, 43, 40, 37, 34, 31, 28,
		25, 22, 19, 16, 12, 9, 6, 3, 0, -3, -6, -9,
		-12, -16, -19, -22, -25, -28, -31, -34, -37, -40, -43, -46,
		-49, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78,
		-81, -83, -85, -88, -90, -92, -94, -96, -98, -100, -102, -104,
		-106, -107, -109, -111, -112, -113, -115, -116, -117, -118, -120, -121,
		-122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
		-127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122,
		-122, -121, -120, -118, -117, -116, -115, -113, -112, -111, -109, -107,
		-106, -104, -102, -100, -98, -96, -94, -92, -90, -88, -85, -83,
		-81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
		-49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16,
		-12, -9, -6, -3
};

uint8_t GBuf[GBufS];
void PScrn(void){
	uint16_t Cnt;

	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	for(Cnt = 0; Cnt<((XPix*YPix)/8); Cnt++){
		SB(GBuf[Cnt], Dat, 0);
	}
	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void ClrBuf(void){
	memset(GBuf, 0, (XPix*YPix)/8);
}

void DispMode(uint8_t Mode){
	if(Mode == InvDisp) SB(InvDisp, 0, 1);
	else SB(NormDisp, 0, 1);
}

uint8_t WritePix(int16_t X, int16_t Y, PixT V){
	if(X>XPix-1 || X<0 || Y>YPix-1 || Y<0) return 1;

	int16_t Index;
	uint8_t CPix;

	Index = X+((Y>>3)*XPix);
	CPix = GBuf[Index];

	if(V == PixNorm) CPix |= 1<<(Y%8);
	else CPix &= ~(1<<(Y%8));

	GBuf[Index] = CPix;

	return 0;
}

uint8_t SetPix(uint8_t X, uint8_t Y){
	return WritePix(X, Y, PixNorm);
}

uint8_t ClrPix(uint8_t X, uint8_t Y){
	return WritePix(X, Y, PixInv);
}

uint8_t Circle(uint8_t XS, uint8_t YS, uint8_t R, PixT V){
	if((XS+R)>XPix || (YS+R)>YPix) return 1;

	int X, Y, D, X2M1;
	Y = R;
	D = -R;
	X2M1 = -1;

	for(X = 0; X<R/sqrt(2); X++){
		X2M1+=2;
		D+=X2M1;
		if(D>=0){
			Y--;
			D-= (Y<<1);
		}
		WritePix(XS+X, YS+Y, V);
		WritePix(XS-X, YS+Y, V);
		WritePix(XS+X, YS-Y, V);
		WritePix(XS-Y, YS-Y, V);
		WritePix(XS+Y, YS+X, V);
		WritePix(XS-Y, YS+X, V);
		WritePix(XS+Y, YS-X, V);
		WritePix(XS-Y, YS-X, V);
	}
	return 0;
}

uint8_t Semicircle(uint8_t XS, uint8_t YS, uint8_t R, uint8_t Rot, PixT V){
	if((XS+R)>XPix || (YS+R)>YPix) return 1;

	int X, Y, D, X2M1;
	Y = R;
	D = -R;
	X2M1 = -1;

	for(X = 0; X<R/sqrt(2); X++){
		X2M1+=2;
		D+=X2M1;
		if(D>=0){
			Y--;
			D-= (Y<<1);
		}
		switch(Rot){
		case 0:
			WritePix(XS+X, YS-Y, V);
			WritePix(XS-X, YS-Y, V);
			WritePix(XS+Y, YS-X, V);
			WritePix(XS-Y, YS-X, V);
			break;

		case 1:
			WritePix(XS+Y, YS+X, V);
			WritePix(XS+Y, YS-X, V);
			WritePix(XS+X, YS+Y, V);
			WritePix(XS+X, YS-Y, V);
			break;

		case 2:
			WritePix(XS+X, YS+Y, V);
			WritePix(XS-X, YS+Y, V);
			WritePix(XS+Y, YS+X, V);
			WritePix(XS-Y, YS+X, V);
			break;

		case 3:
			WritePix(XS-Y, YS+X, V);
			WritePix(XS-Y, YS-X, V);
			WritePix(XS-X, YS+Y, V);
			WritePix(XS-X, YS-Y, V);
			break;
		}
	}
	switch(Rot){
	case 0:
	case 2:
		LineL(XS-(R-1), YS, XS+(R), YS, V);
		break;
	case 1:
	case 3:
		LineL(XS, YS-(R-1), XS, YS+R, V);
		break;
	}
	return 0;
}

uint8_t Square(uint8_t XS, uint8_t YS, uint8_t XE, uint8_t YE, PixT V){
	if((XE-XS)>XPix-1 || (YE-YS)>YPix-1) return 1;
	if(XS==XE || YS==YE) return 2;
	uint8_t XCnt, YCnt;
	for(XCnt = XS; XCnt<=XE; XCnt++){
		WritePix(XCnt, YS, V);
		WritePix(XCnt, YE, V);
	}
	for(YCnt = YS; YCnt<=YE; YCnt++){
		WritePix(XS, YCnt, V);
		WritePix(XE, YCnt, V);
	}
	return 0;
}

uint8_t LineP(uint8_t XS, uint8_t YS, uint8_t R, int16_t Angle, PixT V){
	uint8_t XE, YE;

	while(Angle>360) Angle-=360;
	XE = XS+((R*ST[((Angle+90)*256/360)&255])>>7);
	YE = YS+((R*ST[(Angle*256/360)&255])>>7);
	return LineL(XS, YS, XE, YE, V);
}

uint8_t LineL(uint8_t XS, uint8_t YS, uint8_t XE, uint8_t YE, PixT V){
	if(abs(XE-XS)>XPix-1 || abs(YE-YS)>YPix-1) return 1;
	int Cnt, Distance;
	int XErr = 0, YErr = 0, dX, dY;
	int XInc, YInc;

	dX = XE-XS;
	dY = YE-YS;

	if(dX>0) XInc = 1;
	else if(dX==0) XInc = 0;
	else XInc = -1;

	if(dY>0) YInc = 1;
	else if(dY==0) YInc = 0;
	else YInc = -1;

	dX = abs(dX);
	dY = abs(dY);

	if(dX>dY) Distance = dX;
	else Distance = dY;

	for(Cnt = 0; Cnt<=Distance+1; Cnt++){
		WritePix(XS, YS, V);

		XErr+=dX;
		YErr+=dY;
		if(XErr>Distance){
			XErr-=Distance;
			XS+=XInc;
		}
		if(YErr>Distance){
			YErr-=Distance;
			YS+=YInc;
		}
	}
	return 0;
}

/*
int PChar(uint16_t CH, int16_t X, int16_t Y, uint8_t Inv){
	uint8_t Cnt;
	if(X<0) return -1;

	X = (X/5)*5; //Ensure its multiple of 5
	Y>>=3;

	CH-=0x20;
	CH*=5;

	for(Cnt = 0; Cnt<5; Cnt++){
		if(Inv) GBuf[X+Cnt+((Y)*XPix)] = ~Chars[CH+Cnt];
		else GBuf[X+Cnt+((Y)*XPix)] = Chars[CH+Cnt];
	}
	return X+5;
}
 */

int PChar(uint16_t Ch, int16_t X, int16_t Y, uint8_t Size, PixT Inv){
	//if(X>XPix-(Size+1)*5 || Y>YPix-(Size+1)*8) return -1;

	uint16_t XCnt, YCnt, CChar;

	Ch-=0x20;
	Ch*=5;

	switch(Size){
	case 0:
		for(XCnt = 0; XCnt<5; XCnt++){
			CChar = Chars[Ch+XCnt];
			for(YCnt = 0; YCnt<8; YCnt++){
				if(CChar & (1<<YCnt)) WritePix(X+XCnt, Y+YCnt, Inv==PixInv);
				else WritePix(X+XCnt, Y+YCnt, Inv==PixNorm);
			}
		}
		break;
	case 1:
		for(XCnt = 0; XCnt<10; XCnt++){
			for(YCnt = 0; YCnt<16; YCnt++){
				if(Chars[Ch+(XCnt>>1)] & (1<<(YCnt>>1))) WritePix(X+XCnt, Y+YCnt, Inv==PixInv);
				else WritePix(X+XCnt, Y+YCnt, Inv==PixNorm);
			}
		}
		break;
	case 2:
		for(XCnt = 0; XCnt<15; XCnt++){
			for(YCnt = 0; YCnt<24; YCnt++){
				if(Chars[Ch+(XCnt/3)] & (1<<(YCnt/3))) WritePix(X+XCnt, Y+YCnt, Inv==PixInv);
				else WritePix(X+XCnt, Y+YCnt, Inv==PixNorm);
			}
		}
		break;
	}

	return X+(1+Size)*5+LetterSpace;
}

int PStr(const char* Str, int16_t X, int16_t Y, uint8_t Size, PixT Inv){
	uint8_t SCnt, StrL;

	StrL = strlen(Str);

	switch(Size){
	case 0:
		if(X>XPix - StrL*(5+LetterSpace) || Y>YPix-8) return -1;

		for(SCnt = 0; SCnt<StrL; SCnt++){
			PChar(Str[SCnt], X+SCnt*(5+LetterSpace), Y, Size, Inv);
		}
		break;
	case 1:
		if(X>XPix - StrL*(10+LetterSpace) || Y>YPix-16) return -1;

		for(SCnt = 0; SCnt<StrL; SCnt++){
			PChar(Str[SCnt], X+SCnt*(10+LetterSpace), Y, Size, Inv);
		}
		break;
	case 2:
		if(X>XPix - StrL*(15+LetterSpace) || Y>YPix-24) return -1;

		for(SCnt = 0; SCnt<StrL; SCnt++){
			PChar(Str[SCnt], X+SCnt*(15+LetterSpace), Y, Size, Inv);
		}
	}
	return X+StrL*(5*(Size+1)+LetterSpace);
}

int PNum(int Num, int16_t X, int16_t Y, uint8_t Pad, uint8_t Size, PixT Inv){
	char NBuf[10];
	uint8_t Cnt, Len = 0, Sign;

	memset(NBuf, 0, 9);

	if(X<0) return -2;

	if(Num<0){
		Num = -Num;
		Sign = 1;
	}
	else {
		Sign = 0;
	}

	if(Num<10){
		NBuf[0] = Num + '0';
		Len = 1;
	}
	else if(Num<100){
		NBuf[0] = Num/10 + '0';
		NBuf[1] = Num % 10  + '0';
		Len = 2;
	}
	else if(Num<1000){
		NBuf[0] = Num/100 + '0';
		NBuf[1] = Num/10 % 10 + '0';
		NBuf[2] = Num % 10 + '0';
		Len = 3;
	}
	else if(Num<10000){
		NBuf[0] = Num/1000 + '0';
		NBuf[1] = Num/100 % 10 + '0';
		NBuf[2] = Num/10 % 10 + '0';
		NBuf[3] = Num % 10 + '0';
		Len = 4;
	}
	else if(Num<100000){
		NBuf[0] = Num/10000 + '0';
		NBuf[1] = Num/1000 % 10 + '0';
		NBuf[2] = Num/100 % 10 + '0';
		NBuf[3] = Num/10 % 10 + '0';
		NBuf[4] = Num % 10 + '0';
		Len = 5;
	}
	else if(Num<1000000){
		NBuf[0] = Num/100000 + '0';
		NBuf[1] = Num/10000 % 10 + '0';
		NBuf[2] = Num/1000 % 10 + '0';
		NBuf[3] = Num/100 % 10 + '0';
		NBuf[4] = Num/10 % 10 + '0';
		NBuf[5] = Num % 10 + '0';
		Len = 6;
	}
	else if(Num<10000000){
		NBuf[0] = Num/1000000 + '0';
		NBuf[1] = Num/100000 % 10 + '0';
		NBuf[2] = Num/10000 % 10 + '0';
		NBuf[3] = Num/1000 % 10 + '0';
		NBuf[4] = Num/100 % 10 + '0';
		NBuf[5] = Num/10 % 10 + '0';
		NBuf[6] = Num % 10 + '0';
		Len = 7;
	}
	else if(Num<100000000){
		NBuf[0] = Num/10000000 + '0';
		NBuf[1] = Num/1000000 % 10 + '0';
		NBuf[2] = Num/100000 % 10 + '0';
		NBuf[3] = Num/10000 % 10 + '0';
		NBuf[4] = Num/1000 % 10 + '0';
		NBuf[5] = Num/100 % 10 + '0';
		NBuf[6] = Num/10 % 10 + '0';
		NBuf[7] = Num % 10 + '0';
		Len = 8;
	}
	else if(Num<1000000000){
		NBuf[0] = Num/100000000 + '0';
		NBuf[1] = Num/10000000 % 10 + '0';
		NBuf[2] = Num/1000000 % 10 + '0';
		NBuf[3] = Num/100000 % 10 + '0';
		NBuf[4] = Num/10000 % 10 + '0';
		NBuf[5] = Num/1000 % 10 + '0';
		NBuf[6] = Num/100 % 10 + '0';
		NBuf[7] = Num/10 % 10 + '0';
		NBuf[8] = Num % 10 + '0';
		Len = 9;
	}

	switch(Size){
	case 0:
		if(X>XPix - Len*(5+LetterSpace) || Y>YPix-8) return -1;

		if(Sign == 1){
			PChar('-', X, Y, 0, Inv);
			X+=5;
		}
		for(Cnt = Len-1; Cnt<Pad; Cnt++){
			PChar('0', X, Y, 0, Inv);
			X+=5;
		}
		for(Cnt = 0; Cnt<Len; Cnt++){
			PChar(NBuf[Cnt], X, Y, 0, Inv);
			X+=5;
		}
		break;
	case 1:
		if(X>XPix - Len*(10+LetterSpace) || Y>YPix-16) return -1;

		if(Sign == 1){
			PChar('-', X, Y, 1, Inv);
			X+=10;
		}
		for(Cnt = Len-1; Cnt<Pad; Cnt++){
			PChar('0', X, Y, 1, Inv);
			X+=10;
		}
		for(Cnt = 0; Cnt<Len; Cnt++){
			PChar(NBuf[Cnt], X, Y, 1, Inv);
			X+=10;
		}
		break;
	case 2:
		if(X>XPix - Len*(15+LetterSpace) || Y>YPix-24) return -1;

		if(Sign == 1){
			PChar('-', X, Y, 2, Inv);
			X+=15;
		}
		for(Cnt = Len-1; Cnt<Pad; Cnt++){
			PChar('0', X, Y, 2, Inv);
			X+=15;
		}
		for(Cnt = 0; Cnt<Len; Cnt++){
			PChar(NBuf[Cnt], X, Y, 2, Inv);
			X+=15;
		}
		break;
	}
	return X;
}

int PNumF(float Num, int16_t X, int16_t Y, uint8_t Prec, uint8_t Size, PixT Inv){
	int32_t IPart, FracI;
	float Frac;
	uint8_t XPos = 0, Sign;

	if(Num<0){
		Sign = 1;
		Num = -Num;
	}
	else Sign = 0;

	IPart = Num;

	if(Sign) XPos = PNum(-IPart, X, Y, 0, Size, Inv);
	else XPos = PNum(IPart, X, Y, 0, Size, Inv);

	if(Prec>0){
		Frac = Num - IPart;
		switch(Prec){
		case 1:
			FracI = (int32_t)(Frac*10);
			break;
		case 2:
			FracI = (int32_t)(Frac*100);
			break;
		case 3:
			FracI = (int32_t)(Frac*1000);
			break;
		case 4:
			FracI = (int32_t)(Frac*10000);
			break;
		default:
		case 5:
			FracI = (int32_t)(Frac*100000);
			break;
		}

		XPos = PChar('.', XPos, Y, Size, Inv);
		XPos = PNum(FracI, XPos, Y, 0, Size, Inv);
	}

	return XPos;
}

int32_t FPow(int32_t Num, int32_t Pow){
	uint8_t Cnt;
	for(Cnt = 0; Cnt<Pow; Cnt++){
		Num*=Num;
	}
	return Num;
}
