#pragma once

// enter a char into serial port
// use sbi printch function
void k_port_write_ch(char ch);

// enter a message into seraial port
// use sbi printstr function
void k_port_write(char *buf);
long k_port_read();
