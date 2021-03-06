/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2015-2020 Micron Technology, Inc.  All rights reserved.
 */

#include <3rdparty/rbtree.h>

#include <hse_ut/framework.h>

#include <hse_util/inttypes.h>
#include <hse_util/hse_err.h>
#include <hse_util/logging.h>
#include <hse_util/parse_num.h>
#include <hse_util/data_tree.h>
#include <hse_util/perfc.h>

#define BUF_SIZE 4096
char buf[BUF_SIZE];

int
platform_pre(struct mtf_test_info *ti)
{
    return 0;
}

MTF_MODULE_UNDER_TEST(hse_platform);

MTF_BEGIN_UTEST_COLLECTION_PRE(perfc, platform_pre);

#undef COMPNAME
#define COMPNAME __func__

MTF_DEFINE_UTEST(perfc, perfc_basic_create_find_and_remove)
{
    size_t              count;
    struct yaml_context yc = {
        .yaml_indent = 0, .yaml_offset = 0,
    };
    union dt_iterate_parameters dip = {.yc = &yc };
    struct dt_element *         dte;
    int                         rc, n;
    char                        path[128];
    struct perfc_name           ctrnames = { 0 };
    struct perfc_set            set = { 0 };
    merr_t                      err;

    ctrnames.pcn_name = "PERFC_BA_FAM_TEST";
    ctrnames.pcn_hdr = "whysoserious";
    ctrnames.pcn_desc = "joker";
    ctrnames.pcn_flags = 0;
    ctrnames.pcn_prio = 1;

    err = perfc_ctrseti_alloc(COMPNAME, "batman_villains", &ctrnames, 1, "joker", &set);
    ASSERT_EQ(0, err);

    count = dt_iterate_cmd(dt_data_tree, DT_OP_COUNT, PERFC_ROOT_PATH, NULL, NULL, NULL, NULL);
    ASSERT_EQ(1, count);

    yc.yaml_buf = buf;
    yc.yaml_buf_sz = BUF_SIZE;
    yc.yaml_emit = NULL;

    count = dt_iterate_cmd(dt_data_tree, DT_OP_EMIT, PERFC_ROOT_PATH, &dip, NULL, NULL, NULL);

    /* 3, for /data, /data/perfc, /data/perfc/joker */
    ASSERT_EQ(count, 3);

    printf("=====================================\n");
    printf("%s", buf);
    printf("=====================================\n");

    n = snprintf(
        path,
        sizeof(path),
        "%s/%s/%s/%s/%s",
        PERFC_ROOT_PATH,
        COMPNAME,
        "batman_villains",
        "FAM",
        "joker");
    ASSERT_TRUE(n > 0 && n < sizeof(path));

    dte = dt_find(dt_data_tree, path, 1);
    ASSERT_NE(dte, NULL);

    rc = dt_remove(dt_data_tree, dte);
    ASSERT_EQ(rc, 0);

    dte = dt_find(dt_data_tree, path, 1);
    ASSERT_EQ(dte, NULL);
}

MTF_DEFINE_UTEST(perfc, perfc_basic_set)
{
    size_t              count;
    struct yaml_context yc = {
        .yaml_indent = 0, .yaml_offset = 0,
    };
    union dt_iterate_parameters dip;
    struct dt_element *         dte;
    int                         rc, n;
    char                        path[128];
    u64                         new_value = 42;

    struct perfc_name ctrnames = { 0 };
    struct perfc_set  set = { 0 };
    merr_t            err;

    ctrnames.pcn_name = "PERFC_BA_FAM_TEST";
    ctrnames.pcn_hdr = "whysoserious";
    ctrnames.pcn_desc = "joker";
    ctrnames.pcn_flags = 0;
    ctrnames.pcn_prio = 1;

    err = perfc_ctrseti_alloc(COMPNAME, "batman_villains", &ctrnames, 1, "poison_ivy", &set);
    ASSERT_EQ(0, err);

    perfc_set(&set, 0, new_value);
    n = snprintf(
        path,
        sizeof(path),
        "%s/%s/%s/%s/%s",
        PERFC_ROOT_PATH,
        COMPNAME,
        "batman_villains",
        "FAM",
        "poison_ivy");
    ASSERT_TRUE(n > 0 && n < sizeof(path));

    dip.yc = &yc;
    yc.yaml_buf = buf;
    yc.yaml_buf_sz = BUF_SIZE;
    yc.yaml_emit = NULL;
    count = dt_iterate_cmd(dt_data_tree, DT_OP_EMIT, PERFC_ROOT_PATH, &dip, NULL, NULL, NULL);

    /* 3, for /data, /data/perfc, /data/perfc/poison_ivy */
    ASSERT_EQ(count, 3);

    printf("=====================================\n");
    printf("%s", buf);
    printf("=====================================\n");
    ASSERT_NE(NULL, strstr(buf, "value: 42"));

    dte = dt_find(dt_data_tree, path, 1);
    ASSERT_NE(dte, NULL);

    rc = dt_remove(dt_data_tree, dte);
    ASSERT_EQ(rc, 0);

    dte = dt_find(dt_data_tree, path, 1);
    ASSERT_EQ(dte, NULL);
}

