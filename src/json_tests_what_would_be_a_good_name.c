#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/tscjson.h"

#define streq(a, b) (strcmp((a), (b)) == 0)
static int test_count = 0;
static int passed_count = 0;

void assert_json_equals(const char* test_name, const char* input, int indent, bool ensure_ascii) {
    test_count++;

    tsc_json_error_t err;
    tsc_value value = tsc_json_decode(input, &err);

    if (err.status != TSC_JSON_ERROR_SUCCESS) {
        printf("[-] %s: decoding failed with error %s: byte %d\n", test_name, tsc_json_error[err.status], err.index);
        tsc_destroy(value);
        return;
    }

    tsc_buffer output = tsc_json_encode(value, &err, indent, ensure_ascii);
    tsc_destroy(value);

    if (err.status != TSC_JSON_ERROR_SUCCESS) {
        printf("[-] %s: encoding failed with error %s\n", test_name, tsc_json_error[err.status]);
        return;
    }

    if (streq(input, output.mem)) {
        printf("[+] %s\n", test_name);
        passed_count++;
    }
    else {
        printf("[-] %s\n", test_name);
        printf("expected: %s\nactual: %s\n", input, output.mem);
    }

    tsc_saving_deleteBuffer(output);
}

void assert_json_equals2(const char* test_name, const char* input, const char* output, int indent, bool ensure_ascii) {
    test_count++;

    tsc_json_error_t err;
    tsc_value value = tsc_json_decode(input, &err);

    if (err.status != TSC_JSON_ERROR_SUCCESS) {
        printf("[-] %s: decoding failed with error %s: byte %d\n", test_name, tsc_json_error[err.status], err.index);
        return;
    }

    tsc_buffer json = tsc_json_encode(value, &err, indent, ensure_ascii);
    tsc_destroy(value);

    if (err.status != TSC_JSON_ERROR_SUCCESS) {
        printf("[-] %s: encoding failed with error %s\n", test_name, tsc_json_error[err.status]);
        return;
    }

    if (streq(json.mem, output)) {
        printf("[+] %s\n", test_name);
        passed_count++;
    }
    else {
        printf("[-] %s\n", test_name);
        printf("expected: %s\nactual: %s\n", output, json.mem);
    }

    tsc_saving_deleteBuffer(json);
}

void test_null() {
    assert_json_equals2("comment", "null // this is indeed null", "null", 0, 0);
    assert_json_equals2("multiline comment", "null /* this is also null */", "null", 0, 0);
}

void test_boolean() {
    assert_json_equals("true", "true", 0, 0);
    assert_json_equals("false", "false", 0, 0);
}

void test_integer() {
    assert_json_equals("zero", "0", 0, 0);
    assert_json_equals("positive integer", "42", 0, 0);
    assert_json_equals("negative integer", "-123", 0, 0);
    assert_json_equals("large positive", "9223372036854775807", 0, 0);
    assert_json_equals("large negative", "-9223372036854775808", 0, 0);
}

void test_number() {
    assert_json_equals("NaN", "NaN", 0, 0);
    assert_json_equals("-Infinity", "-Infinity", 0, 0);
    assert_json_equals("Infinity", "Infinity", 0, 0);

    assert_json_equals("1e-300", "1e-300", 0, 0);
    assert_json_equals("1e-320", "1e-320", 0, 0);
    assert_json_equals("2.2250738585072014e-308", "2.2250738585072014e-308", 0, 0);
    assert_json_equals("1.23456789e-100", "1.23456789e-100", 0, 0);
    assert_json_equals("1e-100", "1e-100", 0, 0);
    assert_json_equals("1e-50", "1e-50", 0, 0);
    assert_json_equals("1e-20", "1e-20", 0, 0);
    assert_json_equals("1e-15", "1e-15", 0, 0);
    assert_json_equals("-3.2e-15", "-3.2e-15", 0, 0);
    assert_json_equals("1e-10", "1e-10", 0, 0);
    assert_json_equals("1e-5", "1e-5", 0, 0);

    assert_json_equals("0.001", "0.001", 0, 0);
    assert_json_equals("0.01", "0.01", 0, 0);
    assert_json_equals("0.1", "0.1", 0, 0);
    assert_json_equals("-0.123456", "-0.123456", 0, 0);
    assert_json_equals("0.14285714285714285", "0.14285714285714285", 0, 0);
    assert_json_equals("0.25", "0.25", 0, 0);
    assert_json_equals("0.3333333333333333", "0.3333333333333333", 0, 0);
    assert_json_equals("0.5", "0.5", 0, 0);
    assert_json_equals("0.6666666666666666", "0.6666666666666666", 0, 0);

    assert_json_equals("0.0", "0.0", 0, 0);

    assert_json_equals("1.5", "1.5", 0, 0);
    assert_json_equals("-2.718", "-2.718", 0, 0);
    assert_json_equals("3.14", "3.14", 0, 0);
    assert_json_equals("3.141592653589793", "3.141592653589793", 0, 0);
    assert_json_equals("1024.0", "1024.0", 0, 0);
    assert_json_equals("10000.0", "10000.0", 0, 0);
    assert_json_equals("999999999999999.0 is this a portal reference", "999999999999999.0", 0, 0);

    assert_json_equals("1e5", "10000.0", 0, 0);
    assert_json_equals("1e6", "100000.0", 0, 0);
    assert_json_equals2("1.048576e6", "1.048576e6", "1048576.0", 0, 0);
    assert_json_equals2("1.25e8", "1.25e8", "125000000.0", 0, 0);
    assert_json_equals2("1.5e10", "1.5e10", "15000000000.0", 0, 0);
    assert_json_equals("1e15", "1e15", 0, 0);
    assert_json_equals("1e16", "1e16", 0, 0);
    assert_json_equals("1e18", "1e18", 0, 0);
    assert_json_equals("-1e20", "-1e20", 0, 0);
    assert_json_equals("1e50", "1e50", 0, 0);
    assert_json_equals("2e50", "2e50", 0, 0);
    assert_json_equals("7e25", "7e25", 0, 0);
    assert_json_equals("1e100", "1e100", 0, 0);
    assert_json_equals("1e300", "1e300", 0, 0);
}

