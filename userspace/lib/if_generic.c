#include <sys/ioctl.h>

#include "if_generic.h"

int if_parse_generic(const char *name, const char *type)
{
	int ret, n;
	char *fmt;
	size_t len;

	fmt = alloca((len = strlen(type)) + 5);
	strcpy(fmt, type);
	strcpy(fmt + len, "%d%n");

	if (!sscanf(name, fmt, &ret, &n))
		return -1;

	if (strlen(name) != n)
		return -1;

	return ret;
}

int if_get_index(const char *name, int sock_fd)
{
	struct ifreq ifr;
	int ret = 0;

	if (strlen(name) >= IFNAMSIZ)
		return 0;

	strcpy(ifr.ifr_name, name);

	assert(sock_fd != -1);

	if (!ioctl(sock_fd, SIOCGIFINDEX, &ifr))
		ret = ifr.ifr_ifindex;

	return ret;
}
