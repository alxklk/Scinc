#ifdef __SCINC__

#ifdef __linux__
__dyn_lib_import__("alsa_mic_in_dl","MIC_In_Init_Wrapper"  )int MIC_In_Init(int bufLen);
__dyn_lib_import__("alsa_mic_in_dl","MIC_In_Record_Wrapper")int MIC_In_Record(float* buf);
__dyn_lib_import__("alsa_mic_in_dl","MIC_In_Done_Wrapper"  )int MIC_In_Done();
#endif

#ifdef __APPLE__
__dyn_lib_import__("mac_mic_in_dl","MIC_In_Init_Wrapper"  )int MIC_In_Init(int bufLen);
__dyn_lib_import__("mac_mic_in_dl","MIC_In_Record_Wrapper")int MIC_In_Record(float* buf);
__dyn_lib_import__("mac_mic_in_dl","MIC_In_Done_Wrapper"  )int MIC_In_Done();
#endif

#ifdef _WIN32
__dyn_lib_import__("winmm_mic_in_dl","MIC_In_Init_Wrapper"  )int MIC_In_Init(int bufLen);
__dyn_lib_import__("winmm_mic_in_dl","MIC_In_Record_Wrapper")int MIC_In_Record(float* buf);
__dyn_lib_import__("winmm_mic_in_dl","MIC_In_Done_Wrapper"  )int MIC_In_Done();
#endif

#else
int MIC_In_Init(int bufLen){return 0;}
int MIC_In_Record(float*){return 0;}
int MIC_In_Done(){return 0;}
#endif
