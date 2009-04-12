#ifndef _TOKENIZERS_H
#define _TOKENIZERS_H

int swcli_tokenize_line(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_number(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_mac(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_word_mixed(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);
int swcli_tokenize_line_mixed(struct cli_context *ctx, const char *buf, struct menu_node **tree, struct tokenize_out *out);

#endif
