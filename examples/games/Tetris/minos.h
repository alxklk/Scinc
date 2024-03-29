float rotCenter[25*2]=
{
	0.5,2.0,// 0 I
	1.5,1.5,// 1 J
	0.5,1.5,// 2 L
	1.0,1.0,// 3 O
	1.5,1.0,// 4 S
	1.5,0.5,// 5 T
	1.5,1.0 // 6 Z
	,
	0.0,0.0,//  7  5
	0.0,0.0,//  8  5
	0.0,0.0,//  9  5
	0.0,0.0,// 10  5
	0.0,0.0,// 11  5
	0.0,0.0,// 12  5
	0.0,0.0,// 13  5
	0.0,0.0,// 14  5
	0.0,0.0,// 15  5
	0.0,0.0,// 16  5
	0.0,0.0,// 17  5
	0.0,0.0,// 18  5
	0.0,0.0,// 19  5
	0.0,0.0,// 20  5
	0.0,0.0,// 21  5
	0.0,0.0,// 22  5
	0.0,0.0,// 23  5
	0.0,0.0 // 24  5
};
/*
int deltasNew0[25*4]=
{
	0, 0, 0, 0, // 0 I
	0, 1, 0, 0, // 1 J
	0, 1, 0, 0, // 2 L
	0, 0, 0, 0, // 3 O
	0, 1, 0, 1, // 4 S
	0, 1, 0, 0, // 5 T
	0, 1, 0, 1  // 6 Z
	,
	0, 0, 0, 0,//  7  5
	0, 0, 0, 0,//  8  5
	0, 0, 0, 0,//  9  5
	0, 0, 0, 0,// 10  5
	0, 0, 0, 0,// 11  5
	0, 0, 0, 0,// 12  5
	0, 0, 0, 0,// 13  5
	0, 0, 0, 0,// 14  5
	0, 0, 0, 0,// 15  5
	0, 0, 0, 0,// 16  5
	0, 0, 0, 0,// 17  5
	0, 0, 0, 0,// 18  5
	0, 0, 0, 0,// 19  5
	0, 0, 0, 0,// 20  5
	0, 0, 0, 0,// 21  5
	0, 0, 0, 0,// 22  5
	0, 0, 0, 0,// 23  5
	0, 0, 0, 0 // 24  5

};
*/
int offsets[25*8]=
{
	  0,  1, -2, -1,  0, -2,  1, -1,  // 0 I
	 -1,  0, -1, -2,  1, -2,  1,  0,  // 1 J
	  0,  0, -1, -1,  0, -2,  1, -1,  // 2 L
	 -1,  0, -1, -1,  0, -1,  0,  0,  // 3 O
	 -1,  0, -1, -2,  1, -1,  0,  0,  // 4 S
	 -1, -1,  0, -2,  1, -1,  0,  0,  // 5 T
	 -1,  0, -1, -2,  1, -1,  0,  0   // 6 Z
	,
	  0,  1, -2, -1,  0, -3,  2, -1, //  7  5
	 -1,  0, -1, -1,  0, -2,  1,  0, //  8  5
	 -1,  0, -1, -1,  0, -2,  1,  0, //  9  5
	 -1,  1, -2, -1,  0, -2,  1,  0, // 10  5
	 -1,  1, -2, -1,  0, -2,  1,  0, // 11  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 12  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 13  5
	 -2,  0, -1, -2,  1, -1,  0,  1, // 14  5
	 -2,  0, -1, -2,  1, -1,  0,  1, // 15  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 16  5
	 -1,  0, -1, -2,  1, -1,  0,  0, // 17  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 18  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 19  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 20  5
	 -1,  1, -2, -1,  0, -2,  1,  0, // 21  5
	 -1,  1, -2, -1,  0, -2,  1,  0, // 22  5
	 -1,  0, -1, -2,  1, -2,  1,  0, // 23  5
	 -1,  0, -1, -2,  1, -2,  1,  0  // 24  5
 };

int boxes[25*2]=
{
	1, 4,  //  0  I
	2, 3,  //  1  J
	2, 3,  //  2  L
	2, 2,  //  3  O
	3, 2,  //  4  S
	3, 2,  //  5  T
	3, 2   //  6  Z
	,
	0, 0,  //  7  5
	0, 0,  //  8  5
	0, 0,  //  9  5
	0, 0,  // 10  5
	0, 0,  // 11  5
	0, 0,  // 12  5
	0, 0,  // 13  5
	0, 0,  // 14  5
	0, 0,  // 15  5
	0, 0,  // 16  5
	0, 0,  // 17  5
	0, 0,  // 18  5
	0, 0,  // 19  5
	0, 0,  // 20  5
	0, 0,  // 21  5
	0, 0,  // 22  5
	0, 0,  // 23  5
	0, 0   // 24  5
};

char* figtpl=//IJLOSTZ
	"1    " //0 I
	"1    "
	"1    "
	"1    "
	"     "

	" 1   " //1 J
	" 1   "
	"11   "
	"     "
	"     "

	"1    " //2 L
	"1    "
	"11   "
	"     "
	"     "

	"11   " //3 O
	"11   "
	"     "
	"     "
	"     "

	" 11  " //4 S
	"11   "
	"     "
	"     "
	"     "

	"111  " //5 T
	" 1   "
	"     "
	"     "
	"     "

	"11   " //6 Z
	" 11  "
	"     "
	"     "
	"     "

	"1    " //7 5 O
	"1    "
	"1    "
	"1    "
	"1    "

	"11   " //8 5 P
	"11   "
	"1    "
	"     "
	"     "

	"11   " //9 5 P m
	"11   "
	" 1   "
	"     "
	"     "

	"11   " //10 5 q
	" 1   "
	" 1   "
	" 1   "
	"     "

	"11   " //11 5 q m
	"1    "
	"1    "
	"1    "
	"     "

	" 11  " //12 5 R
	"11   "
	" 1   "
	"     "
	"     "

	"11   " //13 5 R m
	" 11  "
	" 1   "
	"     "
	"     "

	"  11 " //14 5 S
	"111  "
	"     "
	"     "
	"     "

	"11   " //15 5 S m
	" 111 "
	"     "
	"     "
	"     "

	"111  " //16 5 T
	" 1   "
	" 1   "
	"     "
	"     "

	"1 1  " //17 5 U
	"111  "
	"     "
	"     "
	"     "

	"111  " //18 5 V
	"1    "
	"1    "
	"     "
	"     "

	" 11  " //19 5 W
	"11   "
	"1    "
	"     "
	"     "

	" 1   " //20 5 X
	"111  "
	" 1   "
	"     "
	"     "

	" 1   " //21 5 Y
	"11   "
	" 1   "
	" 1   "
	"     "

	"1    " //22 5 Y m
	"11   "
	"1    "
	"1    "
	"     "

	"11   " //23 5 Z
	" 1   "
	" 11  "
	"     "
	"     "

	" 11  " //24 5 Z m
	" 1   "
	"11   "
	"     "
	"     "
;
