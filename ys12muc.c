/*
 * ys12muc: decompiler from Ys-I (PC88) to MUCOM88 MML
 *
 * Copyright (c) 2020 Hirokuni Yano
 *
 * Released under the MIT license.
 * see https://opensource.org/licenses/MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

/* global option(s) */
bool g_opt_verbose = false;
bool g_opt_ignore_warning = false;

#define BUFF_SIZE (0x10000)
uint8_t g_data[BUFF_SIZE];

typedef enum
{
    SOUND_TYPE_NONE = 0x0000,
    SOUND_TYPE_FM = 0x0001,
    SOUND_TYPE_SSG = 0x0002,
} SOUND_TYPE;

int DBG(const char *format, ...)
{
    va_list va;
    int ret = 0;

    va_start(va, format);
    if (g_opt_verbose)
    {
        ret = vprintf(format, va);
    }
    va_end(va);
    fflush(stdout);

    return ret;
}

int WARN(const char *format, ...)
{
    va_list va;
    int ret = 0;

    va_start(va, format);
    if (g_opt_verbose || !g_opt_ignore_warning)
    {
        ret = vprintf(format, va);
    }
    va_end(va);

    if (!g_opt_ignore_warning)
    {
        fprintf(stderr, "exit with warning. try -w option to apply workaround.\n");
        exit(1);
    }

    return ret;
}

uint32_t get_word(const uint8_t *p)
{
    return (uint32_t)p[0] + ((uint32_t)p[1] << 8);
}

uint32_t get_word_be(const uint8_t *p)
{
    return (uint32_t)p[1] + ((uint32_t)p[0] << 8);
}

void dump_inst(FILE *fp, const uint8_t *data, uint32_t num, uint32_t offset)
{
    const unsigned char *d = data;
    uint32_t o = offset;

    fprintf(fp, "  @%%%03u\n", num);
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4; // DT/ML
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4; // TL
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4; // KS/AR
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4; // DR
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4; // SR
    fprintf(fp, "  $%03X,$%03X,$%03X,$%03X\n", d[o], d[o + 1], d[o + 2], d[o + 3]);
    o += 4;                         // SL/RR
    fprintf(fp, "  $%03X\n", d[o]); // FB/AL
    fprintf(fp, "\n");
}

void convert_inst(FILE *fp, const uint8_t *data, uint32_t base_addr)
{
    uint32_t i;
    uint32_t a = 5;
    uint32_t e = get_word(&data[6]) - base_addr;
    uint32_t n = (e - a) / 3;

    for (i = 0; i < n; i++)
    {
        dump_inst(fp, data, data[5 + i * 3], get_word(&data[6 + i * 3]) - base_addr);
    }
}

void detect_clock(const uint32_t len_count[256], uint32_t *clock, uint32_t *deflen)
{
    const struct
    {
        uint32_t clock;
        uint32_t count;
    } count_table[] = {
        {192, len_count[192] + len_count[96] + len_count[48] + len_count[24] + len_count[12] + len_count[6] + len_count[3]},
        {144, len_count[144] + len_count[72] + len_count[36] + len_count[18] + len_count[9]},
        {128, len_count[128] + len_count[64] + len_count[32] + len_count[16] + len_count[8] + len_count[4] + len_count[2]},
        {112, len_count[112] + len_count[56] + len_count[28] + len_count[14] + len_count[7]},
        {0, 0},
    };
    uint32_t c;
    uint32_t l;

    {
        DBG("----------------\n");
        for (uint32_t i = 0; count_table[i].clock != 0; i++)
        {
            DBG("%3d: %4d\n", count_table[i].clock, count_table[i].count);
        }
        DBG("--------\n");
        for (uint32_t i = 0; i < 20; i++)
        {
            DBG("%3d:", i * 10);
            for (uint32_t j = 0; j < 10; j++)
            {
                DBG(" %4d", len_count[i * 10 + j]);
            }
            DBG("\n");
        }
        DBG("----------------\n");
    }

    c = 0;
    for (uint32_t i = 1; count_table[i].clock != 0; i++)
    {
        if (count_table[i].count > count_table[c].count)
        {
            c = i;
        }
    }

    l = 1;
    for (uint32_t i = 1; i < 7; i++)
    {
        if (len_count[count_table[c].clock / (1 << i)] > len_count[count_table[c].clock / l])
        {
            l = 1 << i;
        }
    }

    *clock = count_table[c].clock;
    *deflen = l;
}

