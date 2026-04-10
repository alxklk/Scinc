#ifdef __SCINC__
#ifdef __linux__
[[scinc::dynlink("opencv_cam_in_dl","CAM_In_Init_Wrapper")]]int CAM_In_Init(int width, int height, int deviceIndex);
[[scinc::dynlink("opencv_cam_in_dl","CAM_In_Capture_Wrapper")]]int CAM_In_Capture(int* buf);
[[scinc::dynlink("opencv_cam_in_dl","CAM_In_Done_Wrapper")]]int CAM_In_Done();
#else
int CAM_In_Init(int width, int height, int deviceIndex);
int CAM_In_Capture(int* buf);
int CAM_In_Done();
#endif
#else
int CAM_In_Init(int, int, int){return 0;}
int CAM_In_Capture(int*){return 0;}
int CAM_In_Done(){return 0;}
#endif
