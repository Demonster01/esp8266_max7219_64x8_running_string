// converts utf8 cyrillic to cp1251
String utf8ukr(String source) {
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length();
  i = 0;

  while (i < k) {
    n = source[i];
    i++;

    if (n >= 0xC0) { // 0xC0 = B1100 0000
      switch (n) {
        case 0xD0: { // 0xD0 = B1101 0000
          n = source[i];
          i++;
          if (n == 0x81) {
            n = 0xA8; // Ё
            break;
          }
          if (n == 0x84) {
            n = 0xAA; // Є
            break;
          }
          if (n == 0x86) {
            n = 0xB2; // І
            break;
          }
          if (n == 0x87) {
            n = 0xAF; // Ї
            break;
          }
          if (n >= 0x90 && n <= 0xBF) { // from А to п
            n += 0x2F;
          }
          break;
        }
        case 0xD1: { // 0xD1 = B1101 0001
          n = source[i];
          i++;
          if (n == 0x91) {
            n = 0xB8; // ё
            break;
          }
          if (n == 0x94) {
            n = 0xBA; // є
            break;
          }
          if (n == 0x96) {
            n = 0xB3; // і
            break;
          }
          if (n == 0x97) {
            n = 0xBF; // ї
            break;
          }
          if (n >= 0x80 && n <= 0x8F) { // from р to я
            n += 0x6F;
          }
          break;
        }
      }
    }
    m[0] = n;
    target += String(m);
  }
  return target;
}
