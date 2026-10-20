/* Exercise the real CLI and model registry without USB hardware. */
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../td-usb.h"

static jmp_buf exited;
static FILE *test_input, *test_output, *test_error;
static int exit_code, immediate_exit, opens, closes, calls, timer_calls;
static int fail_allocation, fail_timer, interrupt_on_close;
static int last_format, last_skip, last_count, last_interval;
static char last_arg[256], output[2048];
static int scenarios;

static void test_exit(int code) { exit_code = code; longjmp(exited, 1); }
static void test_immediate_exit(int code) { immediate_exit = 1; test_exit(code); }
static void *test_malloc(size_t size) { return fail_allocation ? NULL : malloc(size); }

static int operation(void *parameter)
{
    td_context_t *ctx = parameter;
    calls++;
    last_format = ctx->format;
    last_skip = ctx->skip;
    last_count = ctx->c;
    if (ctx->c) {
        assert(strlen(ctx->v[0]) < sizeof(last_arg));
        strcpy(last_arg, ctx->v[0]);
    }
    return 0;
}

static td_device_t *new_device(const char *name)
{
    td_device_t *device = calloc(1, sizeof(*device));
    assert(device);
    device->product_name = malloc(strlen(name) + 1);
    assert(device->product_name);
    strcpy(device->product_name, name);
    device->get = device->set = operation;
    return device;
}

#define MODELS(X) \
    X(tdfa50507) X(tdfa30604) X(tdfa30608) X(iwt120) X(iws660) X(iws73x) \
    X(tdpc0201) X(iwt303) X(iwt313) X(iwt1320) X(tdsn450) X(tdfa60220) \
    X(tdfa60250) X(tdfa6032) X(tdpc0205) X(tdpc2401) X(tdpw6620) X(tdsn0700) \
    X(tdfa60350) X(tdsn5200) X(tdsn7200) X(tdsn604xx) X(tdsn7400) X(tdsn7360) \
    X(tdsn7502) X(tdfa1104) X(tdfa50607) X(tdsn7300) X(iws600cm) X(tdfa30301) \
    X(tdfa30303) X(tdfa60228)
#define IMPORT(name) \
    static td_device_t *import_##name(void) { return new_device(#name); } \
    td_device_t *(*name##_import)(void) = import_##name;
MODELS(IMPORT)
#undef IMPORT
#include "../device_types.c"

#define main tdusb_main
#define malloc test_malloc
#define exit test_exit
#define _Exit test_immediate_exit
#undef stdin
#undef stdout
#undef stderr
#define stdin test_input
#define stdout test_output
#define stderr test_error
#define printf(...) fprintf(test_output, __VA_ARGS__)
#define vprintf(format, args) vfprintf(test_output, format, args)
#include "../td-usb.c"
#undef vprintf
#undef printf
#undef stderr
#undef stdout
#undef stdin
#undef _Exit
#undef exit
#undef malloc
#undef main

int *TdHidOpenDevice(uint16_t vendor, uint16_t product, const char *name, char *serial)
{
    static int handle;
    (void)vendor; (void)product; (void)name;
    if (serial) assert(!strcmp(serial, "12345"));
    opens++;
    return &handle;
}
void TdHidCloseDevice(int *handle)
{
    assert(handle);
    closes++;
    assert(closes == 1);
    if (interrupt_on_close) raise(SIGINT);
}
int TdHidListDevices(uint16_t vendor, uint16_t product, const char *name, char *buffer, int size)
{
    (void)vendor; (void)product; (void)name; (void)buffer; (void)size;
    assert(!"Unexpected device listing");
    return -1;
}
int TdTimer_Start(void callback(void *), void *parameter, int interval)
{
    timer_calls++;
    last_interval = interval;
    if (fail_timer) return -1;
    callback(parameter);
    return 0;
}

static void run(int argc, char **argv, const char *input, int expected)
{
    assert(context == NULL);
    opens = closes = calls = timer_calls = immediate_exit = 0;
    last_format = last_skip = last_count = last_interval = -1;
    last_arg[0] = 0;
    test_input = tmpfile(); test_output = tmpfile(); test_error = tmpfile();
    assert(test_input && test_output && test_error);
    assert(fputs(input, test_input) >= 0);
    rewind(test_input);
    if (setjmp(exited) == 0) {
        tdusb_main(argc, argv);
        assert(!"CLI must terminate");
    }
    assert(exit_code == expected);
    if (expected == EXITCODE_UNKNOWN_DEVICE || expected == EXITCODE_INVALID_OPTION ||
        expected == EXITCODE_INVALID_FORMAT) assert(calls == 0);
    rewind(test_output);
    size_t length = fread(output, 1, sizeof(output) - 1, test_output);
    output[length] = 0;
    if (context) {
        /* Immediate exit deliberately leaves application cleanup to the OS. */
        assert(immediate_exit);
        delete_device_type(context->device_type);
        free(context);
        context = NULL;
    }
    signal(SIGINT, SIG_DFL);
    fclose(test_input); fclose(test_output); fclose(test_error);
    scenarios++;
}
#define RUN(expected, input, ...) do { \
    char *args[] = { "td-usb", __VA_ARGS__ }; \
    run((int)(sizeof(args) / sizeof(args[0])), args, input, expected); \
} while (0)

