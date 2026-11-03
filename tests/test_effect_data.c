/* The effect data loader (src/effects/effect_data.c, included so that each case starts
   unloaded): the embedded data loads; RAGEXE_EFFECT_DATA, unset or empty, is ignored; a
   good override file replaces the embedded data; a missing or bad file is reported once
   through error_msg and the embedded data is used instead. */
#include "../src/effects/effect_data.c"
#include <assert.h>

static int g_errors, g_rejected;
static char g_last_error[1024];

void error_msg(const char *text, int flags)
{
    assert(flags == 0);
    ++g_errors;
    strncpy(g_last_error, text, sizeof g_last_error - 1);
}

static const char *g_dir;
static char g_path[1024];

static void reset(void)
{
    free_tables(&g_tables);
    g_state = 0;
    g_errors = 0;
    g_last_error[0] = 0;
}

static void set_override(const char *value)
{
    static char env[1200];
    snprintf(env, sizeof env, "RAGEXE_EFFECT_DATA=%s", value ? value : "");
    assert(_putenv(env) == 0);
}

/* Writes text as the override file and loads. */
static void load_text(const char *text)
{
    FILE *f;
    snprintf(g_path, sizeof g_path, "%s\\override.json", g_dir);
    f = fopen(g_path, "wb");
    assert(f && fwrite(text, 1, strlen(text), f) == strlen(text) && fclose(f) == 0);
    reset();
    set_override(g_path);
    assert(effect_data_load() == 1);
}

static int embedded(void)
{
    const struct RagStrBinding *b = effect_data_rag_str(30);
    return b && b->plays && b->cue_count == 1 && !strcmp(b->cues[0].ops[0].wave, "effect\\magician_thunderstorm.wav");
}

static void expect_rejected(const char *text, const char *message)
{
    load_text(text);
    if (g_errors != 1 || !strstr(g_last_error, message) || !embedded()) {
        fprintf(stderr, "case %s\n  got %d errors: %s\n", text, g_errors, g_last_error);
        assert(!"bad file not rejected as expected");
    }
    ++g_rejected;
}

#define HEAD "{\"Format\": \"ragexe effect data 1\", "
#define EZ "\"Ez2StrFile\": {}}"

