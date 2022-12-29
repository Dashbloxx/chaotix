unsigned short int switch_endian16(unsigned short int number) {
    return (number >> 8) | (number << 8);
}

unsigned int switch_endian32(unsigned int number) {
    return ((number >> 24) & 0xff) | ((number << 8) & 0xff0000) | ((number >> 8) & 0xff00) | ((number << 24) & 0xff000000);
}