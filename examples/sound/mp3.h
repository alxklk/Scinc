#ifdef __SCINC__
[[scinc::dynlink("mp3_dl","open_dec_wrapper")]]int open_dec(char *file_name);
[[scinc::dynlink("mp3_dl","decode_samples_wrapper")]]void decode_samples(void* buf, int bytes);
[[scinc::dynlink("mp3_dl","close_dec_wrapper")]]int close_dec();
#else
int open_dec(char *file_name){return 0;};
void decode_samples(void* buf, int bytes){};
int close_dec(){return 0;};
#endif