void parse_music(
    const uint8_t *data, uint32_t offset,
    uint32_t ch, SOUND_TYPE sound_type,
    uint32_t *clock, uint32_t *deflen)
{
    const uint8_t *d = data;
    uint32_t o = offset;
    uint32_t c;
    uint32_t len;
    bool quit = false;
    uint32_t len_count[256];

    memset(len_count, 0, sizeof(len_count));

    while (!quit)
    {
        c = d[o++];
        if (c == 0x00)
        {
            quit = true;
        }
        else if (c >= 0xf0 && c != 0xfa && c != 0xfb)
        {
            switch (c)
            {
            case 0xf2:
            case 0xf4:
            case 0xf9:
                break;
            case 0xf7:
            case 0xf8:
            case 0xfe:
            case 0xff:
                o++;
                break;
            case 0xef:
            case 0xfd:
                o += 2;
                break;
            case 0xf3:
                o += 4;
                break;
            case 0xf6:
                o += 5;
                break;
            case 0xfc:
                o += 6;
                break;
            case 0xf5:
                o += 3;
                if (sound_type == SOUND_TYPE_FM || ch == 2)
                {
                    o++;
                }
                break;
            case 0xf0:
            case 0xf1:
                break;
            }
        }
        else if ((c < 0xef && c >= 0x80) || c == 0xfa)
        {
            if (c == 0xfa)
            {
                len = d[o++];
            }
            else
            {
                len = c & 0x7f;
            }
            len_count[len]++;
        }
        else
        {
            if (c == 0xfb)
            {
                len = d[o++];
            }
            else
            {
                len = c;
            }
            len_count[len]++;
            o++;
        }
    }

    detect_clock(len_count, clock, deflen);
}

int print_length(FILE *fp, uint32_t clock, uint32_t deflen, uint32_t len)
{
    int ret = 0;
    if (clock % len == 0)
    {
        if (clock / len == deflen)
        {
            /* nothing */
        }
        else
        {
            ret += fprintf(fp, "%u", clock / len);
        }
    }
    else if ((len % 3 == 0) && (clock % (len / 3 * 2) == 0))
    {
        if (clock / (len / 3 * 2) == deflen)
        {
            ret += fprintf(fp, ".");
        }
        else
        {
            ret += fprintf(fp, "%u.", clock / (len / 3 * 2));
        }
    }
    else
    {
        ret += fprintf(fp, "%%%u", len);
    }
    return ret;
}

