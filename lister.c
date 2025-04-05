#include <stdio.h>
#include "symtab.h"
#include "vcc.h"



// void print_source_char(void) {
// 	if (current_char == new_line_char) {
// 		start_new_line();
// 		return;
// 	}
// 	if (++listing_pos == 1) {
// 		format_new_line();
// 	}
// 	if (listing_pos > line_size) {
// 		wrap_new_line();
// 	}
// 	fprintf(listing_file, "%c\n", current_char);
// }

// void start_new_line(void) {
// 	if(++listing_line > page_length) {
// 		start_new_page();
// 	}
// 	fprintf(listing_file, "%c", new_line_char);
// 	listing_pos = 0;
// 	return;
// }

// void start_new_page(void) {
// 	if(++listing_page > 1) {
// 		printf_footing();
// 	}
// 	fprintf(listing_file, "%c", new_page_char);
// 	print_heading();
// 	listing_line = 1;
// 	return;
// }

// void wrap_new_line(void) {
// 	if(++listing_line > page_length) {
// 		start_new_page();
// 	}
// 	fprintf(listing_file, "%c\t", new_line_char);
// 	listing_pos = tab_width;
// 	return;
// }

// void format_new_line(void) {
// 	fprintf(listing_file, "%3d|\t", current_line);
// 	return;
// }