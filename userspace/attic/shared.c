/* FIXME FIXME FIXME: These functions were part of the old
 * shared.c lib. Find an apropriate place to move these
 */

static void sw_redisplay_password(void) {
	fprintf(rl_outstream, "\rPassword: ");
	fflush(rl_outstream);
}

int cfg_checkpass(int retries, int (*validator)(char *, void *), void *arg) {
	rl_voidfunc_t *old_redisplay = rl_redisplay_function;
	char *pw;
	int i;

	rl_redisplay_function = sw_redisplay_password;
	for(i = 0; i < retries; i++) {
		pw = readline(NULL);
		if(validator(pw, arg))
			break;
	}
	rl_redisplay_function = old_redisplay;
	return i < retries;
}

static void sw_redisplay_void(void) {
}

int cfg_waitcr(void) {
	rl_voidfunc_t *old_redisplay = rl_redisplay_function;

	rl_redisplay_function = sw_redisplay_void;
	readline(NULL);
	rl_redisplay_function = old_redisplay;
	return 0;
}

int read_key() {
	int ret;
	struct termios t_old, t_new;

	tcgetattr(0, &t_old);
	t_new = t_old;
	t_new.c_lflag = ~ICANON;
	t_new.c_cc[VTIME] = 0;
	t_new.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &t_new);
	ret = getchar();
	tcsetattr(0, TCSANOW, &t_old);
	return ret;
}