void convert_music(FILE *fp, uint32_t ch, SOUND_TYPE sound_type, const char *chname,
                   const uint8_t *data, uint32_t base_addr, uint32_t data_addr, uint32_t tempo_div)
{
    static const char *notestr[16] = {
        "c", "c+", "d", "d+", "e", "?", "f", "f+", "g", "g+", "a", "a+", "b",
        "?", "?", "?"};
    const uint8_t *d = data;
    uint32_t h = data_addr - base_addr;
    uint32_t o = get_word(&data[h + 1 + ch * 4]) - base_addr;
    uint32_t loop_offset = get_word(&data[h + 1 + ch * 4 + 2]) - base_addr;
    uint32_t repeat_offset = UINT32_MAX;
    uint32_t repeat_count = 0;
    uint32_t c;
    uint32_t prev_oct, oct, note, len;
    uint32_t clock, deflen;
    uint32_t tempo = d[h];
    bool slow_down = false;
    uint32_t slow_down_count = 12;
    bool init = false;
    bool quit = false;
    int ll;

    parse_music(data, o, ch, sound_type, &clock, &deflen);

    ll = 0;
    prev_oct = 0xff;

    if (ch == 0)
    {
        fprintf(fp, "%s t%u\n", chname, 256 - (256 - tempo) * tempo_div);
    }

    while (!quit)
    {
        if (ll <= 0)
        {
            fprintf(fp, "\n");
            ll = 70;
            ll -= fprintf(fp, "%s ", chname);
            if (!init)
            {
                ll -= fprintf(fp, "C%ul%u", clock, deflen);
                init = true;
            }
        }

        if (o == repeat_offset)
        {
            ll -= fprintf(fp, "]%u", repeat_count);
        }
        if (o == loop_offset)
        {
            ll -= fprintf(fp, " L ");
        }

        c = d[o++];
        DBG("[%04x]: %02x\n", o - 1, c);
        if (c == 0x00)
        {
            quit = true;
        }
        else if (c >= 0xf0 && c != 0xfa && c != 0xfb)
        {
            switch (c)
            {
            case 0xf1:
                slow_down = true;
                break;
            case 0xf2:
                ll -= fprintf(fp, "S0,0,0,0");
                break;
            case 0xf3:
                ll -= fprintf(fp, "S%u,%u,%u,%u",
                              (uint32_t)d[o + 0], (uint32_t)d[o + 1], (uint32_t)d[o + 2], (uint32_t)d[o + 3]);
                o += 4;
                break;
            case 0xf4:
                ll -= fprintf(fp, "MF0");
                break;
            case 0xf5:
                if (sound_type & SOUND_TYPE_FM)
                {
                    ll -= fprintf(fp, "M%u,%u,%d,%u",
                                  (uint32_t)d[o], (uint32_t)d[o + 1],
                                  (int8_t)d[o + 2], (uint32_t)d[o + 3]);
                    o += 4;
                    if (ch == 2)
                    {
                        o++;
                    }
                }
                break;
            case 0xf6:
                repeat_count = d[o++];
                repeat_offset = o + 2 + get_word_be(&d[o]);
                o += 2;
                ll -= fprintf(fp, "[");
                break;
            case 0xf7:
                c = d[o++];
                if (c != 0xff)
                {
                    ll -= fprintf(fp, "w%u", c);
                }
                break;
            case 0xf8:
                c = d[o++];
                DBG("P(f8): %02x\n", c);
                ll -= fprintf(fp, "P%u", (uint32_t)(((c << 1) & 2) | ((c >> 3) & 1)));
                break;
            case 0xf9:
                ll -= fprintf(fp, "&");
                break;
            case 0xfc:
                ll -= fprintf(fp, "E%u,%u,%u,%u,%u,%u",
                              (uint32_t)d[o + 0], (uint32_t)d[o + 1], (uint32_t)d[o + 2],
                              (uint32_t)d[o + 3], (uint32_t)d[o + 4], (uint32_t)d[o + 5]);
                o += 6;
                break;
            case 0xfd:
                if (sound_type & SOUND_TYPE_FM)
                {
                    ll -= fprintf(fp, "D%d", (int16_t)get_word_be(&d[o]));
                }
                else
                {
                    ll -= fprintf(fp, "D%d", -(int16_t)get_word_be(&d[o]));
                }
                o += 2;
                break;
            case 0xfe:
                ll -= fprintf(fp, "v%u", d[o++]);
                break;
            case 0xff:
                if (sound_type & SOUND_TYPE_FM)
                {
                    ll -= fprintf(fp, "@%u", (uint32_t)d[o++]);
                }
                else
                {
                    o++;
                }
                break;
            }
        }
        else if ((c < 0xef && c >= 0x80) || c == 0xfa)
        {
            if (c == 0xfa)
            {
                len = d[o++];
            }
            else
            {
                len = c & 0x7f;
            }
            if (!slow_down)
            {
                ll -= fprintf(fp, "r");
                ll -= print_length(fp, clock, deflen, len);
            }
            else
            {
                while (len >= slow_down_count)
                {
                    ll -= fprintf(fp, "r");
                    ll -= print_length(fp, clock, deflen, slow_down_count);
                    len -= slow_down_count;
                    slow_down_count = 12;
                    tempo--;
                    ll -= fprintf(fp, "t%u", 256 - (256 - tempo) * tempo_div);
                }
                if (len > 0)
                {
                    ll -= fprintf(fp, "r");
                    ll -= print_length(fp, clock, deflen, len);
                    slow_down_count -= len;
                }
            }
        }
        else
        {
            if (c == 0xfb)
            {
                len = d[o++];
            }
            else
            {
                len = c;
            }
            oct = ((d[o] >> 4) & 0x07) + 1;
            note = d[o] & 0x0f;
            if (oct != prev_oct)
            {
                if (oct == prev_oct + 1)
                {
                    ll -= fprintf(fp, ">");
                }
                else if (oct == prev_oct - 1)
                {
                    ll -= fprintf(fp, "<");
                }
                else
                {
                    ll -= fprintf(fp, "o%u", oct);
                }
                prev_oct = oct;
            }
            if (!slow_down)
            {
                ll -= fprintf(fp, "%s", notestr[note]);
                ll -= print_length(fp, clock, deflen, len);
            }
            else
            {
                while (len >= slow_down_count)
                {
                    ll -= fprintf(fp, "%s", notestr[note]);
                    ll -= print_length(fp, clock, deflen, slow_down_count);
                    len -= slow_down_count;
                    slow_down_count = 12;
                    if (len > 0)
                    {
                        fprintf(fp, "&");
                    }
                    tempo--;
                    ll -= fprintf(fp, "t%u", 256 - (256 - tempo) * tempo_div);
                }
                if (len > 0)
                {
                    ll -= fprintf(fp, "%s", notestr[note]);
                    ll -= print_length(fp, clock, deflen, len);
                    slow_down_count -= len;
                }
            }
            o++;
        }
    }

    fprintf(fp, "\n");
}