int main(void)
{
    /* Verify every registry entry, including its serial-qualified spelling. */
#define NAME(name) #name,
    const char *models[] = { MODELS(NAME) };
#undef NAME
    for (size_t i = 0; i < sizeof(models) / sizeof(models[0]); ++i) {
        char qualified[64];
        td_device_t *device = import_device_type(models[i], (int)strlen(models[i]));
        assert(device && !strcmp(device->product_name, models[i]));
        delete_device_type(device);
        sprintf(qualified, "%s:12345", models[i]);
        device = import_device_type(qualified, (int)strlen(models[i]));
        assert(device && !strcmp(device->product_name, models[i]));
        delete_device_type(device);
    }
    RUN(0, "", "tdfa50507:12345", "get");
    assert(opens == 1 && closes == 1 && calls == 1);
    const char *bad_models[] = { "", ":12345", "td", "tdfa306", "tdfa50507extra", "unknown" };
    for (size_t i = 0; i < sizeof(bad_models) / sizeof(bad_models[0]); ++i) {
        RUN(2, "", (char *)bad_models[i], "get");
        assert(opens == 0);
    }

    RUN(0, "", "tdfa50507", "set", "--verbose", "VALUE=%s%n%%");
    assert(!strcmp(output, "OPTION:VALUE=%s%n%%\n"));
    assert(!strcmp(last_arg, "VALUE=%s%n%%"));

    const char *bad_options[] = {
        "--lopp=1000", "--looping=50", "--formats=json", "--skipx=1",
        "--verbose=1", "--verbose-extra", "--format", "--skip",
        "--loop=", "--loop=49", "--loop=-1", "--loop=50oops", "--loop=2147483648",
        "--loop=999999999999999999999999999999", "--skip=", "--skip=0",
        "--skip=-1", "--skip=1oops", "--skip=2147483648"
    };
    for (size_t i = 0; i < sizeof(bad_options) / sizeof(bad_options[0]); ++i) {
        RUN(6, "", "tdfa50507", "get", (char *)bad_options[i]);
        assert(opens == 0);
    }
    RUN(13, "", "tdfa50507", "get", "--format=bogus");
    assert(opens == 0);
    RUN(13, "", "tdfa50507", "get", "--format=");
    assert(opens == 0);
    const char *formats[] = { "simple", "raw", "json", "csv", "tsv" };
    for (int i = 0; i < 5; ++i) {
        char option[32];
        sprintf(option, "--format=%s", formats[i]);
        RUN(0, "", "tdfa50507", "get", option);
        assert(last_format == i);
    }
    RUN(0, "", "tdfa50507", "get", "--format=json", "--format=simple");
    assert(last_format == FORMAT_SIMPLE);
    RUN(0, "", "tdfa50507", "get", "--loop=50", "--skip=1");
    assert(timer_calls == 1 && calls == 2 && last_interval == 50 && last_skip == 1);
    RUN(0, "", "tdfa50507", "get", "--loop");
    assert(last_interval == OPTION_DEFAULT_INTERVAL);
    RUN(0, "", "tdfa50507", "get", "--loop=2147483647", "--skip=2147483647");
    assert(last_interval == INT_MAX && last_skip == INT_MAX);

    RUN(0, "A=1 B=2\nC=3\n", "tdfa50507", "set", "--loop");
    assert(calls == 2 && last_count == 1 && !strcmp(last_arg, "C=3"));
    RUN(0, "A=1", "tdfa50507", "set", "--loop");
    assert(calls == 1);
    char long_line[300];
    memset(long_line, ' ', 250);
    strcpy(long_line + 250, "A=12345\n");
    RUN(6, long_line, "tdfa50507", "set", "--loop");
    for (int length = 254; length <= 256; ++length) {
        memset(long_line, ' ', (size_t)length);
        memcpy(long_line, "A=1", 3);
        long_line[length] = 0;
        RUN(length <= 255 ? 0 : 6, long_line, "tdfa50507", "set", "--loop");
        if (length <= 255) assert(calls == 1);
        strcpy(long_line + length, "\nB=2\n");
        RUN(length <= 255 ? 0 : 6, long_line, "tdfa50507", "set", "--loop");
        if (length <= 255) assert(calls == 2 && !strcmp(last_arg, "B=2"));
    }
    char many[128] = "";
    for (int i = 0; i < 16; ++i) strcat(many, "A=1 ");
    RUN(0, many, "tdfa50507", "set", "--loop");
    assert(last_count == 16);
    strcat(many, "A=1\n");
    RUN(6, many, "tdfa50507", "set", "--loop");
    RUN(6, "", "tdfa50507", "set", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1",
        "A=1", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1", "A=1");
    assert(opens == 0);

    fail_allocation = 1;
    RUN(11, "", "tdfa50507", "get");
    assert(opens == 0);
    fail_allocation = 0;
    fail_timer = 1;
    RUN(12, "", "tdfa50507", "get", "--loop");
    assert(calls == 1 && timer_calls == 1 && closes == 1);
    fail_timer = 0;
    interrupt_on_close = 1;
    RUN(0, "", "tdfa50507", "get");
    assert(immediate_exit && closes == 1);
    printf("PASS: %d common CLI scenarios and all 32 model registrations\n", scenarios);
    return 0;
}
