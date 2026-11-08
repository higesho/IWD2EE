/* 行頭・行末禁則: 折る位置 brk（バイト位置）を、禁則に合うように前へずらして返す
 *   s: その行の文字列（Shift_JIS）, len: バイト数, brk: エンジンが決めた折る位置（次の行の先頭）
 * 他の番地を参照しない（表を持たず、比較だけで書く）ので、機械語をそのまま別の場所に置いて呼べる。
 * cl /c /O2 /GS- /Gy- で組み立て、tools/extract_obj.py で機械語を取り出す。
 */
static __forceinline int lead(unsigned c)
{
    return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}

/* 行頭に来てはいけない文字か */
static __forceinline int nobegin(const unsigned char *s, int rest)
{
    unsigned c = s[0];
    if (lead(c) && rest >= 2) {
        unsigned w = (c << 8) | s[1];
        if (w >= 0x8141 && w <= 0x814B) return 1;   /* 、。，．・：；？！゛゜ */
        if (w >= 0x8152 && w <= 0x8155) return 1;   /* ヽヾゝゞ */
        if (w == 0x8158 || w == 0x815B || w == 0x815C || w == 0x815D) return 1; /* 々ー―‐ */
        if (w == 0x8160 || w == 0x8163 || w == 0x8164) return 1;   /* ～…‥ */
        if (w == 0x8166 || w == 0x8168) return 1;   /* ’” */
        if (w == 0x816A || w == 0x816C || w == 0x816E || w == 0x8170 || w == 0x8172 ||
            w == 0x8174 || w == 0x8176 || w == 0x8178 || w == 0x817A) return 1; /* ）〕］｝〉》」』】 */
        if (w == 0x829F || w == 0x82A1 || w == 0x82A3 || w == 0x82A5 || w == 0x82A7 ||
            w == 0x82C1 || w == 0x82E1 || w == 0x82E3 || w == 0x82E5 || w == 0x82EC) return 1; /* ぁぃぅぇぉっゃゅょゎ */
        if (w == 0x8340 || w == 0x8342 || w == 0x8344 || w == 0x8346 || w == 0x8348 ||
            w == 0x8362 || w == 0x8383 || w == 0x8385 || w == 0x8387 || w == 0x838E ||
            w == 0x8394 || w == 0x8395) return 1;   /* ァィゥェォッャュョヮヵヶ */
        return 0;
    }
    return c == ')' || c == ']' || c == '}' || c == ',' || c == '.' || c == ':' ||
           c == ';' || c == '!' || c == '?' || c == '%';
}

/* 行末に来てはいけない文字か */
static __forceinline int noend(const unsigned char *s, int rest)
{
    unsigned c = s[0];
    if (lead(c) && rest >= 2) {
        unsigned w = (c << 8) | s[1];
        return w == 0x8165 || w == 0x8167 || w == 0x8169 || w == 0x816B || w == 0x816D ||
               w == 0x816F || w == 0x8171 || w == 0x8173 || w == 0x8175 || w == 0x8177 ||
               w == 0x8179;   /* ‘“（〔［｛〈《「『【 */
    }
    return c == '(' || c == '[' || c == '{';
}

/* b より前の文字の先頭（無ければ -1） */
static __forceinline int prevstart(const unsigned char *s, int len, int b)
{
    int i = 0, p = -1;
    while (i < b) {
        p = i;
        i += (lead(s[i]) && i + 1 < len) ? 2 : 1;
    }
    return i == b ? p : -2;   /* -2: 文字の途中（触らない） */
}

int __cdecl kinsoku(const unsigned char *s, int len, int brk)
{
    if (!s || brk <= 0 || brk > len) return brk;
    int b = brk;
    for (int k = 0; k < 4; k++) {
        int p = prevstart(s, len, b);
        if (p == -2) return brk;
        int bad = (b < len && nobegin(s + b, len - b)) || (p >= 0 && noend(s + p, len - p));
        if (!bad) break;
        if (p <= 0) return brk;       /* 前へずらすと行が空になる */
        b = p;
    }
    return b;
}
