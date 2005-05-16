#ifndef _SHARED_H
#define _SHARED_H

#define CLI_PASS_LEN 30
#define CLI_SECRET_LEN 30
#define CLI_MAX_ENABLE 15
#define CLI_MAX_VTY 15

struct cli_vty_config {
	char passwd[CLI_PASS_LEN];
};

struct cli_config {
	char enable_secret[CLI_MAX_ENABLE + 1][CLI_SECRET_LEN + 1];
	struct cli_vty_config vty_cfg[CLI_MAX_VTY + 1];
};

extern struct cli_config *cfg;

extern int cfg_init(void);
extern int cfg_lock(void);
extern int cfg_unlock(void);

#endif
