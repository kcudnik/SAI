
typedef struct  _sai_meta_ver_enum_t
{
    int32_t value;

    const char * name;

} sai_meta_ver_enum_t;

typedef struct _sai_meta_version_t
{
    int32_t saiversion;

    const sai_meta_ver_enum_t* enums;

} sai_meta_version_t;