int main(int argc, char **argv)
{
    const struct RagStrBinding *b;
    const struct Ez2StrFile *f;
    int cases = 0;

    assert(argc == 2);
    g_dir = argv[1];

    /* Embedded, with the variable unset and empty. */
    reset();
    set_override(NULL);
    assert(effect_data_load() == 1 && g_errors == 0 && embedded());
    b = effect_data_rag_str(1021);
    assert(b && !strcmp(b->file, "dust.str") && b->loops == 10 && b->plays && b->on_end == RAG_STR_END_STOP);
    b = effect_data_rag_str(10);
    assert(b && !b->plays && !strcmp(b->min_file, "memor_min.str"));
    b = effect_data_rag_str(204);
    assert(b && !strcmp(b->file, "\xbb\xa1\xb0\xa3\xc6\xf7\xbc\xc7.str"));     /* 빨간포션 in CP949 */
    f = effect_data_ez2str(141);
    assert(f && f->variants == 3 && f->open == EZ2STR_LOAD && !strcmp(f->file, "Pneuma%d.str"));
    f = effect_data_ez2str(1741);
    assert(f && f->open == EZ2STR_OPEN);
    assert(!effect_data_rag_str(-1) && !effect_data_rag_str(70000) && !effect_data_ez2str(0));
    cases += 2;

    /* A good override replaces the embedded data. */
    load_text(HEAD "\"RagEffect\": {\"30\": {\"Str\": {\"File\": \"x%d.str\", \"Variants\": 12}, \"Play\": "
              "{\"Timeline\": [{\"At\": 4, \"AfterStr\": true, \"Do\": [{\"Follow\": true}, {\"Wave\": \"w.wav\", "
              "\"Height\": -2.5}, {\"Wave\": \"p.wav\", \"AtPlayer\": true}, {\"Quake\": 1}, {\"Duration\": 7}]}], "
              "\"Offset\": [1, 2.25, -3], \"OnStrEnd\": \"Stop\", \"StopFrame\": 9}}}, "
              "\"Ez2StrFile\": {\"7\": {\"File\": \"\xed\x9d\xa1\xea\xb8\xb0.str\", \"FullOnly\": true}}}");
    assert(g_errors == 0);
    b = effect_data_rag_str(30);
    assert(b && b->variants == 12 && !strcmp(b->file, "x%d.str") && b->plays && b->cue_count == 1);
    assert(b->cues[0].at == 4 && b->cues[0].after_str == 1 && b->cues[0].op_count == 5);
    assert(b->cues[0].ops[0].kind == RAG_STR_FOLLOW && b->cues[0].ops[1].kind == RAG_STR_WAVE);
    assert(b->cues[0].ops[1].height == -2.5f && b->cues[0].ops[2].kind == RAG_STR_PLAYER_WAVE);
    assert(b->cues[0].ops[3].kind == RAG_STR_QUAKE && b->cues[0].ops[3].value == 1);
    assert(b->cues[0].ops[4].kind == RAG_STR_DURATION && b->cues[0].ops[4].value == 7);
    assert(b->offset[0] == 1.0f && b->offset[1] == 2.25f && b->offset[2] == -3.0f && b->stop_frame == 9);
    assert(!effect_data_rag_str(1021) && !effect_data_ez2str(141));
    f = effect_data_ez2str(7);
    assert(f && f->open == EZ2STR_FULL_ONLY && !strcmp(f->file, "\xc8\xed\xb1\xe2.str"));   /* 흡기 */
    ++cases;

    /* A missing file and bad files fall back to the embedded data, with one report. */
    reset();
    snprintf(g_path, sizeof g_path, "%s\\no such file.json", g_dir);
    set_override(g_path);
    assert(effect_data_load() == 1 && g_errors == 1 && strstr(g_last_error, "cannot open") && embedded());
    assert(strstr(g_last_error, "built-in effect data instead"));
    ++cases;
    expect_rejected("", "not JSON");
    expect_rejected("{\"Format\": \"ragexe effect data 1\",}", "not JSON");
    expect_rejected("\xef\xbb\xbf" HEAD "\"RagEffect\": {}, " EZ, "not JSON");
    expect_rejected("{\"Format\": \"ragexe effect data 2\", \"RagEffect\": {}, " EZ, "Format must be");
    expect_rejected(HEAD "\"RagEffect\": {}, \"Extra\": 1, " EZ, "unknown key \"Extra\"");
    expect_rejected(HEAD "\"RagEffect\": {}}", "Ez2StrFile must be an object");
    expect_rejected(HEAD "\"RagEffect\": {\"030\": {\"Play\": {}}}, " EZ, "\"030\" is not an ID");
    expect_rejected(HEAD "\"RagEffect\": {\"-3\": {\"Play\": {}}}, " EZ, "\"-3\" is not an ID");
    expect_rejected(HEAD "\"RagEffect\": {\"65536\": {\"Play\": {}}}, " EZ, "is over 65535");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {}}, \"5\": {\"Play\": {}}}, " EZ, "RagEffect 5: given twice");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {}}, " EZ, "needs Str or Play");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {}, \"Play\": {}}}, " EZ, "given twice");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"MinFile\": \"a.str\"}}}, " EZ, "File is missing");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"\"}}}, " EZ, "must be a file name");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": 5}}}, " EZ, "File must be a string");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%d.str\"}}}, " EZ, "must not hold %");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a.str\", \"Variants\": 3}}}, " EZ,
                    "one %d for the variant");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%d%d.str\", \"Variants\": 3}}}, " EZ,
                    "one %d for the variant");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%s.str\", \"Variants\": 3}}}, " EZ,
                    "one %d for the variant");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%d.str\", \"Variants\": 1}}}, " EZ,
                    "Variants must be from 2 to 99");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%d.str\", \"Variants\": 2.5}}}, " EZ,
                    "Variants must be an integer");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a%d.str\", \"Variants\": 2, "
                    "\"MinFile\": \"b.str\"}}}, " EZ, "has no MinFile");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": "
                    "\"0123456789012345678901234567890123456789012345678901234567890123.str\"}}}, " EZ,
                    "longer than 63 bytes");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"\xf0\x9f\x98\x80.str\"}}}, " EZ,
                    "CP949 does not hold");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Str\": {\"File\": \"a.str\", \"Loops\": -1}}}, " EZ,
                    "Loops must be from 0");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Offset\": [0, 0.1, 0]}}}, " EZ,
                    "a float holds exactly");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Offset\": [0, 0]}}}, " EZ, "three numbers");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"OnStrEnd\": \"Loop\"}}}, " EZ, "OnStrEnd must be");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"OnStrEnd\": \"Continue\", \"StopFrame\": 3}}}, " EZ,
                    "StopFrame belongs to");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"At\": 1}]}}}, " EZ,
                    "Do must be a list");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"At\": -1, \"Do\": [{\"Quake\": 0}]}]}}}, "
                    EZ, "At must be from 0");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Wave\": \"a.wav\", "
                    "\"Quake\": 0}]}]}}}, " EZ, "one of Follow, Wave, Quake or Duration");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Height\": 1}]}]}}}, " EZ,
                    "one of Follow, Wave, Quake or Duration");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Quake\": 0, \"Height\": 1}]}]}}}, "
                    EZ, "belong to a Wave");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Wave\": \"a.wav\", "
                    "\"Height\": 1, \"AtPlayer\": true}]}]}}}, " EZ, "has no Height");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Follow\": false}]}]}}}, " EZ,
                    "Follow must be true");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Fly\": true}]}]}}}, " EZ,
                    "unknown key \"Fly\"");
    expect_rejected(HEAD "\"RagEffect\": {\"5\": {\"Play\": {\"Timeline\": [{\"Do\": [{\"Duration\": -5}]}]}}}, " EZ,
                    "Duration must be from 0");
    expect_rejected(HEAD "\"RagEffect\": {}, \"Ez2StrFile\": {\"9\": {\"File\": \"a.str\", \"Open\": true, "
                    "\"MinFile\": \"b.str\"}}}", "exclude each other");
    expect_rejected(HEAD "\"RagEffect\": {}, \"Ez2StrFile\": {\"9\": {\"File\": \"a.str\", \"FullOnly\": false}}}",
                    "FullOnly must be true");
    cases += g_rejected;

    reset();
    set_override(NULL);
    printf("Effect data: %d cases passed: the embedded data, an override file, a missing file and %d rejected "
           "files that fall back to the embedded data.\n", cases, g_rejected);
    return 0;
}
