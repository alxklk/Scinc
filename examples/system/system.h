#ifdef __SCINC__
#ifdef __linux__
[[scinc::dynlink("system_dl", "system_Wrapper")]]int system(const char* command);
#else
int system(const char* command);
#endif
#else
int system(const char*) { return 0; }
#endif
