#include "swcli.h"
#include "cdp_client.h"

int cmd_sh_cdp(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	FILE *out;

	shared_get_cdp(&cdp);
	if (cdp.enabled) {
		out =  ctx->out_open(ctx, 0);
		fprintf(out, "Global CDP information:\n"
				"\tSending CDP packets every %d seconds\n"
				"\tSending a holdtime value of %d seconds\n"
				"\tSending CDPv2 advertisements is %s\n",
				cdp.timer, cdp.holdtime, cdp.version==2? "enabled" : "disabled");
		fclose(out);
	}

	return 0;
}

int cmd_sh_cdp_entry(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_session *cdp;
	struct cdp_configuration cfg;
	int protocol, version, err, i;
	char *entry = NULL;
	FILE *out;

	shared_get_cdp(&cfg);
	if (!cfg.enabled)
		return 0;

	for (i=3; i<argc; i++) {
		if (!strncmp(nodev[i]->name, "protocol", strlen(nodev[i]->name)))
			protocol = 1;
		else if (!strncmp(nodev[i]->name, "version", strlen(nodev[i]->name)))
			version = 1;
		else if (strcmp(argv[i], "*"))
			entry = argv[i];
	}

	CDP_SESSION_OPEN(ctx, cdp);

	if ((err = cdp_get_neighbors(cdp, 0, entry)))
		goto out_close;

	out = ctx->out_open(ctx, 1);

	if (protocol || version)
		cdp_print_neighbors_filtered(cdp, out, protocol, version);
	else
		cdp_print_neighbors_detail(cdp, out);

	fclose(out);
out_close:
	CDP_SESSION_CLOSE(ctx, cdp);

	return err;
}

int cmd_sh_cdp_holdtime(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	FILE *out;

	shared_get_cdp(&cdp);
	if (cdp.enabled) {
		out =  ctx->out_open(ctx, 0);
		fprintf(out, "%d secs\n", cdp.holdtime);
		fclose(out);
	}
	return 0;
}

int cmd_sh_cdp_int(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_session *cdp;
	struct cdp_configuration cfg;
	int err, sock_fd, if_index = 0;
	char buf[IFNAMSIZ];
	FILE *out;

	shared_get_cdp(&cfg);
	if (!cfg.enabled)
		return 0;

	if (argc > 3) {
		if (!strncasecmp(nodev[3]->name, "netdev", strlen(nodev[3]->name))) {
			SW_SOCK_OPEN(ctx, sock_fd);
			if_index = if_get_index(argv[4], sock_fd);
			SW_SOCK_CLOSE(ctx, sock_fd);
		}
		else if (!strncasecmp(nodev[3]->name, "ethernet", strlen(nodev[3]->name))) {
			memset(buf, 0, sizeof(buf));
			snprintf(buf, IFNAMSIZ, "eth%s", argv[4]);
			SW_SOCK_OPEN(ctx, sock_fd);
			if_index = if_get_index(buf, sock_fd);
			SW_SOCK_CLOSE(ctx, sock_fd);
		}
		else
			return -ENODEV;
	}

	CDP_SESSION_OPEN(ctx, cdp);

	if ((err = cdp_get_interfaces(cdp, if_index)))
		goto out_close;

	out = ctx->out_open(ctx, 1);

	/* FIXME FIXME FIXME: print cdp interfaces. Need function for
	 * showing interface status (interface up/down, line protocol
	 * up/down).
	 */
	fclose(out);
out_close:
	CDP_SESSION_CLOSE(ctx, cdp);

	return err;
}

int cmd_sh_cdp_ne(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cfg;
	struct cdp_session *cdp;
	FILE *out;
	int err;

	shared_get_cdp(&cfg);
	if (!cfg.enabled)
		return 0;

	CDP_SESSION_OPEN(ctx, cdp);

	if ((err = cdp_get_neighbors(cdp, 0, NULL)))
		goto out_err;

	out = ctx->out_open(ctx, 1);

	cdp_print_neighbors_brief(cdp, out);

	fclose(out);
out_err:
	CDP_SESSION_CLOSE(ctx, cdp);
	return err;
}

int cmd_sh_cdp_run(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	FILE *out;

	shared_get_cdp(&cdp);

	out =  ctx->out_open(ctx, 1);
	fprintf(out, "CDP is %s\n", cdp.enabled? "enabled" : "disabled");
	fclose(out);

	return 0;
}

int cmd_sh_cdp_timer(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_configuration cdp;
	FILE *out;

	shared_get_cdp(&cdp);

	if (cdp.enabled) {
		out = ctx->out_open(ctx, 1);
		fprintf(out, "%d secs\n", cdp.timer);
	}
	return 0;
}

int cmd_sh_cdp_traffic(struct cli_context *ctx, int argc, char **argv, struct menu_node **nodev)
{
	struct cdp_session *cdp;
	struct cdp_configuration cfg;
	struct cdp_traffic_stats stats;
	FILE *out;
	int err;

	shared_get_cdp(&cfg);
	if (!cfg.enabled)
		return 0;

	CDP_SESSION_OPEN(ctx, cdp);

	if ((err = cdp_get_stats(cdp, &stats)))
		goto out_close;

	out = ctx->out_open(ctx, 1);
	fprintf(out, "CDP counters:\n"
			"\tTotal packets output: %u, Input: %u\n"
			"\tCDP version 1 advertisements output: %u, Input: %u\n"
			"\tCDP version 2 advertisements output: %u, Input: %u\n",
			stats.v1_out + stats.v2_out, stats.v1_in + stats.v2_in,
			stats.v1_out, stats.v1_in, stats.v2_out, stats.v2_in);
	fclose(out);

out_close:
	CDP_SESSION_CLOSE(ctx, cdp);
	return err;
}
