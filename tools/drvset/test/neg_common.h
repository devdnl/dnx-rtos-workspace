/* Common functions of negative test drivers of drvset (tools/drvset/test). MIT License. */
#define API_MOD_COMMON(name)                                                                                           \
        API_MOD_RELEASE(name, void *hdl)                                                                               \
        {                                                                                                              \
                UNUSED_ARG1(hdl);                                                                                      \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_OPEN(name, void *hdl, u32_t flags)                                                                     \
        {                                                                                                              \
                UNUSED_ARG2(hdl, flags);                                                                               \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_CLOSE(name, void *hdl, bool force)                                                                     \
        {                                                                                                              \
                UNUSED_ARG2(hdl, force);                                                                               \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_WRITE(name, void *hdl, const u8_t *src, size_t count, fpos_t *fpos, size_t *wrcnt,                    \
                struct vfs_fattr fattr)                                                                                \
        {                                                                                                              \
                UNUSED_ARG6(hdl, src, count, fpos, wrcnt, fattr);                                                      \
                return ENOTSUP;                                                                                        \
        }                                                                                                              \
        API_MOD_READ(name, void *hdl, u8_t *dst, size_t count, fpos_t *fpos, size_t *rdcnt, struct vfs_fattr fattr)    \
        {                                                                                                              \
                UNUSED_ARG6(hdl, dst, count, fpos, rdcnt, fattr);                                                      \
                return ENOTSUP;                                                                                        \
        }                                                                                                              \
        API_MOD_IOCTL(name, void *hdl, int request, void *arg)                                                         \
        {                                                                                                              \
                UNUSED_ARG3(hdl, request, arg);                                                                        \
                return EBADRQC;                                                                                        \
        }                                                                                                              \
        API_MOD_FLUSH(name, void *hdl)                                                                                 \
        {                                                                                                              \
                UNUSED_ARG1(hdl);                                                                                      \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_STAT(name, void *hdl, struct vfs_dev_stat *stat)                                                       \
        {                                                                                                              \
                UNUSED_ARG2(hdl, stat);                                                                                \
                return ESUCC;                                                                                          \
        }
