// Terminal services from term_dl.so. Build it first with ./build_so.sh
#ifndef TERM_H
#define TERM_H

[[scinc::dynlink("term_dl","term_open_wrapper")]]int term_open();
[[scinc::dynlink("term_dl","term_close_wrapper")]]int term_close();
[[scinc::dynlink("term_dl","term_read_wrapper")]]int term_read(int timeoutMs);
[[scinc::dynlink("term_dl","term_cols_wrapper")]]int term_cols();
[[scinc::dynlink("term_dl","term_rows_wrapper")]]int term_rows();
[[scinc::dynlink("term_dl","term_time_wrapper")]]int term_time(int what);

[[scinc::dynlink("term_dl","fs_list_wrapper")]]int fs_list(char* path, char* resolved, int max, int showHidden);
[[scinc::dynlink("term_dl","fs_name_wrapper")]]int fs_name(int i, char* out, int max);
[[scinc::dynlink("term_dl","fs_info_wrapper")]]void fs_info(int i, char* out, int max);
[[scinc::dynlink("term_dl","fs_index_wrapper")]]int fs_index(char* name);
[[scinc::dynlink("term_dl","fs_find_wrapper")]]int fs_find(int letter, int start);
[[scinc::dynlink("term_dl","fs_home_wrapper")]]void fs_home(char* out, int max);

#endif // TERM_H
