#pragma once

// enter a char into serial port
// use sbi printch function
void port_write_ch(char ch);

// enter a message into seraial port
// use sbi printstr function
void port_write(char *buf);
long k_port_read();
