// Terminal services from term_dl.so. Build it first with ./build_so.sh
#ifndef TERM_H
#define TERM_H

[[scinc::dynlink("term_dl","term_open_wrapper")]]int term_open();
[[scinc::dynlink("term_dl","term_close_wrapper")]]int term_close();
[[scinc::dynlink("term_dl","term_read_wrapper")]]int term_read(int timeoutMs);
[[scinc::dynlink("term_dl","term_cols_wrapper")]]int term_cols();
[[scinc::dynlink("term_dl","term_rows_wrapper")]]int term_rows();
[[scinc::dynlink("term_dl","term_time_wrapper")]]int term_time(int what);

#endif // TERM_H
