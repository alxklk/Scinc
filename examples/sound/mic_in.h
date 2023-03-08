#ifdef __SCINC__

#ifdef __linux__
[[scinc::dynlink("alsa_mic_in_dl","MIC_In_Init_Wrapper"  )]]int MIC_In_Init(int bufLen);
[[scinc::dynlink("alsa_mic_in_dl","MIC_In_Record_Wrapper")]]int MIC_In_Record(float* buf);
[[scinc::dynlink("alsa_mic_in_dl","MIC_In_Done_Wrapper"  )]]int MIC_In_Done();
#endif

#ifdef __APPLE__
[[scinc::dynlink("mac_mic_in_dl","MIC_In_Init_Wrapper"  )]]int MIC_In_Init(int bufLen);
[[scinc::dynlink("mac_mic_in_dl","MIC_In_Record_Wrapper")]]int MIC_In_Record(float* buf);
[[scinc::dynlink("mac_mic_in_dl","MIC_In_Done_Wrapper"  )]]int MIC_In_Done();
#endif

#ifdef _WIN32
[[scinc::dynlink("winmm_mic_in_dl","MIC_In_Init_Wrapper"  )]]int MIC_In_Init(int bufLen);
[[scinc::dynlink("winmm_mic_in_dl","MIC_In_Record_Wrapper")]]int MIC_In_Record(float* buf);
[[scinc::dynlink("winmm_mic_in_dl","MIC_In_Done_Wrapper"  )]]int MIC_In_Done();
#endif

#else
int MIC_In_Init(int bufLen){return 0;}
int MIC_In_Record(float*){return 0;}
int MIC_In_Done(){return 0;}
#endif