void test_string() {
    assert_json_equals("empty string", "\"\"", 0, 0);
    assert_json_equals("string", "\"hello\"", 0, 0);
    assert_json_equals("string with spaces", "\"despite popular beleaf it is not popular to be a leaf\"", 0, 0);
    assert_json_equals("string with quotes", "\"say \\\"hello\\\"\"", 0, 0);
    assert_json_equals("string with backslash", "\"c\\\\windowse\\\\system234\"", 0, 0);
    assert_json_equals("string with newline", "\"line1\\nline2\"", 0, 0);
    assert_json_equals("string with tab", "\"col1\\tcol2\"", 0, 0);
    assert_json_equals("string with carriage return", "\"text\\rmore\"", 0, 0);
    assert_json_equals("string with bell", "\"ring\\a\"", 0, 0);
    assert_json_equals("string with backspace", "\"oops\\b\"", 0, 0);
}

void test_unicode() {
    assert_json_equals("unicode emoji", "\"👋\"", 0, 0);
    assert_json_equals("unicode accents", "\"café\"", 0, 0);
    assert_json_equals("unicode symbols", "\"€50\"", 0, 0);
    assert_json_equals("unicode", "\"дсдсдс и смерть 💀✨ кампютер ели ели 🌲 а чайник вовси в шкиле 🦈 и смерть в раю и солнце ☀️ бесплатно без регистрации смс и регистрации на сайте Роскомнадзор и не только о 😼\"", 0, 0); assert_json_equals("unicode", "\"\\u2600\\uFE0F\\U0001F63C\"", 0, 1); }

void test_array() {
    assert_json_equals("empty array", "[]", 0, 0);
    assert_json_equals("array", "[1, 2, 3]", 0, 0);
    assert_json_equals("mixed type array", "[\"hello\", 42, true, null]", 0, 0);
    assert_json_equals("nested array", "[\"nested\", [1, 2]]", 0, 0);
}

void test_object() {
    assert_json_equals("empty object", "{}", 0, 0);
    assert_json_equals("simple object", "{\"name\": \"jeremy\", \"age\": 30}", 0, 0);
    assert_json_equals("complex object", "{\"string\": \"value\", \"number\": 3.14, \"boolean\": false, \"null\": null}", 0, 0);
    assert_json_equals("nested object", "{\"position\": {\"x\": 10, \"y\": 20}, \"name\": \"point\"}", 0, 0);
}

void test_pretty_printing() {
    assert_json_equals("pretty printed object",
        "{\n"
        "  \"name\": \"jeremy\",\n"
        "  \"age\": 30,\n"
        "  \"i dont know\": [\n"
        "    \"this is jeremy\"\n"
        "  ]\n"
        "}", 2, 0);
}

void test_edge_cases() {
    assert_json_equals("object with special keys", "{\"key with spaces\": \"value1\", \"key\\\"with\\\"quotes\": \"value2\"}", 0, 0);
    assert_json_equals("deep nesting", "{\"level1\": {\"level2\": {\"level3\": {\"value\": \"deep\"}}}}", 0, 0);
    assert_json_equals("array of objects", "[{\"id\": 1, \"name\": \"lobster\"}, {\"id\": 2, \"name\": \"bobster\"}]", 0, 0);
}

void test_binary_and_hex() {
    assert_json_equals2("binary zero", "0b0", "0", 0, 0);
    assert_json_equals2("binary max int64", "0b111111111111111111111111111111111111111111111111111111111111111", "9223372036854775807", 0, 0);

    assert_json_equals2("hex zero", "0x0", "0", 0, 0);
    assert_json_equals2("hex 1", "0xaAcCeE", "11193582", 0, 0);
    assert_json_equals2("hex 2", "0xDEADC0DE", "3735929054", 0, 0);
    assert_json_equals2("hex max int64", "0x7FFFFFFFFFFFFFFF", "9223372036854775807", 0, 0);

    assert_json_equals2("negative binary", "-0b101", "-5", 0, 0);
    assert_json_equals2("negative hex", "-0xFF", "-255", 0, 0);
}

int main() {
    test_null();
    test_boolean();
    test_integer();
    test_number();
    test_string();
    test_unicode();
    test_array();
    test_object();
    test_pretty_printing();
    test_edge_cases();
    test_binary_and_hex();

    printf("tests ran: %d\n", test_count);
    printf("passed: %d\n", passed_count);
    printf("failed: %d\n", test_count - passed_count);
    return test_count == passed_count ? EXIT_SUCCESS : EXIT_FAILURE;
}