static inline void
perfc_test_ctrs(struct perfc_set *set)
{
    perfc_set(set, 0, 0);
    perfc_add(set, 0, 10);
    perfc_inc(set, 0);
    perfc_dec(set, 0);
    perfc_sub(set, 0, 2);
    perfc_sub(set, 0, 10);
    perfc_sub(set, 0, 8);
    perfc_dec(set, 0);
}

MTF_DEFINE_UTEST(perfc, clear_counters)
{
    struct perfc_name *         ctrnames;
    struct perfc_set            set = { 0 };
    merr_t                      err;
    struct dt_set_parameters    dsp;
    union dt_iterate_parameters dip;
    char *                      name;
    int                         i, ctrc = 4;
    int                         count;

    ctrnames = calloc(ctrc, sizeof(*ctrnames) + 32);
    ASSERT_NE(0, ctrnames);
    name = (void *)(ctrnames + ctrc);

    for (i = 0; i < ctrc; i++) {
        snprintf(name, 32, "mycounter-%d", i);
        ctrnames[i].pcn_desc = name;
        ctrnames[i].pcn_flags = 0;
        ctrnames[i].pcn_prio = 1;
        ctrnames[i].pcn_hdr = "mycounterhdr";
        name += 32;
    }

    ctrnames[0].pcn_name = "PERFC_BA_FAM_TEST";
    ctrnames[1].pcn_name = "PERFC_RA_FAM_TEST";
    ctrnames[2].pcn_name = "PERFC_LT_FAM_TEST";
    ctrnames[3].pcn_name = "PERFC_SL_FAM_TEST";

    err = perfc_ctrseti_alloc("mycomp", "myset", ctrnames, ctrc, "alltypes", &set);
    ASSERT_EQ(0, err);

    perfc_test_ctrs(&set);

    perfc_ctrseti_path(&set);

    dsp.path = "/data/perfc/mycomp/myset/FAM/alltypes";
    dsp.value = "1";
    dsp.value_len = strlen(dsp.value);
    dsp.field = DT_FIELD_CLEAR;
    dip.dsp = &dsp;

    count = dt_iterate_cmd(dt_data_tree, DT_OP_SET, dsp.path, &dip, NULL, NULL, NULL);
    ASSERT_EQ(1, count);

    perfc_ctrseti_free(&set);
    free(ctrnames);
}

MTF_DEFINE_UTEST(perfc, enable_counters)
{
    struct perfc_name           ctrnames = { 0 };
    struct perfc_set            set = { 0 };
    merr_t                      err;
    struct dt_set_parameters    dsp;
    union dt_iterate_parameters dip;
    int                         count;
    char *                      path;

    perfc_verbosity = 1;
    ctrnames.pcn_desc = "mycounter";
    ctrnames.pcn_hdr = "mycounterhdr";
    ctrnames.pcn_flags = 0;
    ctrnames.pcn_prio = 3;
    ctrnames.pcn_name = "PERFC_BA_FAM_TEST";

    err = perfc_ctrseti_alloc("mycomp", "myset", &ctrnames, 1, "basic", &set);
    ASSERT_EQ(0, err);

    perfc_test_ctrs(&set);

    path = perfc_ctrseti_path(&set);
    ASSERT_EQ(0, strcmp(path, "/data/perfc/mycomp/myset/FAM/basic"));

    dsp.path = path;
    dsp.value = "1";
    dsp.value_len = strlen(dsp.value);
    dsp.field = DT_FIELD_ENABLED;
    dip.dsp = &dsp;

    count = dt_iterate_cmd(dt_data_tree, DT_OP_SET, dsp.path, &dip, NULL, NULL, NULL);
    ASSERT_EQ(1, count);

    dsp.value = "0";
    count = dt_iterate_cmd(dt_data_tree, DT_OP_SET, dsp.path, &dip, NULL, NULL, NULL);
    ASSERT_EQ(1, count);

    perfc_ctrseti_free(&set);
}

