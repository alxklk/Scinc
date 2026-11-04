// Terminal services from term_dl.so. Build it first with ./build_so.sh
#ifndef TERM_H
#define TERM_H

[[scinc::dynlink("term_dl","term_open_wrapper")]]int term_open();
[[scinc::dynlink("term_dl","term_close_wrapper")]]int term_close();
[[scinc::dynlink("term_dl","term_read_wrapper")]]int term_read(int timeoutMs);
[[scinc::dynlink("term_dl","term_pending_wrapper")]]int term_pending();
[[scinc::dynlink("term_dl","term_cols_wrapper")]]int term_cols();
[[scinc::dynlink("term_dl","term_rows_wrapper")]]int term_rows();
[[scinc::dynlink("term_dl","term_time_wrapper")]]int term_time(int what);

[[scinc::dynlink("term_dl","fs_list_wrapper")]]int fs_list(char* path, char* resolved, int max, int showHidden);
[[scinc::dynlink("term_dl","fs_name_wrapper")]]int fs_name(int i, char* out, int max);
[[scinc::dynlink("term_dl","fs_info_wrapper")]]void fs_info(int i, char* out, int max);
[[scinc::dynlink("term_dl","fs_index_wrapper")]]int fs_index(char* name);
[[scinc::dynlink("term_dl","fs_find_wrapper")]]int fs_find(int letter, int start);
[[scinc::dynlink("term_dl","fs_home_wrapper")]]void fs_home(char* out, int max);

[[scinc::dynlink("term_dl","fs_error_wrapper")]]void fs_error(char* out, int max);
[[scinc::dynlink("term_dl","fs_abspath_wrapper")]]int fs_abspath(char* path, char* out, int max);
[[scinc::dynlink("term_dl","fs_size_wrapper")]]int fs_size(char* path);
[[scinc::dynlink("term_dl","fs_load_wrapper")]]int fs_load(char* path, char* buf, int max);
[[scinc::dynlink("term_dl","fs_stamp_wrapper")]]int fs_stamp(char* path);
[[scinc::dynlink("term_dl","fs_isdir_wrapper")]]int fs_isdir(char* path);
[[scinc::dynlink("term_dl","fs_save_begin_wrapper")]]int fs_save_begin(char* path);
[[scinc::dynlink("term_dl","fs_save_write_wrapper")]]int fs_save_write(char* data, int n);
[[scinc::dynlink("term_dl","fs_save_end_wrapper")]]int fs_save_end(int commit);

#endif // TERM_H