void help(void)
{
    fprintf(stderr, "Usage: ys12muc [option(s)] file\n");
    fprintf(stderr, "  -h\t\tprint this help message and exit\n");
    fprintf(stderr, "  -v\t\tverbose (debug info)\n");
    fprintf(stderr, "  -w\t\tapply workaround and ignore warnings\n");
    fprintf(stderr, "  -o FILE\toutput file (default: stdout)\n");
    fprintf(stderr, "  -n BGM\tBGM number\n");
    fprintf(stderr, "  -T div\ttempo divisor\n");
    fprintf(stderr, "  -m VERSION\tMUCOM88 version\n");
    fprintf(stderr, "  -t TITLE\ttitle for tag\n");
    fprintf(stderr, "  -a AUTHOR\tauthor for tag\n");
    fprintf(stderr, "  -c COMPOSER\tcomposer for tag\n");
    fprintf(stderr, "  -d DATE\tdate for tag\n");
    fprintf(stderr, "  -C COMMENT\tcomment for tag\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int c;
    FILE *fp;
    uint8_t *data = &g_data[0x0000];
    uint32_t base_addr = 0;
    uint32_t data_addr = 0;
    uint32_t music = 0;
    uint32_t tempo_div = 1;
    uint32_t ch;
    const char *chname[] = {"A", "B", "C", "D", "E", "F"};
    const char *mucom88ver = NULL;
    const char *title = NULL;
    const char *author = NULL;
    const char *composer = NULL;
    const char *date = NULL;
    const char *comment = NULL;
    const char *outfile = NULL;

    /* command line options */
    while ((c = getopt(argc, argv, "vwo:n:T:m:t:a:c:d:C:F:")) != -1)
    {
        switch (c)
        {
        case 'v':
            /* debug option */
            g_opt_verbose = true;
            break;
        case 'w':
            /* apply workaround and ignore warnings */
            g_opt_ignore_warning = true;
            break;
        case 'o':
            outfile = optarg;
            break;
        case 'n':
            music = atoi(optarg);
            break;
        case 'T':
            tempo_div = atoi(optarg);
            break;
        case 'm':
            /* 1.7 is required for using "r%n" */
            mucom88ver = optarg;
            break;
        case 't':
            title = optarg;
            break;
        case 'a':
            author = optarg;
            break;
        case 'c':
            composer = optarg;
            break;
        case 'd':
            date = optarg;
            break;
        case 'C':
            comment = optarg;
            break;
        default:
            help();
            break;
        }
    }

    if (optind != argc - 1)
    {
        help();
    }

    /* read data to buffer */
    fp = fopen(argv[optind], "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Can't open '%s'\n", argv[optind]);
        exit(1);
    }
    fread(g_data, sizeof(uint8_t), sizeof(g_data), fp);
    fclose(fp);

    if (outfile != NULL)
    {
        fp = fopen(outfile, "w");
        if (fp == NULL)
        {
            fprintf(stderr, "Can't open '%s'\n", outfile);
            exit(1);
        }
    }
    else
    {
        fp = stdout;
    }

    base_addr = get_word(&data[0x0000]) & 0xf000;
    if (base_addr == 0x4000) base_addr = 0x4d00;
    if (base_addr == 0x6000) base_addr = 0x6000;
    data_addr = get_word(&data[0x0000]) + 0x0019 * music;

    /* insert tag */
    if (mucom88ver != NULL)
    {
        fprintf(fp, "#mucom88 %s\n", mucom88ver);
    }
    if (title != NULL)
    {
        fprintf(fp, "#title %s\n", title);
    }
    if (author != NULL)
    {
        fprintf(fp, "#author %s\n", author);
    }
    if (composer != NULL)
    {
        fprintf(fp, "#composer %s\n", composer);
    }
    if (date != NULL)
    {
        fprintf(fp, "#date %s\n", date);
    }
    if (comment != NULL)
    {
        fprintf(fp, "#comment %s\n", comment);
    }
    fprintf(fp, "\n");

    /* convert */
    convert_inst(fp, data, base_addr);

    for (ch = 0; ch < 6; ch++)
    {
        convert_music(
            fp,
            ch,
            (ch < 3) ? SOUND_TYPE_FM : SOUND_TYPE_SSG,
            chname[ch],
            data,
            base_addr, data_addr,
            tempo_div);
    }

    if (outfile)
    {
        fclose(fp);
    }

    return 0;
}