MTF_DEFINE_UTEST(perfc, perfc_verbosity_set_test)
{
    struct perfc_name           ctrnames = { 0 };
    struct perfc_set            set = { 0 };
    merr_t                      err;
    struct dt_set_parameters    dsp;
    union dt_iterate_parameters dip;
    int                         count;

    struct yaml_context yc = {
        .yaml_indent = 0, .yaml_offset = 0,
    };

    perfc_shutdown();
    dt_shutdown();
    dt_init();
    perfc_init();

    ctrnames.pcn_desc = "mycounter";
    ctrnames.pcn_hdr = "mycounterhdr";
    ctrnames.pcn_flags = 0;
    ctrnames.pcn_prio = 3;
    ctrnames.pcn_name = "PERFC_BA_FAM_TEST";

    err = perfc_ctrseti_alloc("mycomp", "myset", &ctrnames, 1, "basic", &set);
    ASSERT_EQ(0, err);
    perfc_add(&set, 0, 3);

    dsp.path = "/data/config/platform/perfc/perfc_verbosity";
    dsp.value = "3";
    dsp.value_len = strlen(dsp.value);
    dsp.field = DT_FIELD_ENABLED;
    dip.dsp = &dsp;

    count = dt_iterate_cmd(dt_data_tree, DT_OP_SET, dsp.path, &dip, NULL, NULL, NULL);
    ASSERT_EQ(1, count);

    perfc_add(&set, 0, 7);

    /* Emit */
    dip.yc = &yc;
    yc.yaml_buf = buf;
    yc.yaml_buf_sz = BUF_SIZE;
    yc.yaml_emit = NULL;
    count = dt_iterate_cmd(dt_data_tree, DT_OP_EMIT, PERFC_ROOT_PATH, &dip, NULL, NULL, NULL);
    printf("=====================================\n");
    printf("%s", buf);
    printf("=====================================\n");
    ASSERT_NE(NULL, strstr(buf, "value: 7"));
}

MTF_DEFINE_UTEST(perfc, ctrset_path)
{
    struct perfc_name ctrnames = { 0 };
    struct perfc_set  set;
    merr_t            err;

    ctrnames.pcn_name = "PERFC_BA_FAM_TEST";
    ctrnames.pcn_hdr = "mycounterhdr";
    ctrnames.pcn_desc = "mycounter";
    ctrnames.pcn_flags = 0;
    ctrnames.pcn_prio = 1;

    err = perfc_ctrseti_alloc("c", "n", &ctrnames, 1, "s", &set);
    ASSERT_EQ(0, err);
    ASSERT_EQ(0, strcmp(PERFC_ROOT_PATH "/c/n/FAM/s", perfc_ctrseti_path(&set)));

    perfc_ctrseti_free(&set);
}

static u64
perfc_value(struct perfc_set *pcs, u32 cidx)
{
    struct perfc_ctr_hdr *hdr;
    struct perfc_seti *   pcsi;
    u64                   vadd;
    u64                   vsub;
    int                   i;

    pcsi = perfc_ison(pcs, cidx);
    hdr = &pcsi->pcs_ctrv[cidx].hdr;
    assert(hdr->pch_type == PERFC_TYPE_BA);

    vadd = vsub = 0;

    for (i = 0; i < PERFC_PCV_MAX; ++i) {
        struct perfc_val *v = &hdr->pch_val[i];

        vadd += atomic64_read(&v->pcv_vadd);
        vsub += atomic64_read(&v->pcv_vsub);
    }

    return vadd - vsub;
}

MTF_DEFINE_UTEST(perfc, perfc_rollup)
{
    enum perfc_rollup_sidx {
        PERFC_BA_RUTEST_INC,
        PERFC_BA_RUTEST_INC2,
        PERFC_BA_RUTEST_ADD2,
        PERFC_EN_RUTEST
    };
    struct perfc_name perfc_rollup_op[] = {
        NE(PERFC_BA_RUTEST_INC, 0, "rutest_inc", "rutest_inc"),
        NE(PERFC_BA_RUTEST_INC2, 0, "rutest_inc2", "rutest_inc2"),
        NE(PERFC_BA_RUTEST_ADD2, 0, "rutest_add2", "rutest_add2"),
    };

    struct perfc_set perfc_rollup_pc;
    merr_t           err;
    u64              sum;
    u64              val;
    int              i;

    err = perfc_ctrseti_alloc(
        COMPNAME, "rollup", perfc_rollup_op, PERFC_EN_RUTEST, "set", &perfc_rollup_pc);
    ASSERT_EQ(err, 0);

    for (i = 0, sum = 0; i < 1024 * 1024; ++i, sum += i) {
        PERFC_INC_RU(&perfc_rollup_pc, PERFC_BA_RUTEST_INC, 17);

        PERFC_INCADD_RU(&perfc_rollup_pc, PERFC_BA_RUTEST_INC2, PERFC_BA_RUTEST_ADD2, i, 1024);
    }

    val = perfc_value(&perfc_rollup_pc, PERFC_BA_RUTEST_INC);
    ASSERT_GE(val, i - 17);

    val = perfc_value(&perfc_rollup_pc, PERFC_BA_RUTEST_INC2);
    ASSERT_GE(val, i - 1024);

    val = perfc_value(&perfc_rollup_pc, PERFC_BA_RUTEST_ADD2);
    ASSERT_GE(val, sum - i);

    perfc_ctrseti_free(&perfc_rollup_pc);
}

MTF_END_UTEST_COLLECTION(perfc)
