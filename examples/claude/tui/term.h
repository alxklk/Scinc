// Terminal services from term_dl.so. Build it first with ./build_so.sh
// A program run from another folder defines TERM_DL as the path to it, as in
//   #define TERM_DL "../../claude/tui/term_dl"
#ifndef TERM_H
#define TERM_H

#ifndef TERM_DL
#define TERM_DL "term_dl"
#endif

[[scinc::dynlink(TERM_DL,"term_open_wrapper")]]int term_open();
[[scinc::dynlink(TERM_DL,"term_close_wrapper")]]int term_close();
[[scinc::dynlink(TERM_DL,"term_read_wrapper")]]int term_read(int timeoutMs);
[[scinc::dynlink(TERM_DL,"term_pending_wrapper")]]int term_pending();
[[scinc::dynlink(TERM_DL,"term_cols_wrapper")]]int term_cols();
[[scinc::dynlink(TERM_DL,"term_rows_wrapper")]]int term_rows();
[[scinc::dynlink(TERM_DL,"term_time_wrapper")]]int term_time(int what);
[[scinc::dynlink(TERM_DL,"term_ms_wrapper")]]int term_ms();
[[scinc::dynlink(TERM_DL,"term_truecolor_wrapper")]]int term_truecolor();

[[scinc::dynlink(TERM_DL,"fs_list_wrapper")]]int fs_list(char* path, char* resolved, int max, int showHidden);
[[scinc::dynlink(TERM_DL,"fs_name_wrapper")]]int fs_name(int i, char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_info_wrapper")]]void fs_info(int i, char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_index_wrapper")]]int fs_index(char* name);
[[scinc::dynlink(TERM_DL,"fs_find_wrapper")]]int fs_find(int letter, int start);
[[scinc::dynlink(TERM_DL,"fs_home_wrapper")]]void fs_home(char* out, int max);

[[scinc::dynlink(TERM_DL,"fs_error_wrapper")]]void fs_error(char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_abspath_wrapper")]]int fs_abspath(char* path, char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_size_wrapper")]]int fs_size(char* path);
[[scinc::dynlink(TERM_DL,"fs_load_wrapper")]]int fs_load(char* path, char* buf, int max);
[[scinc::dynlink(TERM_DL,"fs_stamp_wrapper")]]int fs_stamp(char* path);
[[scinc::dynlink(TERM_DL,"fs_isdir_wrapper")]]int fs_isdir(char* path);
[[scinc::dynlink(TERM_DL,"fs_save_begin_wrapper")]]int fs_save_begin(char* path);
[[scinc::dynlink(TERM_DL,"fs_save_write_wrapper")]]int fs_save_write(char* data, int n);
[[scinc::dynlink(TERM_DL,"fs_save_end_wrapper")]]int fs_save_end(int commit);

// two file panels (t2fc): listing slots, columns, marks, and step-by-step file jobs
[[scinc::dynlink(TERM_DL,"fs_slot_wrapper")]]int fs_slot(int k);
[[scinc::dynlink(TERM_DL,"fs_sort_wrapper")]]void fs_sort(int mode);
[[scinc::dynlink(TERM_DL,"fs_field_wrapper")]]void fs_field(int i, int k, char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_mark_wrapper")]]int fs_mark(int i, int v);
[[scinc::dynlink(TERM_DL,"fs_marked_wrapper")]]int fs_marked(int i);
[[scinc::dynlink(TERM_DL,"fs_mark_match_wrapper")]]int fs_mark_match(int v, char* pattern, int dirsToo);
[[scinc::dynlink(TERM_DL,"fs_mark_info_wrapper")]]int fs_mark_info(char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_disk_wrapper")]]int fs_disk(char* path, char* out, int max);
[[scinc::dynlink(TERM_DL,"fs_mkdir_wrapper")]]int fs_mkdir(char* path);
[[scinc::dynlink(TERM_DL,"fs_job_clear_wrapper")]]void fs_job_clear();
[[scinc::dynlink(TERM_DL,"fs_job_add_wrapper")]]void fs_job_add(char* path);
[[scinc::dynlink(TERM_DL,"fs_job_start_wrapper")]]int fs_job_start(int op, char* dst);
[[scinc::dynlink(TERM_DL,"fs_job_step_wrapper")]]int fs_job_step(int ms);
[[scinc::dynlink(TERM_DL,"fs_job_answer_wrapper")]]void fs_job_answer(int k);
[[scinc::dynlink(TERM_DL,"fs_job_info_wrapper")]]int fs_job_info(int k);
[[scinc::dynlink(TERM_DL,"fs_job_name_wrapper")]]void fs_job_name(int k, char* out, int max);

// running programs: on this terminal (after TvSuspend), or detached from it
[[scinc::dynlink(TERM_DL,"run_here_wrapper")]]int run_here(char* cmd, char* dir);
[[scinc::dynlink(TERM_DL,"run_detached_wrapper")]]int run_detached(char* cmd, char* dir);
[[scinc::dynlink(TERM_DL,"run_has_display_wrapper")]]int run_has_display();
[[scinc::dynlink(TERM_DL,"run_terminal_wrapper")]]int run_terminal(char* out, int max);
[[scinc::dynlink(TERM_DL,"run_quote_wrapper")]]void run_quote(char* s, char* out, int max);

#endif // TERM_H
