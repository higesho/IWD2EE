/* Icewind Dale II（IWD2EE）日本語化の道具
 *   ゲームのフォルダで実行すると、次の英文を IWD2JP.dat の訳に置き換え、
 *   icewind2.ini に日本語表示の設定を書く。
 *     dialog.tlk の全文
 *     Party.ini のパーティー名と説明
 *     Characters\*.RES（キャラクターの経歴）と *.CHR（キャラクターの名前）
 *   IWD2JP.exe /restore … 最初に控えた英語のファイルに戻す
 * 訳の表は英文そのものを持たず、英文（\r を除いた cp1252 のバイト列）の FNV-1a 64 ビット値で引く。
 * 何度実行しても同じ結果になる（日本語になった文は値が合わないので、そのまま残る）。
 */
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long u64;
typedef unsigned int u32;

/* dir: いま処理しているフォルダ（ゲームのフォルダ、または VirtualStore の写し）
 * exedir: この exe と IWD2JP.dat・IWD2JP.lua があるフォルダ */
static char dir[MAX_PATH], bakdir[MAX_PATH], exedir[MAX_PATH];

static u64 fnv(const unsigned char *p, u32 n)
{
    u64 h = 0xcbf29ce484222325ULL;
    for (u32 i = 0; i < n; i++) {
        if (p[i] == '\r') continue;
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static unsigned char *load(const char *path, u32 *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = (unsigned char *)malloc(n + 1);
    if (fread(b, 1, n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
    fclose(f);
    b[n] = 0;
    *size = (u32)n;
    return b;
}

static int save(const char *path, const void *p, size_t n)
{
    char tmp[MAX_PATH];
    sprintf(tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (f) {
        size_t w = fwrite(p, 1, n, f);
        fclose(f);
        if (w == n && MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING)) return 1;
        DeleteFileA(tmp);
    }
    /* 置き換えできないとき（振り替えの下など）は直接書く */
    f = fopen(path, "wb");
    if (!f) return 0;
    size_t w = fwrite(p, 1, n, f);
    return fclose(f) == 0 && w == n;
}

static void finish(int code)
{
    printf("\nEnter キーで閉じます。");
    getchar();
    exit(code);
}

/* IWD2JP.dat: "IWD2JP01" 件数(u32) [ハッシュ(u64) 位置(u32) 長さ(u32)]*件数 文字列（cp932, 改行は \n） */
typedef struct { u64 h; u32 off, len; } Ent;
static Ent *ents;
static u32 nent;
static const unsigned char *pool;

static Ent *find(u64 h)
{
    u32 lo = 0, hi = nent;
    while (lo < hi) {
        u32 mid = (lo + hi) / 2;
        if (ents[mid].h < h) lo = mid + 1;
        else hi = mid;
    }
    return (lo < nent && ents[lo].h == h) ? &ents[lo] : NULL;
}

/* 訳を取り出す。crlf なら \n を \r\n に、oneline なら改行を空白にする。呼び出し側で free */
static unsigned char *trans(Ent *m, int crlf, int oneline, u32 *outlen)
{
    const unsigned char *j = pool + m->off;
    unsigned char *c = (unsigned char *)malloc(m->len * 2 + 1);
    u32 k = 0;
    for (u32 q = 0; q < m->len; q++) {
        if (j[q] == '\n' && oneline) { c[k++] = ' '; continue; }
        if (j[q] == '\n' && crlf) c[k++] = '\r';
        c[k++] = j[q];
    }
    c[k] = 0;
    *outlen = k;
    return c;
}

/* 控え: IWD2JP_backup\<相対パス> に、無いときだけ写す */
static int backup(const char *rel)
{
    char src[MAX_PATH], dst[MAX_PATH];
    sprintf(src, "%s\\%s", dir, rel);
    sprintf(dst, "%s\\%s", bakdir, rel);
    if (GetFileAttributesA(dst) != INVALID_FILE_ATTRIBUTES) return 1;
    char d2[MAX_PATH];
    strcpy(d2, dst);
    *strrchr(d2, '\\') = 0;
    CreateDirectoryA(bakdir, NULL);
    CreateDirectoryA(d2, NULL);
    return CopyFileA(src, dst, TRUE);
}

static int restore_one(const char *rel)
{
    char src[MAX_PATH], dst[MAX_PATH];
    sprintf(src, "%s\\%s", bakdir, rel);
    sprintf(dst, "%s\\%s", dir, rel);
    if (GetFileAttributesA(src) == INVALID_FILE_ATTRIBUTES) return 0;
    return CopyFileA(src, dst, FALSE);
}

static int is_space(unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

/* ---- dialog.tlk ---- */
static void do_tlk(void)
{
    char tlk[MAX_PATH];
    sprintf(tlk, "%s\\dialog.tlk", dir);
    if (GetFileAttributesA(tlk) == INVALID_FILE_ATTRIBUTES) return;   /* VirtualStore の写しに無いときなど */
    u32 tsize;
    unsigned char *t = load(tlk, &tsize);
    if (!t || tsize < 18 || memcmp(t, "TLK V1  ", 8)) { printf("dialog.tlk を読めません。\n"); finish(1); }
    u32 cnt = *(u32 *)(t + 10), soff = *(u32 *)(t + 14);
    if (18 + 26ULL * cnt > tsize) { printf("dialog.tlk が壊れています。\n"); finish(1); }
    if (!backup("dialog.tlk")) { printf("dialog.tlk の控えを作れませんでした。\n"); finish(1); }

    size_t cap = tsize * 2 + 1024 * 1024, used = 0;
    unsigned char *body = (unsigned char *)malloc(cap);
    unsigned char *tab = (unsigned char *)malloc(26 * cnt);
    memcpy(tab, t + 18, 26 * cnt);
    u32 hit = 0, total = 0;
    for (u32 i = 0; i < cnt; i++) {
        unsigned char *e = tab + 26 * i;
        u32 o = *(u32 *)(e + 18), l = *(u32 *)(e + 22);
        if ((u64)soff + o + l > tsize) l = 0;
        const unsigned char *s = t + soff + o;
        const unsigned char *src = s;
        u32 slen = l;
        unsigned char *conv = NULL;
        if (l) {
            total++;
            Ent *m = find(fnv(s, l));
            if (m) {
                conv = trans(m, memchr(s, '\r', l) != NULL, 0, &slen);
                src = conv;
                hit++;
            }
        }
        if (used + slen > cap) { cap = (used + slen) * 2; body = (unsigned char *)realloc(body, cap); }
        memcpy(body + used, src, slen);
        *(u32 *)(e + 18) = (u32)used;
        *(u32 *)(e + 22) = slen;
        used += slen;
        free(conv);
    }
    size_t outn = 18 + 26 * (size_t)cnt + used;
    unsigned char *out = (unsigned char *)malloc(outn);
    memcpy(out, t, 18);
    memcpy(out + 18, tab, 26 * cnt);
    memcpy(out + 18 + 26 * cnt, body, used);
    if (!save(tlk, out, outn)) { printf("dialog.tlk を書き換えられませんでした（書き込み権限を確かめてください）。\n"); finish(1); }
    printf("dialog.tlk: %u / %u 文を日本語にしました。\n", hit, total);
    free(out); free(body); free(tab); free(t);
}

/* ---- Party.ini ---- */
static void do_party(void)
{
    char path[MAX_PATH];
    sprintf(path, "%s\\Party.ini", dir);
    u32 n;
    unsigned char *b = load(path, &n);
    if (!b) return;
    backup("Party.ini");
    size_t cap = n * 2 + 4096, used = 0;
    unsigned char *out = (unsigned char *)malloc(cap);
    u32 hit = 0;
    u32 i = 0;
    while (i < n) {
        u32 e = i;
        while (e < n && b[e] != '\n') e++;
        u32 le = e < n ? e + 1 : e;          /* 改行を含めた行末 */
        u32 ce = e;                           /* 改行を除いた行末 */
        if (ce > i && b[ce - 1] == '\r') ce--;
        const unsigned char *line = b + i;
        u32 ll = ce - i;
        int done = 0;
        /* キー = 値 */
        u32 k = 0;
        while (k < ll && is_space(line[k])) k++;
        if (k < ll && (strncmp((const char *)line + k, "Name", 4) == 0 || strncmp((const char *)line + k, "Descr", 5) == 0)) {
            const unsigned char *eq = (const unsigned char *)memchr(line, '=', ll);
            if (eq) {
                u32 vs = (u32)(eq - line) + 1;
                while (vs < ll && is_space(line[vs])) vs++;
                u32 ve = ll;
                while (ve > vs && is_space(line[ve - 1])) ve--;
                Ent *m = ve > vs ? find(fnv(line + vs, ve - vs)) : NULL;
                if (m) {
                    u32 jl;
                    unsigned char *j = trans(m, 0, 1, &jl);
                    size_t need = used + vs + jl + (le - ce) + 4;
                    if (need > cap) { cap = need * 2; out = (unsigned char *)realloc(out, cap); }
                    memcpy(out + used, line, vs); used += vs;
                    memcpy(out + used, j, jl); used += jl;
                    memcpy(out + used, b + ce, le - ce); used += le - ce;
                    free(j);
                    hit++;
                    done = 1;
                }
            }
        }
        if (!done) {
            if (used + (le - i) > cap) { cap = (used + (le - i)) * 2; out = (unsigned char *)realloc(out, cap); }
            memcpy(out + used, b + i, le - i);
            used += le - i;
        }
        i = le;
    }
    if (hit && !save(path, out, used)) printf("Party.ini を書き換えられませんでした。\n");
    else printf("Party.ini: %u 項目を日本語にしました。\n", hit);
    free(out); free(b);
}

/* ---- Characters\*.RES と *.CHR ---- */
static void do_chars(void)
{
    char pat[MAX_PATH];
    WIN32_FIND_DATAA fd;
    u32 nres = 0, nchr = 0;
    sprintf(pat, "%s\\Characters\\*.*", dir);
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const char *ext = strrchr(fd.cFileName, '.');
        if (!ext) continue;
        char rel[MAX_PATH], path[MAX_PATH];
        sprintf(rel, "Characters\\%s", fd.cFileName);
        sprintf(path, "%s\\%s", dir, rel);
        u32 n;
        if (_stricmp(ext, ".RES") == 0) {
            unsigned char *b = load(path, &n);
            if (!b) continue;
            u32 s = 0, e = n;
            while (s < e && is_space(b[s])) s++;
            while (e > s && is_space(b[e - 1])) e--;
            Ent *m = e > s ? find(fnv(b + s, e - s)) : NULL;
            if (m) {
                u32 jl;
                unsigned char *j = trans(m, 1, 0, &jl);
                backup(rel);
                if (save(path, j, jl)) nres++;
                free(j);
            }
            free(b);
        } else if (_stricmp(ext, ".CHR") == 0) {
            unsigned char *b = load(path, &n);
            if (!b || n < 40 || memcmp(b, "CHR ", 4)) { free(b); continue; }
            u32 l = 0;
            while (l < 32 && b[8 + l]) l++;
            Ent *m = l ? find(fnv(b + 8, l)) : NULL;
            if (m && m->len < 32) {
                backup(rel);
                memset(b + 8, 0, 32);
                memcpy(b + 8, pool + m->off, m->len);
                if (save(path, b, n)) nchr++;
            }
            free(b);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    printf("キャラクター: 経歴 %u 件、名前 %u 件を日本語にしました。\n", nres, nchr);
}

/* ---- IWD2EE（IEex）: IEex_TRA.lua の訳と、IWD2JP.lua（禁則・パターンの手直し）の組み込み ---- */
static int lead_sj(unsigned char c) { return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc); }

static void do_ieex(void)
{
    char mpath[MAX_PATH], tpath[MAX_PATH], lsrc[MAX_PATH], ldst[MAX_PATH];
    sprintf(mpath, "%s\\override\\M__IEex.lua", dir);
    sprintf(tpath, "%s\\override\\IEex_TRA.lua", dir);
    sprintf(lsrc, "%s\\IWD2JP.lua", exedir);
    sprintf(ldst, "%s\\override\\IWD2JP.lua", dir);
    if (GetFileAttributesA(mpath) == INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesA(tpath) == INVALID_FILE_ATTRIBUTES) return;   /* IWD2EE が無い */

    /* IEex_TRA.lua: 名前 = "英文" の行を訳す */
    u32 n, hit = 0;
    unsigned char *b = load(tpath, &n);
    if (b) {
        backup("override\\IEex_TRA.lua");
        size_t cap = n * 3 + 4096, used = 0;
        unsigned char *out = (unsigned char *)malloc(cap);
        u32 i = 0;
        while (i < n) {
            u32 e = i;
            while (e < n && b[e] != '\n') e++;
            u32 le = e < n ? e + 1 : e;
            const unsigned char *line = b + i;
            u32 ll = le - i;
            const unsigned char *q1 = NULL, *q2 = NULL;
            if (ll > 8 && memcmp(line, "ex_str_", 7) == 0) {
                q1 = (const unsigned char *)memchr(line, '"', ll);
                for (u32 k = ll; k > 0; k--) if (line[k - 1] == '"') { q2 = line + k - 1; break; }
            }
            Ent *m = (q1 && q2 && q2 > q1 + 1) ? find(fnv(q1 + 1, (u32)(q2 - q1 - 1))) : NULL;
            if (m) {
                size_t need = used + ll + m->len * 2 + 16;
                if (need > cap) { cap = need * 2; out = (unsigned char *)realloc(out, cap); }
                u32 head = (u32)(q1 - line) + 1;
                memcpy(out + used, line, head); used += head;
                const unsigned char *j = pool + m->off;
                for (u32 q = 0; q < m->len; q++) {
                    unsigned char c = j[q];
                    if (lead_sj(c) && q + 1 < m->len) {
                        out[used++] = c;
                        unsigned char t2 = j[++q];
                        if (t2 == '\\') out[used++] = '\\';   /* 2 バイト目の \ は lua の記号にならないよう逃がす */
                        out[used++] = t2;
                    } else if (c == '"') {
                        out[used++] = '\\'; out[used++] = '"';
                    } else if (c == '\n') {
                        out[used++] = '\\'; out[used++] = 'n';
                    } else if (c != '\r') {
                        out[used++] = c;
                    }
                }
                u32 tail = (u32)(line + ll - q2);
                memcpy(out + used, q2, tail); used += tail;
                hit++;
            } else {
                if (used + ll > cap) { cap = (used + ll) * 2; out = (unsigned char *)realloc(out, cap); }
                memcpy(out + used, line, ll); used += ll;
            }
            i = le;
        }
        if (hit && !save(tpath, out, used)) printf("IEex_TRA.lua を書き換えられませんでした。\n");
        free(out); free(b);
    }

    /* IWD2JP.lua を override に置き、M__IEex.lua に読み込みの行を足す */
    if (GetFileAttributesA(mpath) == INVALID_FILE_ATTRIBUTES) {
        printf("IWD2EE: 画面の文 %u 件を日本語にしました。\n", hit);
        return;
    }
    int lua_ok = GetFileAttributesA(lsrc) != INVALID_FILE_ATTRIBUTES && CopyFileA(lsrc, ldst, FALSE);
    int hooked = 0;
    unsigned char *mb = load(mpath, &n);
    if (mb && lua_ok) {
        const char *anchor = "dofile(\"override/IEex_IWD2_Patch.lua\")";
        /* IEex_IWD2_Patch.lua の前に読む（その最後でメモリの取り方がゲーム本体のものに切り替わるため） */
        const char *add = "local iwd2jp = io.open(\"override/IWD2JP.lua\", \"r\") -- IWD2JP"
                          "\r\n\t\tif iwd2jp then iwd2jp:close() local ok, err = pcall(dofile, \"override/IWD2JP.lua\")"
                          " if not ok then print(\"IWD2JP: \"..tostring(err)) end end -- IWD2JP\r\n\t\t";
        if (strstr((const char *)mb, "IWD2JP")) hooked = 1;
        else {
            char *p = strstr((char *)mb, anchor);
            if (p) {
                backup("override\\M__IEex.lua");
                size_t pre = (size_t)(p - (char *)mb);
                size_t addn = strlen(add);
                unsigned char *o = (unsigned char *)malloc(n + addn);
                memcpy(o, mb, pre);
                memcpy(o + pre, add, addn);
                memcpy(o + pre + addn, mb + pre, n - pre);
                hooked = save(mpath, o, n + addn);
                free(o);
            }
        }
    }
    free(mb);
    printf("IWD2EE: 画面の文 %u 件を日本語にしました。", hit);
    printf(hooked ? "禁則と文字の手直し（IWD2JP.lua）を組み込みました。\n" : "禁則の手直しは組み込めませんでした。\n");
}

static int do_restore(void)
{
    char ini[MAX_PATH], pat[MAX_PATH];
    sprintf(ini, "%s\\icewind2.ini", dir);
    if (GetFileAttributesA(bakdir) == INVALID_FILE_ATTRIBUTES) return -1;
    int n = 0;
    n += restore_one("dialog.tlk");
    n += restore_one("Party.ini");
    n += restore_one("override\\IEex_TRA.lua");
    n += restore_one("override\\M__IEex.lua");
    {
        char l[MAX_PATH];
        sprintf(l, "%s\\override\\IWD2JP.lua", dir);
        DeleteFileA(l);
    }
    WIN32_FIND_DATAA fd;
    sprintf(pat, "%s\\Characters\\*.*", bakdir);
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char rel[MAX_PATH];
            sprintf(rel, "Characters\\%s", fd.cFileName);
            n += restore_one(rel);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    if (GetFileAttributesA(ini) != INVALID_FILE_ATTRIBUTES) {
        WritePrivateProfileStringA("Program Options", "Double Byte Character Support", "0", ini);
        WritePrivateProfileStringA("Program Options", "Font Name", "", ini);
    }
    return n;
}

/* ---- Windows のファイルの振り替え（VirtualStore） ----
 * IWD2.exe は manifest を持たないので、Program Files の下では書き込みが
 *   %LOCALAPPDATA%\VirtualStore\<ゲームのフォルダ（ドライブ名を除く）>
 * に振り替えられ、以後そちらの写し（ゲームが終了時に書く Icewind2.ini など）が読まれる。
 * この exe も manifest を持たないので、ふつうに実行すればゲームと同じ振り替えを受ける。
 * 管理者として実行されたとき（振り替えが効いていないとき）は、写しがあればそちらも処理する。 */
static int virtualized(void)
{
    HANDLE tok;
    DWORD v = 0, len = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) return 0;
    if (!GetTokenInformation(tok, TokenVirtualizationEnabled, &v, sizeof(v), &len)) v = 0;
    CloseHandle(tok);
    return v != 0;
}

static int vstore_dir(char *out)
{
    char la[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", la, MAX_PATH);
    if (!n || n >= MAX_PATH || strlen(exedir) < 3 || exedir[1] != ':') return 0;
    sprintf(out, "%s\\VirtualStore%s", la, exedir + 2);
    DWORD a = GetFileAttributesA(out);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static void set_dir(const char *d)
{
    strcpy(dir, d);
    sprintf(bakdir, "%s\\IWD2JP_backup", dir);
}

static void write_ini(int must)
{
    char ini[MAX_PATH];
    sprintf(ini, "%s\\icewind2.ini", dir);
    if (!must && GetFileAttributesA(ini) == INVALID_FILE_ATTRIBUTES) return;
    if (WritePrivateProfileStringA("Program Options", "Double Byte Character Support", "1", ini) &&
        WritePrivateProfileStringA("Program Options", "Font Name", "MS Gothic", ini))
        printf("icewind2.ini に日本語表示の設定を書きました。\n");
    else
        printf("icewind2.ini に設定を書けませんでした。\n");
}

int main(int argc, char **argv)
{
    char tlk[MAX_PATH], datp[MAX_PATH], vs[MAX_PATH];
    GetModuleFileNameA(NULL, exedir, MAX_PATH);
    *strrchr(exedir, '\\') = 0;
    set_dir(exedir);
    sprintf(tlk, "%s\\dialog.tlk", exedir);
    sprintf(datp, "%s\\IWD2JP.dat", exedir);
    int virt = virtualized();
    if (GetEnvironmentVariableA("IWD2JP_TEST_NOVIRT", vs, MAX_PATH)) virt = 0;   /* 試験用: 管理者で実行されたときの動きを試す */
    int has_vs = !virt && vstore_dir(vs);

    printf("Icewind Dale II 日本語化\n\n");
    if (GetFileAttributesA(tlk) == INVALID_FILE_ATTRIBUTES) {
        printf("dialog.tlk が見つかりません。この exe を Icewind Dale II のフォルダに置いて実行してください。\n");
        finish(1);
    }
    if (argc > 1 && _stricmp(argv[1], "/restore") == 0) {
        int n = do_restore();
        if (has_vs) {
            set_dir(vs);
            int m = do_restore();
            if (m > 0) n = (n > 0 ? n : 0) + m;
            else {
                /* 写しに控えが無くても、写しの ini があれば日本語表示の設定は外す */
                char ini[MAX_PATH];
                sprintf(ini, "%s\\icewind2.ini", dir);
                if (GetFileAttributesA(ini) != INVALID_FILE_ATTRIBUTES) {
                    WritePrivateProfileStringA("Program Options", "Double Byte Character Support", "0", ini);
                    WritePrivateProfileStringA("Program Options", "Font Name", "", ini);
                }
            }
        }
        if (n < 0) { printf("控え（IWD2JP_backup フォルダ）がありません。\n"); finish(1); }
        printf("%d 個のファイルを英語に戻しました。\n", n);
        printf("（IWD2EE の部品を入れ直した後に戻すと、その分の文は失われます。そのときは IWD2EE を入れ直してください。）\n");
        finish(0);
    }

    u32 dsize;
    unsigned char *dat = load(datp, &dsize);
    if (!dat || dsize < 12 || memcmp(dat, "IWD2JP01", 8)) {
        printf("IWD2JP.dat が見つからないか、壊れています。\n");
        finish(1);
    }
    nent = *(u32 *)(dat + 8);
    ents = (Ent *)malloc(sizeof(Ent) * nent);
    const unsigned char *p = dat + 12;
    for (u32 i = 0; i < nent; i++, p += 16) {
        memcpy(&ents[i].h, p, 8);
        memcpy(&ents[i].off, p + 8, 4);
        memcpy(&ents[i].len, p + 12, 4);
    }
    pool = dat + 12 + 16 * nent;

    do_tlk();
    do_party();
    do_chars();
    do_ieex();
    write_ini(1);

    if (has_vs) {
        /* 管理者として実行されたとき: ゲームが実際に読む VirtualStore の写しも日本語にする */
        printf("\nWindows が振り替えたゲームのファイルの写し（%s）も直します。\n", vs);
        set_dir(vs);
        do_tlk();
        do_party();
        do_chars();
        do_ieex();
        write_ini(0);
    }

    printf("元のファイルは IWD2JP_backup フォルダに控えてあります（/restore で戻せます）。\n");
    printf("\nIWD2EE.exe からゲームを起動してください。\n");
    printf("IWD2EE の部品を入れ直したときは、もう一度この exe を実行してください。\n");
    finish(0);
    return 0;
}
